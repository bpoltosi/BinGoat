#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAX_NUMERO_BINGO 60
#define TAMANHO_CARTELA 15
#define MIN_JOGADORES 2
#define MAX_JOGADORES 5
#define CARTELA_LINHAS 3
#define CARTELA_COLUNAS 5
#define MAX_LINHA_NUMEROS 15
#define MAX_LINHAS_ATIVAS 48

typedef struct
{
    int numero[TAMANHO_CARTELA];
    int marcado[TAMANHO_CARTELA]; /* 1 para sorteado ; 0 para disponivel */
} Cartela;

typedef struct
{
    int id;
    char nome[50];
    Cartela cartela;
    int venceu; /* 0 se ainda esta jogando, 1 se ja ganhou */
    int rodadaVitoria;
} Jogador;

typedef struct
{
    int orientacao; /* 0 = horizontal, 1 = vertical, 2 = diagonal */
    int indiceA;
    int indiceB;
    int tamanho;
    int numeros[MAX_LINHA_NUMEROS];
} LinhaCompleta;

int sortearNumero(int *numerosSorteados, int qtsNumerosSorteados)
{
    int n;
    int repetido;
    int i;

    if (qtsNumerosSorteados >= MAX_NUMERO_BINGO)
    {
        return 0;
    }

    n = rand() % MAX_NUMERO_BINGO + 1;
    repetido = 1;

    while (repetido)
    {
        repetido = 0;

        for (i = 0; i < qtsNumerosSorteados; i++)
        {
            if (n == numerosSorteados[i])
            {
                repetido = 1;
                n = rand() % MAX_NUMERO_BINGO + 1;
                break;
            }
        }
    }
    return n;
}

void armazenaNumeroSorteado(int *numerosSorteados, int *qtsNumerosSorteados)
{
    int n;

    if (*qtsNumerosSorteados >= MAX_NUMERO_BINGO)
    {
        return;
    }

    n = sortearNumero(numerosSorteados, *qtsNumerosSorteados);
    if (n == 0)
    {
        return;
    }

    numerosSorteados[*qtsNumerosSorteados] = n;
    (*qtsNumerosSorteados)++;
}

void ordenarCartela(Cartela *cartela)
{
    int i;
    int j;
    int tmpNum;
    int tmpMarc;

    for (i = 0; i < TAMANHO_CARTELA - 1; i++)
    {
        for (j = i + 1; j < TAMANHO_CARTELA; j++)
        {
            if (cartela->numero[j] < cartela->numero[i])
            {
                tmpNum = cartela->numero[i];
                cartela->numero[i] = cartela->numero[j];
                cartela->numero[j] = tmpNum;

                tmpMarc = cartela->marcado[i];
                cartela->marcado[i] = cartela->marcado[j];
                cartela->marcado[j] = tmpMarc;
            }
        }
    }
}

void geraCartela(Cartela *cartela)
{
    int z;
    int i;
    int repetido;
    int n;

    for (z = 0; z < TAMANHO_CARTELA; z++)
    {
        repetido = 1;
        n = rand() % MAX_NUMERO_BINGO + 1;

        while (repetido)
        {
            repetido = 0;

            for (i = 0; i < z; i++)
            {
                if (n == cartela->numero[i] ||
                    n == (cartela->numero[i] - 1) ||
                    n == (cartela->numero[i] + 1))
                {
                    repetido = 1;
                    n = rand() % MAX_NUMERO_BINGO + 1;
                    break;
                }
            }
        }
        cartela->numero[z] = n;
        cartela->marcado[z] = 0;
    }

    ordenarCartela(cartela);
}

void marcaNumero(Jogador *players, int qtdJogadores, int numeroSorteado)
{
    int i;
    int j;

    for (i = 0; i < qtdJogadores; i++)
    {
        if (players[i].venceu == 1)
        {
            continue;
        }

        for (j = 0; j < TAMANHO_CARTELA; j++)
        {
            if (players[i].cartela.numero[j] == numeroSorteado)
            {
                players[i].cartela.marcado[j] = 1;
            }
        }
    }
}

