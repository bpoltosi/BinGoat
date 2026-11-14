#include "http_json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expression) \
    do { \
        if (!(expression)) { \
            fprintf(stderr, "Falha: %s (linha %d)\\n", #expression, __LINE__); \
            return 1; \
        } \
    } while (0)

static int test_valid_json(void)
{
    static const char body[] = " { \"players\": [\"Ana\", \"Bia\"], \"round\": 1 } \n";
    cJSON *json = NULL;

    CHECK(bingo_http_parse_json(body, strlen(body), &json) == BINGO_HTTP_JSON_OK);
    CHECK(json != NULL);
    CHECK(cJSON_IsObject(json));
    CHECK(cJSON_IsArray(cJSON_GetObjectItemCaseSensitive(json, "players")));
    cJSON_Delete(json);
    return 0;
}

static int test_invalid_json(void)
{
    static const char body[] = "{\"players\":[}";
    cJSON *json = NULL;

    CHECK(bingo_http_parse_json(body, strlen(body), &json) == BINGO_HTTP_JSON_INVALID_JSON);
    CHECK(json == NULL);
    return 0;
}

static int test_trailing_data(void)
{
    static const char body[] = "{\"ok\":true} garbage";
    cJSON *json = NULL;

    CHECK(bingo_http_parse_json(body, strlen(body), &json) == BINGO_HTTP_JSON_TRAILING_DATA);
    CHECK(json == NULL);
    return 0;
}

static int test_empty_and_oversized(void)
{
    cJSON *json = NULL;
    char *oversized = (char *)malloc(BINGO_HTTP_MAX_JSON_BODY + 1U);

    CHECK(oversized != NULL);
    memset(oversized, ' ', BINGO_HTTP_MAX_JSON_BODY + 1U);
    CHECK(bingo_http_parse_json("", 0, &json) == BINGO_HTTP_JSON_EMPTY_BODY);
    CHECK(bingo_http_parse_json(oversized, BINGO_HTTP_MAX_JSON_BODY + 1U, &json) ==
          BINGO_HTTP_JSON_BODY_TOO_LARGE);
    CHECK(json == NULL);
    free(oversized);
    return 0;
}

int main(void)
{
    if (test_valid_json() || test_invalid_json() || test_trailing_data() ||
        test_empty_and_oversized()) {
        return EXIT_FAILURE;
    }
    puts("Testes do parser JSON HTTP passaram.");
    return EXIT_SUCCESS;
}
