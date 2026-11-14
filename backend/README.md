# Servidor HTTP nativo

O executável `bingo-server` inicializa o motor reutilizável (`bingo_engine.c`) e o servidor CivetWeb. O módulo `backend/http_json.c` também oferece leitura de corpos JSON com `Content-Type` validado, limite de 1 MiB e erros explícitos para handlers da API. O servidor expõe health check e operações de partida em `/api/games/current`, mantendo o motor C como fonte única do estado.

## Dependências fixadas

| Dependência | Versão | Licença | Responsabilidade |
|---|---:|---|---|
| CivetWeb | 1.16 | MIT | Servidor HTTP e entrega de arquivos estáticos |
| cJSON | 1.7.19 | MIT | Criação e serialização de respostas JSON |
| CMake | 3.16+ | — | Configuração e obtenção reproduzível das dependências |

O CMake baixa as versões fixadas dos repositórios oficiais na primeira configuração. As fontes e licenças das dependências ficam no diretório de build, sem copiar código de terceiros para o repositório.

## Linux/macOS (GCC)

Na raiz do repositório:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/bingo-server
```

Para servir o frontend, gere primeiro o build do Vite (`npm install && npm run build`). O servidor usa `dist/` como raiz estática por padrão.

## Windows (MinGW-w64)

Abra um terminal com GCC, CMake e MinGW-w64 disponíveis no PATH:

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
.\build\bingo-server.exe
```

Se estiver usando um gerador diferente, omita `-G "MinGW Makefiles"`. O mesmo CMakeLists é usado nos dois sistemas.

## Configuração

Padrões: host `127.0.0.1`, porta `8080`, diretório estático `dist`.

Argumentos (têm precedência sobre variáveis de ambiente):

```sh
./build/bingo-server --host 127.0.0.1 --port 8080 --dist dist
```

Variáveis equivalentes: `BINGOAT_HOST`, `BINGOAT_PORT` e `BINGOAT_DIST`.

O host padrão é loopback para não expor o serviço na rede local sem decisão explícita. Para acesso externo, configure conscientemente outro endereço e aplique controles de rede adequados.

## Testes de fumaça HTTP

Após instalar GCC, CMake, Git e curl:

- Linux/macOS: `sh tests/run-server-tests.sh`
- Windows/MinGW-w64: `./tests/run-server-tests.ps1`

Os testes compilam o executável, aguardam a prontidão, verificam o JSON do health check, a entrega do index/assets, o fallback SPA, 404 de API, rejeição de tentativa de sair de `dist/` e encerramento SIGTERM no Linux. Com as dependências npm instaladas e o frontend compilado, `sh tests/run-vite-proxy-test.sh` inicia o servidor C e o Vite e confirma que `/api/health` e `/api/games/current` chegam ao backend nativo por meio do proxy. A suíte unitária do parser JSON roda com `ctest --test-dir build --output-on-failure`. Os testes de fumaça também exercitam consulta, sorteio, marcação do tabuleiro e de cartela, sincronização, desfazer e reset. O workflow `.github/workflows/c-server.yml` executa build GCC/Linux, smoke tests e build MinGW-w64/Windows.

## API de jogo\n\nTodas as respostas de sucesso são JSON e incluem `calledNumbers`, `boardLines`, `autoMark` e `players`. Cada jogador contém `id`, `name`, `numbers`, `markedNumbers`, `wonRound` (nulo enquanto não vence) e `lines`. Linhas completas incluem `key`, `orientation`, `label` e `numbers`. Os valores aceitos para `boardCols` são 5, 6 ou 10.\n\n| Método | Rota | Corpo / parâmetros |\n|---|---|---|\n| GET | `/api/games/current?boardCols=10` | `boardCols` opcional |\n| POST | `/api/games/current/draw` | `boardCols`, `autoMark` opcionais |\n| POST | `/api/games/current/toggle-board` | `number`, `boardCols`, `autoMark` |\n| POST | `/api/games/current/toggle-card` | `playerId`, `number`, `boardCols` |\n| POST | `/api/games/current/sync` | `playerCount`, `playerNames`, `regenerateCards`, `autoMark`, `boardCols` (todos opcionais) |\n| POST | `/api/games/current/undo` | `boardCols` opcional |\n| POST | `/api/games/current/reset` | `newCards`, `boardCols` opcionais |\n\nCorpos POST devem usar `Content-Type: application/json`. Erros retornam JSON com `error` e `code`; status usados incluem 400, 405, 409, 413, 415 e 503. A partida inicia com dois jogadores. `sync` aceita de 2 a 5 jogadores e nomes de até 32 caracteres.\n\n## Health check

```sh
curl -i http://127.0.0.1:8080/api/health
```

Resposta esperada (HTTP 200):

```json
{"status":"ok","service":true,"engine":"ready"}
```

O status é 200 somente depois que `bingo_game_init` conclui com sucesso. Se a inicialização do motor falhar, o servidor HTTP ainda sobe para health check e arquivos estáticos, mas responde HTTP 503 com `engine: "not_ready"`; as operações de jogo devem permanecer indisponíveis.

## Arquivos estáticos e segurança

- CivetWeb recebe `document_root=dist`, desativa listagem de diretórios e usa `index.html`.
- Arquivos existentes são servidos pelo mecanismo estático do CivetWeb, que mantém a resolução de caminhos confinada à raiz configurada.
- Rotas GET/HEAD sem extensão e fora de `/api` recebem fallback para o `dist/index.html`, permitindo navegação SPA.
- Caminhos com segmentos de travessia, barras invertidas, codificação percentual ou caracteres de controle são rejeitados no fallback.
- Rotas desconhecidas sob `/api` recebem 404 JSON e nunca caem no HTML da aplicação.\n- As operações da partida são serializadas por lock próprio; nenhuma referência ao estado é mantida após a resposta.
- O servidor não cria, altera nem remove arquivos de `dist`.

## Concorrência e ciclo de vida

O estado do motor fica em uma única instância em memória. O acesso é serializado por um spinlock C11 (`atomic_flag`), portável entre GCC/Linux e GCC/MinGW-w64. Toda operação futura que leia ou altere `BingoGame` deve adquirir o mesmo lock; handlers não devem manter referências ao estado depois de liberá-lo. O health check já segue essa política.

O processo trata SIGINT/SIGTERM, interrompe o loop de espera, chama `mg_stop` para aguardar/encerrar os workers do CivetWeb e libera a biblioteca com `mg_exit_library`. No Windows, Ctrl+C aciona SIGINT.

## Desenvolvimento com Vite

O frontend chama caminhos relativos `/api`. O proxy de desenvolvimento já está configurado em `vite.config.js` para encaminhar essas chamadas ao servidor C em `http://127.0.0.1:8080`. Para usar outro endereço, defina `BINGOAT_API_TARGET` antes de iniciar o Vite. Inicie o servidor C em um terminal e execute `npm run dev` em outro.

## Fora do escopo

A integração do frontend com o servidor C e o proxy do Vite fazem parte da Issue #10. A remoção da ponte Node.js (`server.js`, `src/server/cEngineRunner.js` e `picoc-js`) permanece isolada na Issue #16.