int verificaVitoria(Jogador *jogador, int rodada)
{
    int i;

    for (i = 0; i < TAMANHO_CARTELA; i++)
    {
        if (jogador->cartela.marcado[i] == 0)
        {
            return 0;
        }
    }
    if (jogador->venceu == 0)
    {
        jogador->venceu = 1;
        jogador->rodadaVitoria = rodada > 0 ? rodada : 1;
    }
    return 1;
}

int estaNoConjunto(int valor, int *lista, int tamanho)
{
    int i;
    for (i = 0; i < tamanho; i++)
    {
        if (lista[i] == valor)
        {
            return 1;
        }
    }
    return 0;
}

int detectarLinhasGrade(
    int *valores,
    int *marcados,
    int totalValores,
    int linhas,
    int colunas,
    LinhaCompleta *saida,
    int maxSaida
)
{
    int count = 0;
    int r;
    int c;
    int k;
    int idx;
    int completa;
    int diagLen;

    /* Horizontais */
    for (r = 0; r < linhas && count < maxSaida; r++)
    {
        completa = 1;
        for (c = 0; c < colunas; c++)
        {
            idx = r * colunas + c;
            if (idx >= totalValores || !marcados[idx])
            {
                completa = 0;
                break;
            }
            saida[count].numeros[c] = valores[idx];
        }
        if (completa)
        {
            saida[count].orientacao = 0;
            saida[count].indiceA = r;
            saida[count].indiceB = 0;
            saida[count].tamanho = colunas;
            count++;
        }
    }

    /* Verticais */
    for (c = 0; c < colunas && count < maxSaida; c++)
    {
        completa = 1;
        for (r = 0; r < linhas; r++)
        {
            idx = r * colunas + c;
            if (idx >= totalValores || !marcados[idx])
            {
                completa = 0;
                break;
            }
            saida[count].numeros[r] = valores[idx];
        }
        if (completa)
        {
            saida[count].orientacao = 1;
            saida[count].indiceA = c;
            saida[count].indiceB = 0;
            saida[count].tamanho = linhas;
            count++;
        }
    }

    /* Diagonais de comprimento min(linhas, colunas) */
    diagLen = linhas < colunas ? linhas : colunas;
    if (diagLen >= 2)
    {
        /* Diagonal principal (\) */
        for (r = 0; r <= linhas - diagLen && count < maxSaida; r++)
        {
            for (c = 0; c <= colunas - diagLen && count < maxSaida; c++)
            {
                completa = 1;
                for (k = 0; k < diagLen; k++)
                {
                    idx = (r + k) * colunas + (c + k);
                    if (idx >= totalValores || !marcados[idx])
                    {
                        completa = 0;
                        break;
                    }
                    saida[count].numeros[k] = valores[idx];
                }
                if (completa)
                {
                    saida[count].orientacao = 2;
                    saida[count].indiceA = r;
                    saida[count].indiceB = c;
                    saida[count].tamanho = diagLen;
                    count++;
                }
            }
        }

        /* Diagonal secundaria (/) */
        for (r = diagLen - 1; r < linhas && count < maxSaida; r++)
        {
            for (c = 0; c <= colunas - diagLen && count < maxSaida; c++)
            {
                completa = 1;
                for (k = 0; k < diagLen; k++)
                {
                    idx = (r - k) * colunas + (c + k);
                    if (idx >= totalValores || !marcados[idx])
                    {
                        completa = 0;
                        break;
                    }
                    saida[count].numeros[k] = valores[idx];
                }
                if (completa)
                {
                    saida[count].orientacao = 3;
                    saida[count].indiceA = r;
                    saida[count].indiceB = c;
                    saida[count].tamanho = diagLen;
                    count++;
                }
            }
        }
    }

    return count;
}

