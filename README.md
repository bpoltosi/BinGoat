# BinGoat — Bingo em C com interface web

O BinGoat é uma aplicação de bingo com regras implementadas em C e uma interface web construída com HTML, CSS e JavaScript. O projeto preserva também a execução original pelo terminal.

## Estado atual da implementação

A branch `main` já possui uma interface web integrada à API. A integração atual, porém, ainda usa uma camada Node.js:

- **Motor:** `TrabalhoBingo.c` contém as regras de geração de cartelas, sorteio, marcação e vitória.
- **Adaptador da API:** `src/server/cEngineRunner.js` recebe as requisições, mantém o estado da partida e executa o arquivo C usando `picoc-js`.
- **Desenvolvimento:** `vite.config.js` registra o middleware da API junto ao Vite.
- **Produção atual:** `server.js` atende as rotas HTTP e serve os arquivos gerados em `dist/`.
- **Frontend:** `src/main.js` e os componentes visuais consomem a API por `src/services/api.js`.

Assim, as regras do jogo são processadas pelo código C, mas o servidor HTTP e o estado entre requisições ainda dependem do Node.js. **O backend ainda não é um servidor C nativo.**

A transição planejada para um servidor HTTP compilado em C está detalhada em [docs/BACKEND-C-MIGRATION.md](docs/BACKEND-C-MIGRATION.md). O documento descreve a extração do motor reutilizável, a adoção de CivetWeb e cJSON, a preservação do contrato da API e a remoção gradual da ponte Node.js.

## Regras do jogo

- De 2 a 5 jogadores por partida.
- Cada cartela possui 15 números distintos entre 1 e 60.
- Os números de uma cartela são ordenados e não podem se repetir nem ser consecutivos.
- Os sorteios não repetem números.
- A marcação automática acompanha os números sorteados quando habilitada.
- Um jogador vence ao completar os 15 números da cartela.
- A partida pode ser desfeita ou reiniciada.

## Requisitos

### Interface web atual

- Node.js 18 ou superior.
- npm.

### Aplicação de terminal em C

- GCC compatível com C11 ou MinGW-w64 no Windows.

A versão web atual executa o código C por meio de `picoc-js`; não exige GCC para iniciar a interface.

## Executar a aplicação web

Na raiz do repositório:

```bash
npm install
npm run dev
```

Abra [http://localhost:3000](http://localhost:3000).

Para executar a versão de produção atual:

```bash
npm run build
npm start
```

O servidor atual atende a interface e a API na porta 3000. Os arquivos compilados do frontend ficam em `dist/`, que não deve ser versionada.

## Executar a versão de terminal

### Linux e macOS

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 TrabalhoBingo.c -o bingo
./bingo
```

### Windows (MinGW-w64)

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 TrabalhoBingo.c -o bingo.exe
.\bingo.exe
```

## API atual

A interface utiliza os seguintes endpoints:

| Método | Endpoint | Finalidade |
|---|---|---|
| GET | `/api/health` | Verificar a disponibilidade da API |
| GET | `/api/games/current` | Consultar a partida atual |
| POST | `/api/games` | Criar uma partida |
| POST | `/api/games/current/draw` | Sortear um número |
| POST | `/api/games/current/toggle-board` | Alternar marcação no tabuleiro |
| POST | `/api/games/current/toggle-card` | Alternar marcação em uma cartela |
| POST | `/api/games/current/sync` | Sincronizar jogadores e opções |
| POST | `/api/games/current/undo` | Desfazer o último sorteio |
| POST | `/api/games/current/reset` | Limpar a partida ou gerar novas cartelas |

O estado da partida é mantido em memória pelo backend atual. Reiniciar o servidor encerra a partida; não há persistência em banco de dados.

## Arquitetura atual

```text
Navegador
  HTML + CSS + JavaScript
          |
          | HTTP / JSON
          v
Node.js / Vite
  src/server/cEngineRunner.js
          |
          | picoc-js
          v
TrabalhoBingo.c
  regras do jogo e saída JSON
```

## Próxima evolução

A próxima etapa arquitetural é substituir o adaptador Node.js por um servidor HTTP nativo em C, mantendo o frontend e o contrato atual da API. O plano está em [Migração do backend para C nativo](docs/BACKEND-C-MIGRATION.md).

A migração deverá preservar o executável de terminal e evitar duplicar as regras do bingo. O servidor C passará a manter o estado da partida, atender as rotas HTTP e servir o frontend compilado. Node.js/npm continuarão sendo usados apenas no fluxo de desenvolvimento e build do frontend.
