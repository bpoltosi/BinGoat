# Migração do backend para C nativo

> Proposta técnica para substituir a execução interpretada de `TrabalhoBingo.c` por um backend HTTP compilado em C. Este documento descreve a transição; não realiza a migração nem altera o contrato consumido pelo frontend.

## 1. Objetivo e decisão arquitetural

O objetivo é fazer com que o próprio processo em C seja responsável por:

- manter o estado da partida ativa;
- aplicar as regras do bingo;
- gerar e atualizar cartelas;
- realizar sorteios;
- calcular marcações, linhas e vencedores;
- receber e responder às requisições HTTP da interface.

A proposta mantém o frontend atual em HTML, CSS e JavaScript. O Vite e o Node.js continuam sendo ferramentas de desenvolvimento e build do frontend, mas deixam de executar a API em produção.

**Decisão recomendada:** usar CivetWeb como servidor HTTP embutido em C e cJSON para interpretar requisições e montar respostas JSON. São dependências pequenas e de código aberto, compatíveis com a arquitetura já registrada no roadmap do projeto.

Não faz parte desta migração adicionar banco de dados, autenticação, múltiplas partidas simultâneas ou salas online. A primeira versão continuará mantendo uma única partida ativa em memória.

## 2. Diagnóstico do estado atual

Na branch `main`, a integração existe, mas ainda funciona como uma ponte Node.js → C:

1. `vite.config.js` registra um middleware que encaminha as rotas `/api` para `handleApiRequest`.
2. `server.js` também usa `handleApiRequest` para atender à API quando a aplicação é executada em produção.
3. `src/server/cEngineRunner.js` mantém o estado atual em `currentState`, serializa operações com `executionQueue`, monta um preâmbulo de macros e lê `TrabalhoBingo.c`.
4. O arquivo C é executado por `runC`, importado de `picoc-js`. O resultado JSON é recuperado da saída padrão e convertido novamente em objeto JavaScript.
5. `package.json` declara `picoc-js` como dependência de runtime.

O modo web de `TrabalhoBingo.c` é ativado por `BINGOAT_WEB_API` e recebe parâmetros por macros como `BINGOAT_CMD`, `BINGOAT_PLAYER_COUNT` e `BINGOAT_CALLED_COUNT`. Cada operação reconstitui os dados enviados pelo Node.js, executa o `main()` do modo web e devolve um snapshot JSON.

Portanto, hoje as regras são executadas pelo código C, mas **o servidor HTTP, o ciclo de vida do processo e o estado da partida continuam sob responsabilidade do Node.js**. O C ainda não é um serviço HTTP nativo e persistente.

## 3. Arquitetura de destino

A separação recomendada é:

```text
Navegador
  HTML + CSS + JavaScript
          |
          | HTTP / JSON — caminhos /api/*
          v
Backend compilado em C
  CivetWeb — servidor e handlers HTTP
  cJSON    — leitura e escrita JSON
  GameState — estado da partida em memória
          |
          v
Motor do bingo em C
  geração de cartelas
  sorteio e histórico
  marcações
  linhas e vencedores

Desenvolvimento:
  Vite (porta 3000) -- proxy /api --> Backend C (porta 8080)

Produção:
  Backend C serve a API e os arquivos estáticos de frontend/dist/
```

### Organização sugerida

A estrutura abaixo é uma referência enxuta, não uma exigência de criar arquivos sem necessidade:

```text
BinGoat/
├── TrabalhoBingo.c             # interface de terminal preservada
├── backend/
│   ├── CMakeLists.txt          # compilação do servidor e dependências
│   ├── include/
│   │   └── bingo.h             # tipos e operações públicas do motor
│   └── src/
│       ├── bingo.c             # regras e estado do jogo
│       ├── http_server.c       # rotas e integração CivetWeb/cJSON
│       └── main.c              # inicialização e encerramento do servidor
├── src/
│   ├── main.js
│   └── services/api.js
└── vite.config.js
```

O motor deve ser extraído do arquivo atual para uma interface reutilizável. `TrabalhoBingo.c` continuará sendo o programa de terminal, chamando o mesmo motor. O servidor HTTP também chamará esse motor. Assim, não haverá duas implementações das regras.

