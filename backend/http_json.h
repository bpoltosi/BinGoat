#ifndef BINGOAT_HTTP_JSON_H
#define BINGOAT_HTTP_JSON_H

#include "cJSON.h"
#include "civetweb.h"

#include <stddef.h>

#define BINGO_HTTP_MAX_JSON_BODY (1024U * 1024U)

typedef enum {
    BINGO_HTTP_JSON_OK = 0,
    BINGO_HTTP_JSON_INVALID_ARGUMENT,
    BINGO_HTTP_JSON_EMPTY_BODY,
    BINGO_HTTP_JSON_BODY_TOO_LARGE,
    BINGO_HTTP_JSON_INVALID_CONTENT_TYPE,
    BINGO_HTTP_JSON_READ_ERROR,
    BINGO_HTTP_JSON_INVALID_JSON,
    BINGO_HTTP_JSON_TRAILING_DATA
} BingoHttpJsonResult;

/* Parses an already-read request body. The caller owns *json on success. */
BingoHttpJsonResult bingo_http_parse_json(
    const char *body,
    size_t body_length,
    cJSON **json
);

/* Reads and validates a JSON request body from a CivetWeb connection. */
BingoHttpJsonResult bingo_http_read_json(
    struct mg_connection *connection,
    cJSON **json
);

const char *bingo_http_json_error(BingoHttpJsonResult result);

#endif
