# BinGoat — Bingo em C com interface web em desenvolvimento

O BinGoat é um projeto de bingo cuja lógica principal é desenvolvida em C. O repositório mantém a aplicação de terminal original, um servidor HTTP nativo em C (CivetWeb + cJSON) e uma interface web independente. Nesta etapa, o servidor expõe o health check; as operações de jogo serão entregues na Issue #5.

## Estado atual

- **Motor C reutilizável:** regras e estado em `bingo_engine.c` / `bingo_engine.h`.
- **CLI:** interface de terminal em `TrabalhoBingo.c`, consumindo o motor compartilhado.
- **Frontend web:** estrutura inicial criada com Vite, HTML, CSS e JavaScript.
- **Servidor HTTP nativo:** executável `bingo-server`, health check em `GET /api/health` e entrega de arquivos de `dist/`; as rotas de jogo ainda dependem da Issue #5.
- **Máquina de bingo e animações:** ainda não implementadas nesta etapa.
- **Dados:** o frontend não usa cartelas, jogadores ou sorteios fictícios.

## Regras atuais do jogo

- De 2 a 5 jogadores por partida.
- Cada cartela possui 15 números distintos entre 1 e 60.
- As cartelas não contêm números repetidos ou consecutivos.
- Os sorteios não repetem números.
- Os números sorteados são marcados automaticamente.
- Um jogador vence ao completar os 15 números da cartela.
- A partida termina quando todos vencem ou quando os 60 números são sorteados.

A versão web deverá preservar essas regras. O backend em C será a única fonte da verdade para geração de cartelas, sorteios, marcações e identificação de vencedores.

## Requisitos

### Aplicação C

- GCC compatível com C11.

### Frontend web

- Node.js 18 ou superior.
- npm.

## Executar a aplicação C

### Linux e macOS

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 TrabalhoBingo.c bingo_engine.c -o bingo
./bingo
```

### Windows (MinGW-w64)

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 TrabalhoBingo.c bingo_engine.c -o bingo.exe
bingo.exe
```

## Testes de caracterização do motor C

A suíte protege o comportamento do CLI e valida a API pública do motor extraído, sem iniciar Node.js ou frontend.

- Linux/macOS: `sh tests/run-tests.sh`
- Windows com MinGW-w64: `.\tests\run-tests.ps1`

A cobertura e os detalhes de reprodutibilidade estão em [tests/README.md](tests/README.md).

## Executar o frontend web

Na raiz do repositório:

```bash
npm install
npm run dev
```

O Vite exibirá no terminal o endereço local para abrir a interface no navegador.

Para validar o build de produção:

```bash
npm run build
npm run preview
```

O resultado do build é gerado em `dist/`, que não deve ser versionada.

## Google AI Studio

Esta branch contém a configuração mínima necessária para o AI Studio reconhecer e iniciar o frontend com npm:

1. Importe a branch `web-app` do repositório.
2. Aguarde a instalação das dependências declaradas em `package.json`.
3. O comando de desenvolvimento configurado é `npm run dev`.
4. A aplicação inicial informa explicitamente que o backend ainda não está conectado.

Se a importação tiver sido feita enquanto outra branch estava selecionada, confirme que o AI Studio está usando `web-app`, e não `main`.

## Arquitetura

```text
BinGoat/
├── TrabalhoBingo.c          # aplicação C original de terminal
├── package.json             # scripts e dependências do frontend
├── index.html               # ponto de entrada do Vite
└── src/
    ├── main.js               # inicialização da interface
    ├── style.css             # estilos responsivos
    └── services/
        └── api.js            # cliente HTTP para a futura API C
```

A arquitetura atual separa domínio e interfaces:

- `bingo_engine.h` / `bingo_engine.c`: tipos e operações reutilizáveis, sem I/O de terminal, HTTP ou JSON.
- `TrabalhoBingo.c`: interface CLI que chama o motor.

A arquitetura final prevista é:

- **C:** regras e estado da partida.
- **CivetWeb + cJSON:** servidor HTTP nativo e serialização JSON; infraestrutura inicial implementada em `backend/server.c`.
- **Vite:** ambiente de desenvolvimento e build do frontend.
- **HTML/CSS/JavaScript/SVG:** interface, cartelas e animações 2D.

O frontend deverá consumir a API por caminhos relativos sob `/api`. Em desenvolvimento, o Vite poderá encaminhar essas requisições ao servidor C por meio de um proxy. Em produção, o servidor HTTP poderá servir os arquivos gerados em `dist/`.

## Endpoints previstos

O health check já está implementado. As operações de jogo abaixo permanecem para a Issue #5:

- `GET /api/health`
- `POST /api/games`
- `GET /api/games/current`
- `POST /api/games/current/draw`
- `POST /api/games/current/reset`

Até a implementação do backend e desses endpoints, a interface não deve simular respostas nem gerar dados de jogo no navegador.

## Próximas etapas

O desenvolvimento deve seguir o roadmap documentado no PR de planejamento da versão web:

1. Separar o motor do bingo da interface de terminal.
2. Criar o servidor HTTP em C.
3. Implementar os endpoints e o contrato JSON.
4. Evoluir o frontend para exibir jogadores e cartelas com dados reais.
5. Criar a máquina de bingo 2D em SVG e suas animações.
6. Integrar o fluxo de sorteio à resposta real da API.
7. Validar a experiência completa e preservar a aplicação de terminal.

## Escopo desta branch

A branch `web-app` prepara apenas a base de execução do frontend. Ela não substitui a aplicação C, não implementa ainda o backend HTTP e não altera a branch `main`.
