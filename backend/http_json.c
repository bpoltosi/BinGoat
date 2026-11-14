#include "http_json.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static int equals_ignore_case(const char *left, const char *right)
{
    while (*left != '\0' && *right != '\0') {
        if (tolower((unsigned char)*left) != tolower((unsigned char)*right)) return 0;
        left++;
        right++;
    }
    return *left == '\0' && *right == '\0';
}

static int is_json_content_type(const char *value)
{
    static const char expected[] = "application/json";
    size_t i = 0;

    if (value == NULL) return 0;
    while (*value == ' ' || *value == '\t') value++;

    while (expected[i] != '\0') {
        if (value[i] == '\0' ||
            tolower((unsigned char)value[i]) != expected[i]) {
            return 0;
        }
        i++;
    }

    while (value[i] == ' ' || value[i] == '\t') i++;
    return value[i] == '\0' || value[i] == ';';
}

BingoHttpJsonResult bingo_http_parse_json(
    const char *body,
    size_t body_length,
    cJSON **json
)
{
    const char *parse_end = NULL;
    const char *body_end;
    char *terminated_body;
    cJSON *parsed;

    if (json == NULL || body == NULL) return BINGO_HTTP_JSON_INVALID_ARGUMENT;
    *json = NULL;

    if (body_length == 0) return BINGO_HTTP_JSON_EMPTY_BODY;
    if (body_length > BINGO_HTTP_MAX_JSON_BODY) return BINGO_HTTP_JSON_BODY_TOO_LARGE;

    terminated_body = (char *)malloc(body_length + 1U);
    if (terminated_body == NULL) return BINGO_HTTP_JSON_READ_ERROR;
    memcpy(terminated_body, body, body_length);
    terminated_body[body_length] = '\0';

    parsed = cJSON_ParseWithOpts(terminated_body, &parse_end, 0);
    if (parsed == NULL || parse_end == NULL) {
        cJSON_Delete(parsed);
        free(terminated_body);
        return BINGO_HTTP_JSON_INVALID_JSON;
    }

    body_end = terminated_body + body_length;
    while (parse_end < body_end && isspace((unsigned char)*parse_end)) {
        parse_end++;
    }
    if (parse_end != body_end) {
        cJSON_Delete(parsed);
        free(terminated_body);
        return BINGO_HTTP_JSON_TRAILING_DATA;
    }

    free(terminated_body);
    *json = parsed;
    return BINGO_HTTP_JSON_OK;
}

BingoHttpJsonResult bingo_http_read_json(
    struct mg_connection *connection,
    cJSON **json
)
{
    const struct mg_request_info *request;
    const char *content_type = NULL;
    char *body;
    size_t body_length;
    size_t received = 0;
    int i;
    BingoHttpJsonResult result;

    if (connection == NULL || json == NULL) return BINGO_HTTP_JSON_INVALID_ARGUMENT;
    *json = NULL;

    request = mg_get_request_info(connection);
    if (request == NULL) return BINGO_HTTP_JSON_INVALID_ARGUMENT;

    for (i = 0; i < request->num_headers; i++) {
        if (request->http_headers[i].name != NULL &&
            equals_ignore_case(request->http_headers[i].name, "Content-Type")) {
            content_type = request->http_headers[i].value;
            break;
        }
    }
    if (!is_json_content_type(content_type)) return BINGO_HTTP_JSON_INVALID_CONTENT_TYPE;
    if (request->content_length <= 0) return BINGO_HTTP_JSON_EMPTY_BODY;
    if ((unsigned long long)request->content_length > BINGO_HTTP_MAX_JSON_BODY) {
        return BINGO_HTTP_JSON_BODY_TOO_LARGE;
    }

    body_length = (size_t)request->content_length;
    body = (char *)malloc(body_length);
    if (body == NULL) return BINGO_HTTP_JSON_READ_ERROR;

    while (received < body_length) {
        int count = mg_read(connection, body + received, body_length - received);
        if (count <= 0) {
            free(body);
            return BINGO_HTTP_JSON_READ_ERROR;
        }
        received += (size_t)count;
    }

    result = bingo_http_parse_json(body, body_length, json);
    free(body);
    return result;
}

const char *bingo_http_json_error(BingoHttpJsonResult result)
{
    switch (result) {
    case BINGO_HTTP_JSON_OK: return "OK";
    case BINGO_HTTP_JSON_INVALID_ARGUMENT: return "Argumento invalido.";
    case BINGO_HTTP_JSON_EMPTY_BODY: return "O corpo JSON esta vazio.";
    case BINGO_HTTP_JSON_BODY_TOO_LARGE: return "O corpo JSON excede 1 MiB.";
    case BINGO_HTTP_JSON_INVALID_CONTENT_TYPE: return "Content-Type deve ser application/json.";
    case BINGO_HTTP_JSON_READ_ERROR: return "Nao foi possivel ler o corpo da requisicao.";
    case BINGO_HTTP_JSON_INVALID_JSON: return "O corpo contem JSON invalido.";
    case BINGO_HTTP_JSON_TRAILING_DATA: return "Ha dados apos o documento JSON.";
    }
    return "Erro desconhecido ao processar JSON.";
}
