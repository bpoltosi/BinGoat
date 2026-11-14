# Servidor HTTP nativo

O executável `bingo-server` inicializa o motor reutilizável (`bingo_engine.c`) e o servidor CivetWeb. O módulo `backend/http_json.c` valida corpos JSON, limita o tamanho a 1 MiB e fornece respostas de erro explícitas. O servidor expõe a API de jogo e serve os arquivos estáticos do frontend; o motor C é a única fonte do estado da partida.

## Dependências fixadas

| Dependência | Versão | Licença | Responsabilidade |
|---|---:|---|---|
| CivetWeb | 1.16 | MIT | Servidor HTTP e arquivos estáticos |
| cJSON | 1.7.19 | MIT | Serialização JSON |
| CMake | 3.16+ | — | Configuração e obtenção das dependências |

O CMake baixa as versões fixadas dos repositórios oficiais na primeira configuração.

## Compilar e executar

Linux/macOS:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/bingo-server --host 127.0.0.1 --port 8080 --dist dist
```

Windows/MinGW-w64:

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
.\build\bingo-server.exe --host 127.0.0.1 --port 8080 --dist dist
```

Gere `dist/` antes de iniciar a versão de produção: `npm install && npm run build`. Durante o desenvolvimento, use `--dist .` para servir os arquivos de desenvolvimento se necessário; normalmente o Vite serve a interface e faz proxy da API.

Padrões: host `127.0.0.1`, porta `8080`, diretório `dist`. Argumentos `--host`, `--port` e `--dist` sobrescrevem as variáveis `BINGOAT_HOST`, `BINGOAT_PORT` e `BINGOAT_DIST`. O host padrão é loopback para evitar exposição involuntária na rede.

## Integração com Vite

O frontend chama caminhos relativos `/api`. `vite.config.js` encaminha essas chamadas ao backend C em `http://127.0.0.1:8080`. Inicie `bingo-server` em um terminal e `npm run dev` em outro. Para alterar o destino do proxy, defina `BINGOAT_API_TARGET`. Vite é somente servidor de desenvolvimento e ferramenta de build; não executa handlers, regras do jogo ou estado de backend.

## Arquivos estáticos e segurança

- CivetWeb usa `document_root` configurado em `--dist`, desativa listagem de diretórios e serve `index.html`.
- Arquivos existentes são servidos pelo mecanismo estático do CivetWeb, com caminhos confinados à raiz configurada.
- Rotas GET/HEAD sem extensão e fora de `/api` recebem fallback SPA para `index.html`.
- Caminhos inválidos e tentativas de travessia são rejeitados; rotas desconhecidas em `/api` recebem 404 JSON e nunca o HTML da aplicação.
- O servidor não altera arquivos do diretório estático.

## API

Todas as respostas de sucesso são JSON. Os valores aceitos para `boardCols` são 5, 6 ou 10.

| Método | Rota | Corpo / parâmetros |
|---|---|---|
| GET | `/api/health` | — |
| GET | `/api/games/current?boardCols=10` | `boardCols` opcional |
| POST | `/api/games/current/draw` | `boardCols`, `autoMark` opcionais |
| POST | `/api/games/current/toggle-board` | `number`, `boardCols`, `autoMark` |
| POST | `/api/games/current/toggle-card` | `playerId`, `number`, `boardCols` |
| POST | `/api/games/current/sync` | `playerCount`, `playerNames`, `regenerateCards`, `autoMark`, `boardCols` |
| POST | `/api/games/current/undo` | `boardCols` opcional |
| POST | `/api/games/current/reset` | `newCards`, `boardCols` opcionais |

POST exige `Content-Type: application/json`. Os erros retornam JSON e status HTTP apropriados (400, 405, 409, 413, 415 ou 503). A partida inicia com dois jogadores; `sync` aceita de 2 a 5 jogadores.

## Health check

```sh
curl -i http://127.0.0.1:8080/api/health
```

Resposta esperada (HTTP 200):

```json
{"status":"ok","service":true,"engine":"ready"}
```

O status 200 só é retornado após a inicialização bem-sucedida do motor.

## Concorrência e ciclo de vida

O estado do motor fica em uma única instância em memória. Operações são serializadas por lock C11 (`atomic_flag`). O processo trata SIGINT/SIGTERM, encerra os workers do CivetWeb e libera a biblioteca ao finalizar.

## Testes HTTP

- Linux/macOS: `sh tests/run-server-tests.sh`
- Windows/MinGW-w64: `./tests/run-server-tests.ps1`
- Proxy Vite → API C (Linux): `sh tests/run-vite-proxy-test.sh`
- Testes unitários: `ctest --test-dir build --output-on-failure`

O workflow `.github/workflows/c-server.yml` executa o build GCC/Linux, smoke tests HTTP, build do frontend e teste do proxy; também compila o servidor com MinGW-w64 no Windows.