## 4. Etapas de migração

### Etapa 1 — Congelar e testar o comportamento atual

Antes de alterar o motor:

- registrar testes para geração de cartelas, sorteios, marcações, desfazer, reiniciar e vitória;
- validar os limites de 2 a 5 jogadores e 15 números por cartela;
- registrar o formato JSON realmente consumido pelo frontend;
- manter uma cópia funcional da integração atual na branch de origem até a nova API passar pelos testes.

Essa etapa evita que a mudança de tecnologia altere as regras do trabalho original.

### Etapa 2 — Extrair o motor de jogo

Separar as estruturas e operações do bingo da entrada/saída do terminal e do modo web baseado em macros.

O motor deverá expor operações explícitas, por exemplo:

- inicializar uma partida;
- criar ou regenerar cartelas;
- sortear um número;
- alternar uma marcação no tabuleiro;
- alternar uma marcação na cartela;
- sincronizar nomes, jogadores e marcação automática;
- desfazer o último sorteio;
- reiniciar a partida;
- obter um snapshot completo do estado.

As funções não devem ler HTTP, imprimir JSON ou depender de macros geradas pelo JavaScript. Devem receber parâmetros tipados e atualizar uma estrutura `GameState`.

O programa de terminal deve continuar compilando e funcionando com as mesmas regras. A migração não deve substituir o CLI por uma segunda implementação.

### Etapa 3 — Criar o servidor HTTP nativo

Implementar os handlers com CivetWeb e usar cJSON para:

- validar o corpo JSON recebido;
- extrair os parâmetros;
- chamar a operação correspondente do motor;
- montar a resposta no contrato atual;
- devolver códigos HTTP e mensagens de erro consistentes.

O estado da partida deverá existir uma única vez no processo do servidor. As operações que modificam o jogo precisam ser atômicas: como o CivetWeb pode atender requisições em threads diferentes, proteger o estado com um mutex. O snapshot da resposta deve ser montado enquanto o estado estiver protegido; a escrita da resposta HTTP pode ocorrer depois de liberar o bloqueio.

Inicializar o gerador pseudoaleatório uma única vez na inicialização do servidor. Não chamar `srand()` a cada requisição. Nos testes, permitir uma semente controlada para reproduzir cenários.

### Etapa 4 — Preservar o contrato da API

O frontend já utiliza caminhos relativos em `src/services/api.js`. A primeira migração deve manter esses caminhos e o formato de resposta, evitando uma refatoração simultânea do frontend.

| Método | Endpoint | Operação |
|---|---|---|
| GET | `/api/health` | Verificar se o servidor e o motor estão prontos |
| GET | `/api/games/current?boardCols=10` | Consultar o estado atual |
| POST | `/api/games` | Criar partida e gerar cartelas |
| POST | `/api/games/current/draw` | Sortear exatamente um número |
| POST | `/api/games/current/toggle-board` | Alternar um número no tabuleiro |
| POST | `/api/games/current/toggle-card` | Alternar uma marcação na cartela |
| POST | `/api/games/current/sync` | Sincronizar jogadores, nomes e opções |
| POST | `/api/games/current/undo` | Desfazer o último sorteio |
| POST | `/api/games/current/reset` | Limpar a partida ou gerar novas cartelas |

Os corpos atualmente utilizados incluem:

- criação: `playerCount`, `players`, `boardCols` e `autoMark`;
- sorteio: `boardCols` e `autoMark`;
- tabuleiro: `number`, `boardCols` e `autoMark`;
- cartela: `playerId`, `number` e `boardCols`;
- sincronização: `playerCount`, `playerNames`, `regenerateCards`, `autoMark` e `boardCols`;
- reinício: `newCards` e `boardCols`.

A resposta de jogo deve continuar expondo os campos usados por `src/main.js`:

- `drawnNumber`;
- `autoMark`;
- `calledNumbers`;
- `boardLines`;
- `players`, com `id`, `name`, `numbers`, `markedNumbers`, `wonRound` e `lines`.

