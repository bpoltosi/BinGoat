#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * Caracterizacao do programa de terminal sem alterar o codigo de producao:
 * - fixa o relogio para tornar o fluxo integral reproduzivel;
 * - desativa apenas a limpeza de tela para manter a saida capturavel;
 * - renomeia o main original para chamar o fluxo completo no teste final.
 */
#define time(value) ((time_t)0)
#define system(command) 0
#define main bingoat_cli_main
#include "../TrabalhoBingo.c"
#undef main
#undef system
#undef time

#define CHECK(condition)                                                        \
    do                                                                          \
    {                                                                           \
        if (!(condition))                                                       \
        {                                                                       \
            fprintf(stderr, "Falha em %s:%d: %s\n", __FILE__, __LINE__,        \
                    #condition);                                               \
            return 0;                                                           \
        }                                                                       \
    } while (0)

static const char *PLAYER_INPUT_PATH = "tests/.players-input.tmp";
static const char *CLI_INPUT_PATH = "tests/.cli-input.tmp";
static const char *CLI_OUTPUT_PATH = "tests/.cli-output.tmp";

static int check_card(const Cartela *cartela)
{
    int i;

    for (i = 0; i < TAMANHO_CARTELA; i++)
    {
        if (cartela->numero[i] < 1 || cartela->numero[i] > MAX_NUMERO_BINGO)
        {
            return 0;
        }

        if (cartela->marcado[i] != 0)
        {
            return 0;
        }

        if (i > 0)
        {
            if (cartela->numero[i] <= cartela->numero[i - 1])
            {
                return 0;
            }

            if (cartela->numero[i] - cartela->numero[i - 1] == 1)
            {
                return 0;
            }
        }
    }

    return 1;
}

static int read_player_count(const char *input)
{
    FILE *file = fopen(PLAYER_INPUT_PATH, "w");

    if (file == NULL)
    {
        return -1;
    }

    if (fputs(input, file) == EOF)
    {
        fclose(file);
        return -1;
    }

    if (fclose(file) != 0)
    {
        return -1;
    }

    if (freopen(PLAYER_INPUT_PATH, "r", stdin) == NULL)
    {
        return -1;
    }

    return recebeNumeroJogadores();
}

static int test_player_count_validation(void)
{
    int expected;
    char input[8];

    for (expected = MIN_JOGADORES; expected <= MAX_JOGADORES; expected++)
    {
        snprintf(input, sizeof(input), "%d\n", expected);
        CHECK(read_player_count(input) == expected);
    }

    CHECK(read_player_count("1\n2\n") == 2);
    CHECK(read_player_count("6\n5\n") == 5);
    CHECK(read_player_count("x\n2\n") == 2);
    CHECK(read_player_count("2x\n5\n") == 5);

    remove(PLAYER_INPUT_PATH);
    return 1;
}

static int test_card_generation(void)
{
    unsigned int seed;
    Cartela first;
    Cartela second;

    for (seed = 0; seed < 128; seed++)
    {
        Cartela cartela;
        srand(seed);
        geraCartela(&cartela);
        CHECK(check_card(&cartela));
    }

    srand(20261113U);
    geraCartela(&first);
    srand(20261113U);
    geraCartela(&second);

    CHECK(memcmp(first.numero, second.numero, sizeof(first.numero)) == 0);
    CHECK(memcmp(first.marcado, second.marcado, sizeof(first.marcado)) == 0);

    return 1;
}

static int collect_draws(unsigned int seed, int output[MAX_NUMERO_BINGO])
{
    int count = 0;
    int i;

    memset(output, 0, sizeof(int) * MAX_NUMERO_BINGO);
    srand(seed);

    for (i = 0; i < MAX_NUMERO_BINGO; i++)
    {
        armazenaNumeroSorteado(output, &count);
        if (count != i + 1)
        {
            return 0;
        }
    }

    return 1;
}

static int test_draws(void)
{
    int first[MAX_NUMERO_BINGO];
    int second[MAX_NUMERO_BINGO];
    int count = MAX_NUMERO_BINGO;
    int i;
    int j;

    CHECK(collect_draws(15015U, first));
    CHECK(collect_draws(15015U, second));
    CHECK(memcmp(first, second, sizeof(first)) == 0);

    for (i = 0; i < MAX_NUMERO_BINGO; i++)
    {
        CHECK(first[i] >= 1 && first[i] <= MAX_NUMERO_BINGO);

        for (j = i + 1; j < MAX_NUMERO_BINGO; j++)
        {
            CHECK(first[i] != first[j]);
        }
    }

    CHECK(sortearNumero(first, MAX_NUMERO_BINGO) == 0);
    armazenaNumeroSorteado(first, &count);
    CHECK(count == MAX_NUMERO_BINGO);

    return 1;
}

static int test_marking_and_winners(void)
{
    Jogador players[2];
    int i;

    memset(players, 0, sizeof(players));

    for (i = 0; i < TAMANHO_CARTELA; i++)
    {
        players[0].cartela.numero[i] = i + 1;
        players[1].cartela.numero[i] = i + 1;
    }

    players[0].venceu = 1;
    marcaNumero(players, 2, 7);

    CHECK(players[0].cartela.marcado[6] == 0);
    CHECK(players[1].cartela.marcado[6] == 1);
    CHECK(players[1].cartela.marcado[5] == 0);

    for (i = 0; i < TAMANHO_CARTELA; i++)
    {
        players[1].cartela.marcado[i] = 1;
    }
    players[1].cartela.marcado[TAMANHO_CARTELA - 1] = 0;
    players[1].venceu = 0;
    players[1].rodadaVitoria = 0;

    CHECK(verificaVitoria(&players[1], 12) == 0);
    CHECK(players[1].venceu == 0);
    CHECK(players[1].rodadaVitoria == 0);

    players[1].cartela.marcado[TAMANHO_CARTELA - 1] = 1;
    CHECK(verificaVitoria(&players[1], 12) == 1);
    CHECK(players[1].venceu == 1);
    CHECK(players[1].rodadaVitoria == 12);

    CHECK(verificaVitoria(&players[1], 13) == 1);
    CHECK(players[1].rodadaVitoria == 12);

    players[1].venceu = 0;
    players[1].rodadaVitoria = 0;
    CHECK(verificaVitoria(&players[1], 0) == 1);
    CHECK(players[1].rodadaVitoria == 1);

    return 1;
}

static int test_all_supported_player_counts_can_win(void)
{
    int player_count;

    for (player_count = MIN_JOGADORES; player_count <= MAX_JOGADORES; player_count++)
    {
        Jogador players[MAX_JOGADORES];
        int winners = 0;
        int i;
        int j;

        memset(players, 0, sizeof(players));

        for (i = 0; i < player_count; i++)
        {
            for (j = 0; j < TAMANHO_CARTELA; j++)
            {
                players[i].cartela.marcado[j] = 1;
            }

            if (verificaVitoria(&players[i], i + 1))
            {
                winners++;
            }
        }

        CHECK(winners == player_count);
    }

    return 1;
}

static int test_line_detection(void)
{
    int values[TAMANHO_CARTELA];
    int marked[TAMANHO_CARTELA] = {0};
    LinhaCompleta lines[32];
    int count;
    int i;

    for (i = 0; i < TAMANHO_CARTELA; i++)
    {
        values[i] = i + 1;
    }

    /* Linha horizontal. */
    for (i = 0; i < 5; i++)
    {
        marked[i] = 1;
    }
    count = detectarLinhasGrade(values, marked, TAMANHO_CARTELA, 3, 5, lines, 32);
    CHECK(count == 1);
    CHECK(lines[0].orientacao == 0);
    CHECK(lines[0].indiceA == 0);
    CHECK(lines[0].tamanho == 5);

    /* Coluna vertical. */
    memset(marked, 0, sizeof(marked));
    marked[2] = 1;
    marked[7] = 1;
    marked[12] = 1;
    count = detectarLinhasGrade(values, marked, TAMANHO_CARTELA, 3, 5, lines, 32);
    CHECK(count == 1);
    CHECK(lines[0].orientacao == 1);
    CHECK(lines[0].indiceA == 2);
    CHECK(lines[0].tamanho == 3);

    /* Diagonal principal. */
    memset(marked, 0, sizeof(marked));
    marked[0] = 1;
    marked[6] = 1;
    marked[12] = 1;
    count = detectarLinhasGrade(values, marked, TAMANHO_CARTELA, 3, 5, lines, 32);
    CHECK(count == 1);
    CHECK(lines[0].orientacao == 2);
    CHECK(lines[0].tamanho == 3);

    /* Diagonal secundaria. */
    memset(marked, 0, sizeof(marked));
    marked[12] = 1;
    marked[8] = 1;
    marked[4] = 1;
    count = detectarLinhasGrade(values, marked, TAMANHO_CARTELA, 3, 5, lines, 32);
    CHECK(count == 1);
    CHECK(lines[0].orientacao == 3);
    CHECK(lines[0].tamanho == 3);

    return 1;
}

static size_t count_occurrences(const char *text, const char *needle)
{
    size_t count = 0;
    size_t needle_length = strlen(needle);
    const char *cursor = text;

    while ((cursor = strstr(cursor, needle)) != NULL)
    {
        count++;
        cursor += needle_length;
    }

    return count;
}

static int test_terminal_game_ends(void)
{
    FILE *input = NULL;
    FILE *output_file = NULL;
    char *output = NULL;
    long output_length;
    size_t bytes_read;
    size_t rounds;
    int i;
    int status;
    int stdin_redirected = 0;
    int stdout_redirected = 0;
    int success = 0;

    input = fopen(CLI_INPUT_PATH, "w");
    if (input == NULL)
    {
        fprintf(stderr, "Nao foi possivel criar a entrada do teste de terminal.\n");
        goto cleanup;
    }

    if (fputs("2\nAlice\nBob\n", input) == EOF)
    {
        fprintf(stderr, "Nao foi possivel gravar a entrada do teste de terminal.\n");
        goto cleanup;
    }

    for (i = 0; i < MAX_NUMERO_BINGO; i++)
    {
        if (fputs("p\n", input) == EOF)
        {
            fprintf(stderr, "Nao foi possivel gravar os comandos de avancar rodada.\n");
            goto cleanup;
        }
    }

    if (fclose(input) != 0)
    {
        input = NULL;
        fprintf(stderr, "Nao foi possivel fechar a entrada do teste de terminal.\n");
        goto cleanup;
    }
    input = NULL;

    if (freopen(CLI_INPUT_PATH, "r", stdin) == NULL)
    {
        fprintf(stderr, "Nao foi possivel redirecionar stdin no teste de terminal.\n");
        goto cleanup;
    }
    stdin_redirected = 1;

    if (freopen(CLI_OUTPUT_PATH, "w", stdout) == NULL)
    {
        fprintf(stderr, "Nao foi possivel redirecionar stdout no teste de terminal.\n");
        goto cleanup;
    }
    stdout_redirected = 1;

    status = bingoat_cli_main();

    if (fflush(stdout) != 0)
    {
        fprintf(stderr, "Falha ao descarregar a saida do jogo de terminal.\n");
        goto cleanup;
    }

    if (fclose(stdout) != 0)
    {
        stdout_redirected = 0;
        fprintf(stderr, "Falha ao fechar a saida do jogo de terminal.\n");
        goto cleanup;
    }
    stdout_redirected = 0;

    if (fclose(stdin) != 0)
    {
        stdin_redirected = 0;
        fprintf(stderr, "Falha ao fechar a entrada do jogo de terminal.\n");
        goto cleanup;
    }
    stdin_redirected = 0;

    if (status != 0)
    {
        fprintf(stderr, "O programa de terminal retornou status diferente de zero.\n");
        goto cleanup;
    }

    output_file = fopen(CLI_OUTPUT_PATH, "r");
    if (output_file == NULL)
    {
        fprintf(stderr, "Nao foi possivel ler a saida do jogo de terminal.\n");
        goto cleanup;
    }

    if (fseek(output_file, 0, SEEK_END) != 0)
    {
        fprintf(stderr, "Nao foi possivel posicionar a saida do jogo de terminal.\n");
        goto cleanup;
    }

    output_length = ftell(output_file);
    if (output_length < 0 || fseek(output_file, 0, SEEK_SET) != 0)
    {
        fprintf(stderr, "Nao foi possivel medir a saida do jogo de terminal.\n");
        goto cleanup;
    }

    output = (char *)malloc((size_t)output_length + 1);
    if (output == NULL)
    {
        fprintf(stderr, "Memoria insuficiente para validar a saida do jogo.\n");
        goto cleanup;
    }

    bytes_read = fread(output, 1, (size_t)output_length, output_file);
    if (bytes_read != (size_t)output_length)
    {
        fprintf(stderr, "Leitura incompleta da saida do jogo de terminal.\n");
        goto cleanup;
    }
    output[bytes_read] = '\0';

    if (strstr(output, "BINGO! Alice venceu!!!") == NULL ||
        strstr(output, "BINGO! Bob venceu!!!") == NULL ||
        strstr(output, "RESULTADO FINAL - RANKING") == NULL)
    {
        fprintf(stderr, "O fluxo nao registrou os dois vencedores e o ranking final.\n");
        goto cleanup;
    }

    rounds = count_occurrences(output, "Rodada: ");
    if (rounds == 0 || rounds > MAX_NUMERO_BINGO)
    {
        fprintf(stderr, "O jogo excedeu ou nao executou o limite esperado de rodadas.\n");
        goto cleanup;
    }

    success = 1;

cleanup:
    if (input != NULL)
    {
        fclose(input);
    }
    if (stdout_redirected)
    {
        fflush(stdout);
        fclose(stdout);
    }
    if (stdin_redirected)
    {
        fclose(stdin);
    }
    if (output_file != NULL)
    {
        fclose(output_file);
    }

    free(output);
    remove(CLI_INPUT_PATH);
    remove(CLI_OUTPUT_PATH);

    return success;
}

static int run_test(const char *name, int (*test)(void))
{
    if (!test())
    {
        fprintf(stderr, "[FALHOU] %s\n", name);
        return 0;
    }

    fprintf(stderr, "[OK] %s\n", name);
    return 1;
}

int main(void)
{
    int success = 1;

    success &= run_test("validacao da quantidade de jogadores (2 a 5)", test_player_count_validation);
    success &= run_test("geracao e reprodutibilidade das cartelas", test_card_generation);
    success &= run_test("sorteios unicos, limitados e reproduziveis", test_draws);
    success &= run_test("marcacoes e identificacao de vencedores", test_marking_and_winners);
    success &= run_test("vitoria para partidas com 2 a 5 jogadores", test_all_supported_player_counts_can_win);
    success &= run_test("deteccao de linhas horizontais, verticais e diagonais", test_line_detection);
    success &= run_test("encerramento do fluxo real de terminal", test_terminal_game_ends);

    remove(PLAYER_INPUT_PATH);

    if (!success)
    {
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Todos os testes de caracterizacao do motor C passaram.\n");
    return EXIT_SUCCESS;
}
