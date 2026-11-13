$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
Push-Location $Root
$Build = Join-Path ([System.IO.Path]::GetTempPath()) ("bingoat-c-tests-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $Build | Out-Null
try {
 $Flags = @("-std=c11","-Wall","-Wextra","-Wpedantic","-O2")
 & gcc @Flags "tests/test_engine.c" "bingo_engine.c" "-o" (Join-Path $Build "test-engine.exe")
 if($LASTEXITCODE -ne 0){throw "Falha ao compilar testes de caracterizacao"}
 & (Join-Path $Build "test-engine.exe")
 if($LASTEXITCODE -ne 0){throw "Falha nos testes de caracterizacao"}
 & gcc @Flags "tests/test_engine_state.c" "bingo_engine.c" "-o" (Join-Path $Build "test-engine-state.exe")
 if($LASTEXITCODE -ne 0){throw "Falha ao compilar testes da API publica"}
 & (Join-Path $Build "test-engine-state.exe")
 if($LASTEXITCODE -ne 0){throw "Falha nos testes da API publica"}
 Write-Host "Suite C concluida com sucesso."
} finally { Remove-Item -Recurse -Force $Build -ErrorAction SilentlyContinue; Pop-Location }
