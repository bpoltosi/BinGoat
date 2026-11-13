# BinGoat — Simulador de Bingo em C

Aplicação de terminal desenvolvida em C para simular partidas de bingo com múltiplos jogadores, geração automática de cartelas e sorteios sem repetição.

## Funcionalidades

- Cadastro de **2 a 5 jogadores** por partida, cada um com um identificador (ID).
- Geração de cartelas individuais com **15 números distintos**, entre 1 e 60.
- Cartelas geradas sem números repetidos ou consecutivos.
- Sorteio aleatório de números de 1 a 60, sem repetir números já sorteados.
- Marcação automática dos números sorteados nas cartelas.
- Identificação de vencedores quando todos os 15 números de uma cartela forem marcados.
- Exibição de um ranking final, ordenado pela rodada em que cada jogador venceu.
- Avanço manual entre rodadas com **Enter** ou avanço automático com **P**, até surgir um novo vencedor.
- Exibição das cartelas ainda em jogo, com um asterisco (`*`) ao lado dos números já marcados.

A partida termina quando todos os jogadores vencem ou quando os 60 números disponíveis são sorteados.

## Requisitos

- Compilador **GCC**.
- No Windows, recomenda-se utilizar o **MinGW-w64** para disponibilizar o GCC.

O código utiliza recursos da linguagem C compatíveis com C11.

## Compilação e execução

### Linux e macOS

No terminal, dentro da pasta do projeto:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 TrabalhoBingo.c -o bingo
./bingo
```

### Windows (MinGW-w64)

No terminal com o GCC do MinGW-w64 configurado no PATH:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 TrabalhoBingo.c -o bingo.exe
bingo.exe
```

## Como jogar

1. Informe a quantidade de jogadores (de 2 a 5).
2. Digite o nome de cada jogador.
3. A cada rodada, o programa realiza um sorteio e marca automaticamente os números correspondentes nas cartelas.
4. Pressione **Enter** para avançar uma rodada.
5. Digite **P** e pressione Enter para avançar automaticamente até que um novo jogador vença. Depois de uma vitória, o jogo volta a aguardar uma ação para continuar.

Cada cartela é apresentada em três linhas e cinco colunas. O asterisco (`*`) indica que o número já foi sorteado e marcado.

## Estrutura do projeto

```text
BinGoat/
├── README.md
├── .gitignore
└── TrabalhoBingo.c
```
