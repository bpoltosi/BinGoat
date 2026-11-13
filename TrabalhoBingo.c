#include "bingo_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void descartarRestanteLinha(void)
{
    int c;

    while ((c = getchar()) != '\n' && c != EOF)
    {
        /* Descarta o restante da entrada */
    }
}

int aguardaEnter(int *modoPular)
{
    char c;

    if (*modoPular)
    {
        return 1;
    }

    printf("\nPressione ENTER para a proxima rodada");
    printf("\nPressione P para pular ate algum jogador ganhar\n");

    c = (char)getchar();
    if (c == '\n')
    {
        return 1;
    }
    else if (c == 'P' || c == 'p')
    {
        *modoPular = 1;
        descartarRestanteLinha();
        return 1;
    }
    else
    {
        descartarRestanteLinha();
        printf("Entrada invalida, tente novamente!\n");
        return aguardaEnter(modoPular);
    }
}

void limparTela(void)
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

int recebeNumeroJogadores(void)
{
    int qtdJogadores;
    int c;

    printf("Informe a quantidade de jogadores (2 a 5): ");

    if (scanf("%d", &qtdJogadores) != 1)
    {
        descartarRestanteLinha();
        printf("Entrada invalida! Digite um numero inteiro.\n");
        return recebeNumeroJogadores();
    }

    c = getchar();

    if (c != '\n' && c != EOF)
    {
        descartarRestanteLinha();
        printf("Digite apenas um numero, sem letras.\n");
        return recebeNumeroJogadores();
    }

    if (qtdJogadores < MIN_JOGADORES || qtdJogadores > MAX_JOGADORES)
    {
        printf("Digite um numero inteiro entre 2 e 5\n");
        return recebeNumeroJogadores();
    }
    return qtdJogadores;
}

int leNome(char nome[], int tamanho)
{
    while (1)
    {
        int tamanhoNome;

        printf("Digite o nome do jogador: ");

        if (fgets(nome, tamanho, stdin) == NULL)
        {
            printf("\nErro ao ler o nome.\n");
            exit(1);
        }

        tamanhoNome = (int)strlen(nome);

        if (tamanhoNome > 0 && nome[tamanhoNome - 1] == '\n')
        {
            nome[tamanhoNome - 1] = '\0';
        }
        else
        {
            int c = getchar();

            if (c != '\n' && c != EOF)
            {
                descartarRestanteLinha();
                printf("Nome muito longo. Tente novamente.\n");
                continue;
            }

            if (c == EOF && ferror(stdin))
            {
                exit(1);
            }
        }

        if (nome[0] == '\0')
        {
            printf("O nome nao pode ficar vazio.\n");
            continue;
        }

        return 1;
    }
}

int cadastraJogador(Jogador players[], int qtdJogadores)
{
    int i;

    for (i = 0; i < qtdJogadores; i++)
    {
        if (!leNome(players[i].nome, (int)sizeof(players[i].nome)))
        {
            return 0;
        }

        players[i].id = i + 1;
        geraCartela(&players[i].cartela);
        players[i].venceu = 0;
        players[i].rodadaVitoria = 0;
    }

    return 1;
}

void exibeCartela(Jogador jogador)
{
    int i;

    printf("\nCartela de Bingo - %s (ID: %d)\n", jogador.nome, jogador.id);

    for (i = 0; i < TAMANHO_CARTELA; i++)
    {
        if (i % CARTELA_COLUNAS == 0)
            printf("|----|----|----|----|----|\n");

        if (jogador.cartela.marcado[i])
        {
            printf("|%2d* ", jogador.cartela.numero[i]);
        }
        else
        {
            printf("|%2d  ", jogador.cartela.numero[i]);
        }

        if ((i + 1) % CARTELA_COLUNAS == 0)
            printf("|\n");
    }
    printf("|----|----|----|----|----|\n");
}

void exibeResumoRodada(int rodada, int numeroSorteado)
{
    printf("\n========== BINGO ==========\n");
    printf("Rodada: %d/60\n", rodada);
    printf("Numero sorteado: %d\n", numeroSorteado);
}

void exibeResultado(Jogador players[], int qtdJogadores, int jogadoresVencedores, int jogoEncerrado, int rodadaAtual)
{
    if (jogoEncerrado)
    {
        int posicao = 1;
        int rodada;
        int i;

        printf("\n========== RESULTADO FINAL - RANKING ==========\n");
        for (rodada = 1; rodada <= MAX_NUMERO_BINGO; rodada++)
        {
            for (i = 0; i < qtdJogadores; i++)
            {
                if (players[i].venceu && players[i].rodadaVitoria == rodada)
                {
                    printf("%d Lugar: %s (ID: %i) - vencendo na rodada %d\n", posicao, players[i].nome, players[i].id, players[i].rodadaVitoria);
                    posicao++;
                }
            }
        }
    }
    else
    {
        int venceramAnteriores = 0;
        int i;

        printf("\n========== STATUS PARCIAL DO JOGO ==========\n");
        printf("Quantos jogadores ja venceram: %d de %d\n", jogadoresVencedores, qtdJogadores);

        for (i = 0; i < qtdJogadores; i++)
        {
            if (players[i].venceu && players[i].rodadaVitoria < rodadaAtual)
            {
                venceramAnteriores = 1;
                break;
            }
        }

        if (venceramAnteriores)
        {
            printf("\nJa venceram em rodadas anteriores:\n");
            for (i = 0; i < qtdJogadores; i++)
            {
                if (players[i].venceu && players[i].rodadaVitoria < rodadaAtual)
                {
                    printf("   - %s (ID: %i) - Venceu na rodada %d\n", players[i].nome, players[i].id, players[i].rodadaVitoria);
                }
            }
        }
    }
}

int main(void)
{
    int qtsNumerosSorteados = 0;
    int numerosSorteados[MAX_NUMERO_BINGO];
    int jogadoresVencedores = 0;
    int qtdJogadores = recebeNumeroJogadores();
    int modoPular = 0;
    Jogador players[MAX_JOGADORES];
    int z;
    int i;

    srand((unsigned int)time(NULL));
    cadastraJogador(players, qtdJogadores);

    for (z = 1; z <= MAX_NUMERO_BINGO && jogadoresVencedores < qtdJogadores; z++)
    {
        int numeroSorteado;
        int novoVencedor = 0;

        armazenaNumeroSorteado(numerosSorteados, &qtsNumerosSorteados);
        numeroSorteado = numerosSorteados[z - 1];

        marcaNumero(players, qtdJogadores, numeroSorteado);
        limparTela();

        exibeResumoRodada(z, numeroSorteado);

        for (i = 0; i < qtdJogadores; i++)
        {
            if (players[i].venceu == 0 && verificaVitoria(&players[i], z))
            {
                printf("\nBINGO! %s venceu!!!\n", players[i].nome);
                jogadoresVencedores++;
                novoVencedor = 1;
            }
        }

        if (novoVencedor)
        {
            modoPular = 0;
        }

        if (jogadoresVencedores < qtdJogadores)
        {
            printf("\n--- Cartelas em jogo ---\n");

            for (i = 0; i < qtdJogadores; i++)
            {
                if (players[i].venceu == 0)
                {
                    exibeCartela(players[i]);
                }
            }

            exibeResultado(players, qtdJogadores, jogadoresVencedores, 0, z);

            if (z < MAX_NUMERO_BINGO)
            {
                aguardaEnter(&modoPular);
            }
        }
    }

    exibeResultado(players, qtdJogadores, jogadoresVencedores, 1, MAX_NUMERO_BINGO);

    return 0;
}