#ifdef BINGOAT_WEB_API

/*
 * Modo Web API: executado sob demanda a partir de TrabalhoBingo.c.
 * Todas as regras (geracao de cartelas, sorteio, marcacao, linhas e vitoria)
 * sao processadas integralmente em C e serializadas em JSON para stdout.
 */

void imprimeLinhasJson(LinhaCompleta *linhas, int qtd, char *prefixo)
{
    int i;
    int j;
    char *orientNome;

    printf("[");
    for (i = 0; i < qtd; i++)
    {
        if (i > 0) printf(",");

        if (linhas[i].orientacao == 0) orientNome = "horizontal";
        else if (linhas[i].orientacao == 1) orientNome = "vertical";
        else orientNome = "diagonal";

        printf("{\"key\":\"%s:%d:%d:%d\",", prefixo, linhas[i].orientacao, linhas[i].indiceA, linhas[i].indiceB);
        printf("\"orientation\":\"%s\",", orientNome);

        if (linhas[i].orientacao == 0)
        {
            printf("\"label\":\"Linha %d\",", linhas[i].indiceA + 1);
        }
        else if (linhas[i].orientacao == 1)
        {
            printf("\"label\":\"Coluna %d\",", linhas[i].indiceA + 1);
        }
        else
        {
            int primeiro = linhas[i].numeros[0];
            int ultimo = linhas[i].numeros[linhas[i].tamanho - 1];
            printf("\"label\":\"Diagonal (%02d-%02d)\",", primeiro, ultimo);
        }

        printf("\"numbers\":[");
        for (j = 0; j < linhas[i].tamanho; j++)
        {
            if (j > 0) printf(",");
            printf("%d", linhas[i].numeros[j]);
        }
        printf("]}");
    }
    printf("]");
}

void sincronizarMarcacoesAutomaticas(
    Jogador *players,
    int qtdJogadores,
    int *numerosSorteados,
    int qtsNumerosSorteados
)
{
    int i;
    int j;

    for (i = 0; i < qtdJogadores; i++)
    {
        for (j = 0; j < TAMANHO_CARTELA; j++)
        {
            if (estaNoConjunto(players[i].cartela.numero[j], numerosSorteados, qtsNumerosSorteados))
            {
                players[i].cartela.marcado[j] = 1;
            }
            else
            {
                players[i].cartela.marcado[j] = 0;
            }
        }
    }
}

void atualizarVencedores(Jogador *players, int qtdJogadores, int rodadaAtual)
{
    int i;
    int j;
    int todosMarcados;

    for (i = 0; i < qtdJogadores; i++)
    {
        todosMarcados = 1;
        for (j = 0; j < TAMANHO_CARTELA; j++)
        {
            if (players[i].cartela.marcado[j] == 0)
            {
                todosMarcados = 0;
                break;
            }
        }

        if (todosMarcados)
        {
            if (players[i].venceu == 0)
            {
                players[i].venceu = 1;
                players[i].rodadaVitoria = rodadaAtual > 0 ? rodadaAtual : 1;
            }
        }
        else
        {
            players[i].venceu = 0;
            players[i].rodadaVitoria = 0;
        }
    }
}

Jogador g_players[MAX_JOGADORES];
int g_numerosSorteados[MAX_NUMERO_BINGO];
int g_boardValues[MAX_NUMERO_BINGO];
int g_boardMarked[MAX_NUMERO_BINGO];
LinhaCompleta g_boardLines[MAX_LINHAS_ATIVAS];
LinhaCompleta g_cardLines[32];

