# Bingo LDP

Projeto em C para simular partidas de bingo no terminal.

## Recursos

- Cartelas com 15 numeros distintos, sorteados entre 1 e 60.
- Cadastro de 1 a 100 jogadores.
- Nomes repetidos recebem um identificador para diferenciacao.
- Sorteio sem repeticao e historico dos numeros sorteados.
- Marcacao automatica das cartelas e registro de todos os vencedores.
- Resultado final com vencedores ordenados pela rodada da vitoria e jogadores que nao completaram a cartela.
- Avanco manual por rodada ou opcao **p** para avancar ate surgir um novo vencedor.

## Compilar

Com GCC instalado, dentro desta pasta execute:

```sh
make
```

Ou compile diretamente:

```sh
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 TrabalhoBingo.c -o bingo
```

## Executar

Linux/macOS:

```sh
./bingo
```

Windows (MinGW):

```sh
bingo.exe
```

Durante a partida, pressione **Enter** para avancar uma rodada ou digite **p** e pressione Enter para pular ate o proximo vencedor. O jogo termina quando todos vencem ou quando os 60 numeros tiverem sido sorteados.

## Cartela

Cada cartela e exibida em tres linhas por cinco colunas. Numeros entre colchetes ja foram sorteados e marcados.
