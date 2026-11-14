$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Build = Join-Path $Root "build"
$Server = Join-Path $Build "bingo-server.exe"
$Static = Join-Path $Root "tests/fixtures/static"
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

    if ((Invoke-WebRequest "$BaseUrl/" -UseBasicParsing).Content -notmatch "BinGoat static fixture") {
        throw "O servidor nao entregou o index.html estatico."
    }
    if ((Invoke-WebRequest "$BaseUrl/partida/123" -UseBasicParsing).Content -notmatch "BinGoat static fixture") {
        throw "O fallback SPA nao funcionou."
    }

    try {
        Invoke-WebRequest "$BaseUrl/api/rota-inexistente" -UseBasicParsing | Out-Null
        throw "Uma rota de API inexistente deveria retornar 404."
    } catch {
        if ($_.Exception.Response.StatusCode.value__ -ne 404) { throw }
    }

    Write-Host "Todos os testes HTTP do servidor passaram."
} finally {
    if (-not $Process.HasExited) {
        Stop-Process -Id $Process.Id -Force
        $Process.WaitForExit()
    }
}
