#include "http_json.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_valid_json(void)
{
    static const char body[] = " { \"players\": [\"Ana\", \"Bia\"], \"round\": 1 } \n";
    cJSON *json = NULL;

    assert(bingo_http_parse_json(body, strlen(body), &json) == BINGO_HTTP_JSON_OK);
    assert(json != NULL);
    assert(cJSON_IsObject(json));
    assert(cJSON_IsArray(cJSON_GetObjectItemCaseSensitive(json, "players")));
    cJSON_Delete(json);
}

static void test_invalid_json(void)
{
    static const char body[] = "{\"players\":[}";
    cJSON *json = NULL;

    assert(bingo_http_parse_json(body, strlen(body), &json) == BINGO_HTTP_JSON_INVALID_JSON);
    assert(json == NULL);
}

static void test_trailing_data(void)
{
    static const char body[] = "{\"ok\":true} garbage";
    cJSON *json = NULL;

    assert(bingo_http_parse_json(body, strlen(body), &json) == BINGO_HTTP_JSON_TRAILING_DATA);
    assert(json == NULL);
}

static void test_empty_and_oversized(void)
{
    cJSON *json = NULL;
    char oversized[BINGO_HTTP_MAX_JSON_BODY + 1];

    assert(bingo_http_parse_json("", 0, &json) == BINGO_HTTP_JSON_EMPTY_BODY);
    memset(oversized, ' ', sizeof(oversized));
    assert(bingo_http_parse_json(oversized, sizeof(oversized), &json) == BINGO_HTTP_JSON_BODY_TOO_LARGE);
    assert(json == NULL);
}

int main(void)
{
    test_valid_json();
    test_invalid_json();
    test_trailing_data();
    test_empty_and_oversized();
    puts("Testes do parser JSON HTTP passaram.");
    return 0;
}
