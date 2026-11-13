# Testes do motor C

Execute sem Node.js ou frontend:

- Linux/macOS: `sh tests/run-tests.sh`
- Windows (MinGW-w64): `./tests/run-tests.ps1`

A suíte cobre o comportamento caracterizado do CLI e a API pública (`bingo_engine.h`): validação de 2 a 5 jogadores, geração de cartelas, sorteios reproduzíveis e únicos, marcação, vitória, linhas, desfazer, reinício, sincronização e alternância manual. Os binários são temporários.
