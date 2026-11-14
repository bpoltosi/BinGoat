# Servidor HTTP nativo

O executável `bingo-server` inicializa o motor reutilizável (`bingo_engine.c`) e o servidor CivetWeb. O módulo `backend/http_json.c` também oferece leitura de corpos JSON com `Content-Type` validado, limite de 1 MiB e erros explícitos para handlers da API. O contrato completo das rotas de jogo permanece na Issue #5; nesta etapa existe somente `GET /api/health`.

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

Os testes compilam o executável, aguardam a prontidão, verificam o JSON do health check, a entrega do index/assets, o fallback SPA, 404 de API, rejeição de tentativa de sair de `dist/` e encerramento SIGTERM no Linux. A suíte unitária do parser JSON roda com `ctest --test-dir build --output-on-failure`. O workflow `.github/workflows/c-server.yml` executa build GCC/Linux, smoke tests e build MinGW-w64/Windows.

## Health check

```sh
curl -i http://127.0.0.1:8080/api/health
```

Resposta esperada (HTTP 200):

```json
{"status":"ok","service":true,"engine":"ready"}
```

O status só é 200 depois que `bingo_game_init` conclui com sucesso. Se o motor não estiver pronto, o endpoint responde HTTP 503 com `engine: "not_ready"`.

## Arquivos estáticos e segurança

- CivetWeb recebe `document_root=dist`, desativa listagem de diretórios e usa `index.html`.
- Arquivos existentes são servidos pelo mecanismo estático do CivetWeb, que mantém a resolução de caminhos confinada à raiz configurada.
- Rotas GET/HEAD sem extensão e fora de `/api` recebem fallback para o `dist/index.html`, permitindo navegação SPA.
- Caminhos com segmentos de travessia, barras invertidas, codificação percentual ou caracteres de controle são rejeitados no fallback.
- Rotas desconhecidas sob `/api` recebem 404 JSON e nunca caem no HTML da aplicação.
- O servidor não cria, altera nem remove arquivos de `dist`.

## Concorrência e ciclo de vida

O estado do motor fica em uma única instância em memória. O acesso é serializado por um spinlock C11 (`atomic_flag`), portável entre GCC/Linux e GCC/MinGW-w64. Toda operação futura que leia ou altere `BingoGame` deve adquirir o mesmo lock; handlers não devem manter referências ao estado depois de liberá-lo. O health check já segue essa política.

O processo trata SIGINT/SIGTERM, interrompe o loop de espera, chama `mg_stop` para aguardar/encerrar os workers do CivetWeb e libera a biblioteca com `mg_exit_library`. No Windows, Ctrl+C aciona SIGINT.

## Desenvolvimento com Vite

O frontend deve continuar chamando caminhos relativos `/api`. Durante o desenvolvimento, configure o proxy do Vite para encaminhar `/api` ao servidor C em `http://127.0.0.1:8080`. A alteração efetiva do proxy e a remoção da ponte Node.js são escopo da Issue #16; o contrato de rotas é escopo da Issue #5.

## Fora do escopo

Esta entrega não implementa criação/sorteio/consulta de partidas, não remove `server.js` ou `picoc-js` e não altera o frontend. Essas etapas dependem da API #5 e da integração #10.