int main(void)
{
    int qtsNumerosSorteados = BINGOAT_CALLED_COUNT;
    int qtdJogadores = BINGOAT_PLAYER_COUNT;
    int drawnNumber = 0;
    int boardLineCount = 0;
    int boardRows;
    int boardCols = BINGOAT_BOARD_COLS;
    int i;
    int j;

    srand((unsigned int)time(NULL) + (unsigned int)BINGOAT_SEED);

    if (qtdJogadores < MIN_JOGADORES) qtdJogadores = MIN_JOGADORES;
    if (qtdJogadores > MAX_JOGADORES) qtdJogadores = MAX_JOGADORES;
    if (boardCols < 1) boardCols = 10;
    if (qtsNumerosSorteados < 0) qtsNumerosSorteados = 0;
    if (qtsNumerosSorteados > MAX_NUMERO_BINGO) qtsNumerosSorteados = MAX_NUMERO_BINGO;

    for (i = 0; i < qtsNumerosSorteados; i++)
    {
        g_numerosSorteados[i] = BINGOAT_CALLED_INIT[i];
    }

    for (i = 0; i < qtdJogadores; i++)
    {
        g_players[i].id = i + 1;
        strncpy(g_players[i].nome, BINGOAT_PLAYER_NAMES[i], 49);
        g_players[i].nome[49] = '\0';
        g_players[i].venceu = BINGOAT_PLAYER_WON[i];
        g_players[i].rodadaVitoria = BINGOAT_PLAYER_WIN_ROUND[i];

        if (BINGOAT_CMD == 1 || i >= BINGOAT_EXISTING_PLAYERS)
        {
            geraCartela(&g_players[i].cartela);
            g_players[i].venceu = 0;
            g_players[i].rodadaVitoria = 0;
        }
        else
        {
            for (j = 0; j < TAMANHO_CARTELA; j++)
            {
                g_players[i].cartela.numero[j] = BINGOAT_PLAYER_CARDS[i][j];
                g_players[i].cartela.marcado[j] = BINGOAT_PLAYER_MARKS[i][j];
            }
        }
    }

    /* CMD 1 = criar/novas cartelas, CMD 2 = sortear, CMD 3 = alternar tabuleiro,
       CMD 4 = alternar celula de cartela, CMD 5 = sincronizar,
       CMD 6 = desfazer ultimo sorteio, CMD 7 = limpar marcacoes mantendo cartelas */
    if (BINGOAT_CMD == 1 && BINGOAT_RESET_CALLED)
    {
        qtsNumerosSorteados = 0;
    }
    else if (BINGOAT_CMD == 2)
    {
        if (qtsNumerosSorteados < MAX_NUMERO_BINGO)
        {
            armazenaNumeroSorteado(g_numerosSorteados, &qtsNumerosSorteados);
            drawnNumber = g_numerosSorteados[qtsNumerosSorteados - 1];
            if (BINGOAT_AUTO_MARK)
            {
                marcaNumero(g_players, qtdJogadores, drawnNumber);
            }
        }
    }
    else if (BINGOAT_CMD == 3)
    {
        int alvo = BINGOAT_TARGET_NUM;
        int pos = -1;
        for (i = 0; i < qtsNumerosSorteados; i++)
        {
            if (g_numerosSorteados[i] == alvo)
            {
                pos = i;
                break;
            }
        }
        if (pos >= 0)
        {
            for (i = pos; i < qtsNumerosSorteados - 1; i++)
            {
                g_numerosSorteados[i] = g_numerosSorteados[i + 1];
            }
            qtsNumerosSorteados--;
        }
        else if (alvo >= 1 && alvo <= MAX_NUMERO_BINGO && qtsNumerosSorteados < MAX_NUMERO_BINGO)
        {
            g_numerosSorteados[qtsNumerosSorteados] = alvo;
            qtsNumerosSorteados++;
        }
    }
    else if (BINGOAT_CMD == 4)
    {
        int pIdx = BINGOAT_TARGET_PLAYER - 1;
        int alvo = BINGOAT_TARGET_NUM;
        if (pIdx >= 0 && pIdx < qtdJogadores)
        {
            for (j = 0; j < TAMANHO_CARTELA; j++)
            {
                if (g_players[pIdx].cartela.numero[j] == alvo)
                {
                    g_players[pIdx].cartela.marcado[j] = g_players[pIdx].cartela.marcado[j] ? 0 : 1;
                }
            }
        }
    }
    else if (BINGOAT_CMD == 6)
    {
        if (qtsNumerosSorteados > 0)
        {
            qtsNumerosSorteados--;
        }
    }
    else if (BINGOAT_CMD == 7)
    {
        qtsNumerosSorteados = 0;
        for (i = 0; i < qtdJogadores; i++)
        {
            for (j = 0; j < TAMANHO_CARTELA; j++)
            {
                g_players[i].cartela.marcado[j] = 0;
            }
            g_players[i].venceu = 0;
            g_players[i].rodadaVitoria = 0;
        }
    }

    if (BINGOAT_AUTO_MARK && BINGOAT_CMD != 4)
    {
        sincronizarMarcacoesAutomaticas(g_players, qtdJogadores, g_numerosSorteados, qtsNumerosSorteados);
    }

    atualizarVencedores(g_players, qtdJogadores, qtsNumerosSorteados);

    for (i = 0; i < MAX_NUMERO_BINGO; i++)
    {
        g_boardValues[i] = i + 1;
        g_boardMarked[i] = estaNoConjunto(i + 1, g_numerosSorteados, qtsNumerosSorteados);
    }

    boardRows = (MAX_NUMERO_BINGO + boardCols - 1) / boardCols;
    boardLineCount = detectarLinhasGrade(
        g_boardValues,
        g_boardMarked,
        MAX_NUMERO_BINGO,
        boardRows,
        boardCols,
        g_boardLines,
        MAX_LINHAS_ATIVAS
    );

    printf("{");
    printf("\"drawnNumber\":");
    if (drawnNumber > 0) printf("%d,", drawnNumber);
    else printf("null,");

    printf("\"autoMark\":%s,", BINGOAT_AUTO_MARK ? "true" : "false");
    printf("\"calledNumbers\":[");
    for (i = 0; i < qtsNumerosSorteados; i++)
    {
        if (i > 0) printf(",");
        printf("%d", g_numerosSorteados[i]);
    }
    printf("],");

    printf("\"boardLines\":");
    imprimeLinhasJson(g_boardLines, boardLineCount, "board");
    printf(",");

    printf("\"players\":[");
    for (i = 0; i < qtdJogadores; i++)
    {
        int cardLineCount = detectarLinhasGrade(
            g_players[i].cartela.numero,
            g_players[i].cartela.marcado,
            TAMANHO_CARTELA,
            CARTELA_LINHAS,
            CARTELA_COLUNAS,
            g_cardLines,
            32
        );
        char prefix[24];
        sprintf(prefix, "card-%d", g_players[i].id);

        if (i > 0) printf(",");
        printf("{");
        printf("\"id\":%d,", g_players[i].id);
        printf("\"name\":\"%s\",", g_players[i].nome);

        printf("\"numbers\":[");
        for (j = 0; j < TAMANHO_CARTELA; j++)
        {
            if (j > 0) printf(",");
            printf("%d", g_players[i].cartela.numero[j]);
        }
        printf("],");

        printf("\"markedNumbers\":[");
        {
            int firstMarked = 1;
            for (j = 0; j < TAMANHO_CARTELA; j++)
            {
                if (g_players[i].cartela.marcado[j])
                {
                    if (!firstMarked) printf(",");
                    printf("%d", g_players[i].cartela.numero[j]);
                    firstMarked = 0;
                }
            }
        }
        printf("],");

        printf("\"wonRound\":");
        if (g_players[i].venceu) printf("%d,", g_players[i].rodadaVitoria);
        else printf("null,");

        printf("\"lines\":");
        imprimeLinhasJson(g_cardLines, cardLineCount, prefix);
        printf("}");
    }
    printf("]");
    printf("}\n");
    printf("__BINGOAT_END__\n");

    return 0;
}

#else

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

#endif