A resposta de erro atual usa o formato simples `{ "error": "mensagem" }`. Mantê-lo durante a migração reduz o risco de incompatibilidade. Uma evolução do contrato de erros pode ser feita depois, em alteração separada.

### Etapa 5 — Conectar Vite e remover a ponte Node.js

Durante o desenvolvimento, o Vite deverá apenas servir o frontend e encaminhar `/api` para o servidor C, por exemplo em `http://127.0.0.1:8080`.

Após a API nativa estar validada:

- remover o middleware `bingoCPlugin` de `vite.config.js` e configurar o proxy de desenvolvimento;
- remover `src/server/cEngineRunner.js`;
- remover `server.js` como servidor HTTP de produção;
- remover `picoc-js` de `package.json` e atualizar o lockfile utilizado pelo projeto;
- manter `src/services/api.js` com `API_BASE_URL = "/api"`;
- configurar o servidor C para servir os arquivos de `dist/` em produção, incluindo fallback para `index.html` nas rotas da interface.

O Node.js/npm continuará necessário para instalar dependências do frontend e executar `vite`/build. O que deixa de existir é a dependência de Node.js para executar o motor e atender à API em produção.

### Etapa 6 — Build, validação e documentação

Usar CMake para padronizar a compilação do servidor e fixar versões das dependências CivetWeb e cJSON. Não versionar executáveis ou arquivos temporários de build.

Validar pelo menos:

1. GCC em Linux;
2. MinGW-w64 em Windows;
3. build do frontend com `npm run build`;
4. execução do CLI original;
5. inicialização do servidor C;
6. fluxo completo no navegador;
7. requisições inválidas e chamadas simultâneas de sorteio.

## 5. Regras de estado, concorrência e segurança

- **Estado:** uma partida ativa em memória por processo. Reiniciar o servidor perde a partida; persistência não faz parte desta etapa.
- **Concorrência:** proteger leitura e escrita de `GameState`; duas requisições simultâneas não podem sortear o mesmo número nem sobrescrever atualizações.
- **Validação:** limitar jogadores a 2–5, números a 1–60, IDs de jogador válidos e corpos JSON a um tamanho máximo.
- **JSON:** usar cJSON para escapar nomes e strings. Não montar JSON com `printf` nem concatenar conteúdo fornecido pelo usuário.
- **Erros:** retornar JSON e status HTTP coerentes para corpo inválido, recurso inexistente, partida encerrada e falha interna.
- **Arquivos estáticos:** impedir traversal de diretório e servir apenas arquivos dentro de `dist/`.
- **Configuração:** permitir configurar host e porta por variáveis de ambiente ou argumentos; não fixar endereços de rede no código.

## 6. Critérios de aceite do backend C

- [ ] O servidor HTTP é um executável compilado em C.
- [ ] O backend não usa `picoc-js`, `runC` nem gera macros para executar o programa a cada requisição.
- [ ] O motor é compartilhado entre o CLI e a API, sem duplicação das regras.
- [ ] Todas as rotas existentes continuam funcionando com o contrato atual.
- [ ] O estado permanece consistente entre requisições.
- [ ] Sorteios simultâneos são serializados corretamente.
- [ ] Cartelas, marcações, histórico, desfazer e vencedores passam nos testes.
- [ ] O CLI original continua compilando e executando.
- [ ] A interface funciona via Vite em desenvolvimento e via servidor C em produção.
- [ ] A compilação está documentada para Windows e Linux.

## 7. Ordem de entrega e estratégia de revisão

A implementação deve ser dividida em PRs pequenos, nesta ordem:

1. testes de caracterização do motor atual;
2. extração do motor C reutilizável, mantendo o CLI;
3. servidor HTTP C e testes do contrato;
4. troca do proxy Vite e integração ponta a ponta;
5. remoção da ponte Node.js/`picoc-js` e atualização da documentação.

Não remover a implementação atual antes de a nova API passar pelos testes de contrato e pelo fluxo manual completo. Cada etapa deve ser revisada antes do merge da seguinte.

**Este PR propõe e documenta a migração. Não altera o backend em execução, não remove dependências e não faz merge automático.**
