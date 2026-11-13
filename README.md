# BinGoat — Bingo em C com interface web em desenvolvimento

O BinGoat é um projeto de bingo cuja lógica principal é desenvolvida em C. O repositório mantém a aplicação de terminal original e prepara uma interface web independente, que futuramente consumirá uma API HTTP fornecida pelo backend em C.

## Estado atual

- **Aplicação C de terminal:** implementação original preservada em `TrabalhoBingo.c`.
- **Frontend web:** estrutura inicial criada com Vite, HTML, CSS e JavaScript.
- **Integração HTTP:** cliente base preparado em `src/services/api.js`; ainda depende da implementação dos endpoints no backend C.
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
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 TrabalhoBingo.c -o bingo
./bingo
```

### Windows (MinGW-w64)

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 TrabalhoBingo.c -o bingo.exe
bingo.exe
```

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

## Arquitetura planejada

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

A arquitetura final prevista é:

- **C:** regras e estado da partida.
- **CivetWeb + cJSON:** servidor HTTP e serialização JSON no backend.
- **Vite:** ambiente de desenvolvimento e build do frontend.
- **HTML/CSS/JavaScript/SVG:** interface, cartelas e animações 2D.

O frontend deverá consumir a API por caminhos relativos sob `/api`. Em desenvolvimento, o Vite poderá encaminhar essas requisições ao servidor C por meio de um proxy. Em produção, o servidor HTTP poderá servir os arquivos gerados em `dist/`.

## Endpoints previstos

Os caminhos abaixo são parte do planejamento e **ainda não estão implementados**:

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
