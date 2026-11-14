# BinGoat

Bingo com motor, regras e backend HTTP em C, interface web com Vite e aplicação de terminal preservada. O executável `bingo-server` (CivetWeb + cJSON) é o único servidor HTTP e a única fonte autoritativa do estado do jogo.

## Componentes

- `bingo_engine.c/.h`: motor reutilizável do bingo.
- `TrabalhoBingo.c`: interface CLI, compilada separadamente.
- `backend/server.c` e `backend/game_api.c`: servidor HTTP nativo e API.
- `src/`: frontend JavaScript, HTML e CSS; usa apenas o cliente HTTP `src/services/api.js`.
- Vite: servidor de desenvolvimento, proxy de `/api` para o servidor C e build estático. Node.js/npm não executam lógica de backend nem atendem a aplicação em produção.

## Requisitos

- GCC C11 e CMake 3.16+.
- Node.js 18+ e npm, somente para instalar dependências e executar Vite/build.
- Git (CMake baixa CivetWeb 1.16 e cJSON 1.7.19 na primeira configuração).

## Executar o frontend em desenvolvimento

Terminal 1 — compilar e iniciar o servidor C:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/bingo-server --host 127.0.0.1 --port 8080 --dist .
```

Terminal 2 — instalar dependências e iniciar Vite:

```sh
npm install
npm run dev
```

Abra `http://localhost:3000`. O proxy do Vite encaminha `/api` para `http://127.0.0.1:8080`. Para outro destino, defina `BINGOAT_API_TARGET` antes de iniciar o Vite. Verifique o backend em `http://127.0.0.1:8080/api/health`.

## Executar em produção

Na raiz do projeto:

```sh
npm install
npm run build
./build/bingo-server --host 127.0.0.1 --port 8080 --dist dist
```

O executável C serve os arquivos de `dist/` e as rotas `/api/*` no mesmo processo. O diretório `dist/` é gerado e não deve ser versionado. No Windows/MinGW-w64, compile com:

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
.\build\bingo-server.exe --host 127.0.0.1 --port 8080 --dist dist
```

O host/porta/diretório também podem ser configurados por `BINGOAT_HOST`, `BINGOAT_PORT` e `BINGOAT_DIST`. Consulte [backend/README.md](backend/README.md) para detalhes da API, configuração e testes HTTP.

## Executar o CLI

Linux/macOS:

```sh
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 TrabalhoBingo.c bingo_engine.c -o bingo
./bingo
```

Windows/MinGW-w64:

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 TrabalhoBingo.c bingo_engine.c -o bingo.exe
.\bingo.exe
```

## Testes

- Motor e CLI: `sh tests/run-tests.sh` (Linux/macOS) ou `./tests/run-tests.ps1` (Windows).
- Servidor HTTP: `sh tests/run-server-tests.sh` (Linux/macOS) ou `./tests/run-server-tests.ps1` (Windows).
- Proxy Vite → servidor C: `sh tests/run-vite-proxy-test.sh` (Linux; requer Node.js/npm, CMake, GCC, Git e curl).
- Parser JSON: `ctest --test-dir build --output-on-failure`.

O workflow `.github/workflows/c-server.yml` valida o build do frontend, servidor C e proxy no Linux/GCC, além da compilação MinGW-w64 no Windows.

## Manutenção do repositório

Consulte [docs/REPOSITORY-AUDIT.md](docs/REPOSITORY-AUDIT.md) para o inventário de arquivos, decisões de manutenção e itens que não devem ser removidos.

## API

- `GET /api/health`
- `GET /api/games/current?boardCols=10`
- `POST /api/games/current/draw`
- `POST /api/games/current/toggle-board`
- `POST /api/games/current/toggle-card`
- `POST /api/games/current/sync`
- `POST /api/games/current/undo`
- `POST /api/games/current/reset`

O contrato e os campos JSON estão documentados em [backend/README.md](backend/README.md). A aplicação mantém uma partida em memória, sem persistência ou autenticação nesta etapa.
