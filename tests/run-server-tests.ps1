$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Build = Join-Path $Root "build"
$Server = Join-Path $Build "bingo-server.exe"
$Static = Join-Path $Root "dist"
if (-not (Test-Path (Join-Path $Static "index.html"))) { $Static = Join-Path $Root "tests/fixtures/static" }
$Port = if ($env:BINGOAT_TEST_PORT) { $env:BINGOAT_TEST_PORT } else { "18080" }
$BaseUrl = "http://127.0.0.1:$Port"

cmake -S $Root -B $Build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build $Build --config Release

$Process = Start-Process -FilePath $Server -ArgumentList @("--host", "127.0.0.1", "--port", $Port, "--dist", $Static) -PassThru -NoNewWindow
try {
    $Health = $null
    for ($Attempt = 0; $Attempt -lt 30; $Attempt++) {
        try {
            $Health = Invoke-RestMethod "$BaseUrl/api/health" -TimeoutSec 2
            break
        } catch {
            Start-Sleep -Seconds 1
        }
    }

    if ($null -eq $Health -or $Health.status -ne "ok" -or $Health.engine -ne "ready") {
        throw "Health check nao confirmou a prontidao do motor."
    }

    $IndexPath = Join-Path $Static "index.html"
    $RootResponse = Join-Path $Build "root-response.html"
    $SpaResponse = Join-Path $Build "spa-response.html"
    Invoke-WebRequest "$BaseUrl/" -UseBasicParsing -OutFile $RootResponse
    Invoke-WebRequest "$BaseUrl/partida/123" -UseBasicParsing -OutFile $SpaResponse
    if ((Get-FileHash $IndexPath).Hash -ne (Get-FileHash $RootResponse).Hash) {
        throw "O servidor nao entregou o index.html estatico."
    }
    if ((Get-FileHash $IndexPath).Hash -ne (Get-FileHash $SpaResponse).Hash) {
        throw "O fallback SPA nao funcionou."
    }

    $Asset = Get-ChildItem $Static -Recurse -File | Where-Object { $_.FullName -ne $IndexPath } | Select-Object -First 1
    if ($null -ne $Asset) {
        $RelativeAsset = $Asset.FullName.Substring($Static.Length).TrimStart([char[]]@([char]92, [char]47)).Replace([string][char]92, "/")
        $AssetResponse = Join-Path $Build "asset-response"
        Invoke-WebRequest "$BaseUrl/$RelativeAsset" -UseBasicParsing -OutFile $AssetResponse
        if ((Get-FileHash $Asset.FullName).Hash -ne (Get-FileHash $AssetResponse).Hash) {
            throw "O servidor nao entregou corretamente um asset de dist/."
        }
    }

    try {
        Invoke-WebRequest "$BaseUrl/api/rota-inexistente" -UseBasicParsing | Out-Null
        throw "Uma rota de API inexistente deveria retornar 404."
    } catch {
        if ($_.Exception.Response.StatusCode.value__ -ne 404) { throw }
    }

    $TraversalStatus = & curl.exe --path-as-is --silent --output NUL --write-out "%{http_code}" "$BaseUrl/../README.md"
    if ($TraversalStatus -eq "200") { throw "O servidor permitiu acesso fora de dist/." }

    Write-Host "Todos os testes HTTP do servidor passaram."
} finally {
    if (-not $Process.HasExited) {
        Stop-Process -Id $Process.Id -Force
        $Process.WaitForExit()
    }
}
