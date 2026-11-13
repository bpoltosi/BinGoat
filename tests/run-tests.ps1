$ErrorActionPreference = "Stop"

$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$BuildDirectory = Join-Path ([System.IO.Path]::GetTempPath()) ("bingoat-c-tests-" + [guid]::NewGuid().ToString("N"))
$Compiler = if ($env:CC) { $env:CC } else { "gcc" }

New-Item -ItemType Directory -Path $BuildDirectory | Out-Null
Push-Location $RepositoryRoot

try {
    $EngineExecutable = Join-Path $BuildDirectory "test-engine.exe"

    & $Compiler -std=c11 -Wall -Wextra -Wpedantic -O2 "tests/test_engine.c" "-o" $EngineExecutable
    if ($LASTEXITCODE -ne 0) {
        throw "Falha ao compilar os testes do motor C (GCC/MinGW-w64)."
    }

    & $EngineExecutable
    if ($LASTEXITCODE -ne 0) {
        throw "Os testes do motor C falharam."
    }

    foreach ($Command in @(6, 7)) {
        $WebExecutable = Join-Path $BuildDirectory ("test-web-command-{0}.exe" -f $Command)

        & $Compiler -std=c11 -Wall -Wextra -Wpedantic -O2 ("-DBINGOAT_CMD={0}" -f $Command) "tests/test_web_commands.c" "-o" $WebExecutable
        if ($LASTEXITCODE -ne 0) {
            throw "Falha ao compilar o teste do comando web $Command."
        }

        & $WebExecutable
        if ($LASTEXITCODE -ne 0) {
            throw "O teste do comando web $Command falhou."
        }
    }

    Write-Host "Suite C concluida com sucesso."
}
finally {
    Remove-Item -Force -ErrorAction SilentlyContinue @(
        "tests/.players-input.tmp",
        "tests/.cli-input.tmp",
        "tests/.cli-output.tmp",
        "tests/.web-command-output.tmp"
    )
    Pop-Location
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $BuildDirectory
}
