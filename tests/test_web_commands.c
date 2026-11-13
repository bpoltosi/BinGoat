#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BINGOAT_CMD
#error "Compile este arquivo com -DBINGOAT_CMD=6 ou -DBINGOAT_CMD=7."
#endif

#define BINGOAT_WEB_API 1
#define BINGOAT_SEED 12345
#define BINGOAT_PLAYER_COUNT 2
#define BINGOAT_CALLED_COUNT 15
#define BINGOAT_BOARD_COLS 10
#define BINGOAT_AUTO_MARK 1
#define BINGOAT_TARGET_NUM 0
#define BINGOAT_TARGET_PLAYER 0
#define BINGOAT_EXISTING_PLAYERS 2
#define BINGOAT_RESET_CALLED 0

int BINGOAT_CALLED_INIT[60] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

char BINGOAT_PLAYER_NAMES[5][50] = {
    "Ana",
    "Bia"
};

int BINGOAT_PLAYER_WON[5] = {1};
int BINGOAT_PLAYER_WIN_ROUND[5] = {15};

int BINGOAT_PLAYER_CARDS[5][15] = {
    {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30}
};

int BINGOAT_PLAYER_MARKS[5][15] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

#define main bingoat_web_main
#include "../TrabalhoBingo.c"
#undef main

static const char *OUTPUT_PATH = "tests/.web-command-output.tmp";

static int output_contains(const char *output, const char *expected)
{
    if (strstr(output, expected) == NULL)
    {
        fprintf(stderr, "Esperado no JSON: %s\n", expected);
        return 0;
    }

    return 1;
}

int main(void)
{
    FILE *file = NULL;
    char *output = NULL;
    long output_length;
    size_t bytes_read;
    int success = 0;

    if (freopen(OUTPUT_PATH, "wb", stdout) == NULL)
    {
        fprintf(stderr, "Nao foi possivel redirecionar stdout do motor web.\n");
        goto cleanup;
    }

    (void)bingoat_web_main();

    if (fflush(stdout) != 0)
    {
        fprintf(stderr, "Falha ao descarregar a saida JSON do motor web.\n");
        goto cleanup;
    }

    if (fclose(stdout) != 0)
    {
        fprintf(stderr, "Falha ao fechar a saida JSON do motor web.\n");
        goto cleanup;
    }

    file = fopen(OUTPUT_PATH, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "Nao foi possivel ler a saida JSON do motor web.\n");
        goto cleanup;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fprintf(stderr, "Nao foi possivel posicionar a saida JSON.\n");
        goto cleanup;
    }

    output_length = ftell(file);
    if (output_length < 0 || fseek(file, 0, SEEK_SET) != 0)
    {
        fprintf(stderr, "Nao foi possivel medir a saida JSON.\n");
        goto cleanup;
    }

    output = (char *)malloc((size_t)output_length + 1);
    if (output == NULL)
    {
        fprintf(stderr, "Memoria insuficiente para validar a saida JSON.\n");
        goto cleanup;
    }

    bytes_read = fread(output, 1, (size_t)output_length, file);
    if (bytes_read != (size_t)output_length)
    {
        fprintf(stderr, "Leitura incompleta da saida JSON.\n");
        goto cleanup;
    }
    output[bytes_read] = '\0';

    if (!output_contains(output, "__BINGOAT_END__"))
    {
        goto cleanup;
    }

#if BINGOAT_CMD == 6
    if (!output_contains(output, "\"calledNumbers\":[1,2,3,4,5,6,7,8,9,10,11,12,13,14]") ||
        !output_contains(output, "\"markedNumbers\":[1,2,3,4,5,6,7,8,9,10,11,12,13,14]") ||
        !output_contains(output, "\"wonRound\":null"))
    {
        goto cleanup;
    }
    fprintf(stderr, "[OK] comando web 6: desfaz o ultimo sorteio e sincroniza marcacoes/vitoria.\n");
#elif BINGOAT_CMD == 7
    if (!output_contains(output, "\"calledNumbers\":[]") ||
        !output_contains(output, "\"markedNumbers\":[]") ||
        !output_contains(output, "\"numbers\":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15]") ||
        !output_contains(output, "\"wonRound\":null"))
    {
        goto cleanup;
    }
    fprintf(stderr, "[OK] comando web 7: limpa sorteios e marcacoes sem regenerar cartelas.\n");
#else
#error "Os testes web cobrem somente os comandos atuais de desfazer (6) e reiniciar (7)."
#endif

    success = 1;

cleanup:
    if (file != NULL)
    {
        fclose(file);
    }

    free(output);
    remove(OUTPUT_PATH);

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
