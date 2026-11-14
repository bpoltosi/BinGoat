#include "game_api.h"
#include "bingo_engine.h"
#include "http_json.h"
#include "cJSON.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { BingoGame game; int ready; atomic_flag lock; } ApiState;
static ApiState state = { .ready = 0, .lock = ATOMIC_FLAG_INIT };

static void lock_state(void) {
    while (atomic_flag_test_and_set_explicit(&state.lock, memory_order_acquire)) { }
}
static void unlock_state(void) {
    atomic_flag_clear_explicit(&state.lock, memory_order_release);
}
static int respond(struct mg_connection *c, int status, cJSON *json) {
    char *body = cJSON_PrintUnformatted(json);
    const char *reason = status == 200 ? "OK" :
        status == 400 ? "Bad Request" : status == 404 ? "Not Found" :
        status == 405 ? "Method Not Allowed" : status == 409 ? "Conflict" :
        status == 413 ? "Payload Too Large" : status == 415 ? "Unsupported Media Type" :
        status == 503 ? "Service Unavailable" : "Internal Server Error";
    int result;
    if (!body) return mg_send_http_error(c, 500, "Falha ao serializar JSON");
    result = mg_printf(c,
        "HTTP/1.1 %d %s\r\nContent-Type: application/json; charset=utf-8\r\n"
        "Cache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\n"
        "Content-Length: %lu\r\nConnection: close\r\n\r\n",
        status, reason, (unsigned long)strlen(body));
    if (result >= 0) {
        const struct mg_request_info *ri = mg_get_request_info(c);
        if (!ri || strcmp(ri->request_method, "HEAD") != 0)
            (void)mg_write(c, body, strlen(body));
    }
    cJSON_free(body);
    return status;
}
static int fail(struct mg_connection *c, int status, const char *code, const char *message) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return mg_send_http_error(c, 500, "Falha ao alocar JSON");
    cJSON_AddStringToObject(root, "error", message);
    cJSON_AddStringToObject(root, "code", code);
    { int result = respond(c, status, root); cJSON_Delete(root); return result; }
}
static int method_is(struct mg_connection *c, const char *method) {
    const struct mg_request_info *ri = mg_get_request_info(c);
    return ri && ri->request_method && strcmp(ri->request_method, method) == 0;
}
static int read_body(struct mg_connection *c, cJSON **body) {
    BingoHttpJsonResult r = bingo_http_read_json(c, body);
    if (r == BINGO_HTTP_JSON_OK) return 1;
    if (r == BINGO_HTTP_JSON_INVALID_CONTENT_TYPE)
        fail(c, 415, "UNSUPPORTED_MEDIA_TYPE", bingo_http_json_error(r));
    else if (r == BINGO_HTTP_JSON_BODY_TOO_LARGE)
        fail(c, 413, "PAYLOAD_TOO_LARGE", bingo_http_json_error(r));
    else
        fail(c, 400, "INVALID_JSON", bingo_http_json_error(r));
    return 0;
}
static int get_int(const cJSON *obj, const char *key, int fallback, int min, int max, int *out) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!v) { *out = fallback; return 1; }
    if (!cJSON_IsNumber(v) || v->valuedouble != (double)v->valueint ||
        v->valueint < min || v->valueint > max) return 0;
    *out = v->valueint; return 1;
}
static int get_bool(const cJSON *obj, const char *key, int fallback, int *out) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!v) { *out = fallback; return 1; }
    if (!cJSON_IsBool(v)) return 0;
    *out = cJSON_IsTrue(v); return 1;
}
static const char *orientation_name(int o) {
    return o == 0 ? "horizontal" : o == 1 ? "vertical" : "diagonal";
}
static void add_lines(cJSON *array, const int *values, const int *marked,
                      int total, int rows, int cols, const char *prefix) {
    LinhaCompleta lines[MAX_LINHAS_ATIVAS];
    int count = detectarLinhasGrade((int *)values, (int *)marked, total, rows, cols,
                                    lines, MAX_LINHAS_ATIVAS);
    int i, j;
    for (i = 0; i < count; i++) {
        cJSON *line = cJSON_CreateObject(), *nums = cJSON_CreateArray();
        char key[96], label[64];
        if (!line || !nums) { cJSON_Delete(line); cJSON_Delete(nums); continue; }
        if (lines[i].orientacao == 0) snprintf(key, sizeof(key), "%s-h-%d", prefix, lines[i].indiceA);
        else if (lines[i].orientacao == 1) snprintf(key, sizeof(key), "%s-v-%d", prefix, lines[i].indiceA);
        else snprintf(key, sizeof(key), "%s-d-%d-%d-%d", prefix, lines[i].orientacao,
                      lines[i].indiceA, lines[i].indiceB);
        snprintf(label, sizeof(label), "%s %d", orientation_name(lines[i].orientacao),
                 lines[i].indiceA + 1);
        for (j = 0; j < lines[i].tamanho; j++) cJSON_AddItemToArray(nums, cJSON_CreateNumber(lines[i].numeros[j]));
        cJSON_AddStringToObject(line, "key", key);
        cJSON_AddStringToObject(line, "orientation", orientation_name(lines[i].orientacao));
        cJSON_AddStringToObject(line, "label", label);
        cJSON_AddItemToObject(line, "numbers", nums);
        cJSON_AddItemToArray(array, line);
    }
}
static cJSON *game_json(const BingoGame *g, int cols) {
    cJSON *root = cJSON_CreateObject(), *called = cJSON_CreateArray();
    cJSON *boardLines = cJSON_CreateArray(), *players = cJSON_CreateArray();
    int boardValues[MAX_NUMERO_BINGO], boardMarked[MAX_NUMERO_BINGO] = {0};
    int i, j;
    if (!root || !called || !boardLines || !players) goto error;
    for (i = 0; i < g->qtdNumerosSorteados; i++) {
        cJSON_AddItemToArray(called, cJSON_CreateNumber(g->numerosSorteados[i]));
        if (g->numerosSorteados[i] >= 1 && g->numerosSorteados[i] <= 60)
            boardMarked[g->numerosSorteados[i] - 1] = 1;
    }
    for (i = 0; i < 60; i++) boardValues[i] = i + 1;
    add_lines(boardLines, boardValues, boardMarked, 60, 60 / cols, cols, "board");
    for (i = 0; i < g->qtdJogadores; i++) {
        const Jogador *p = &g->players[i];
        cJSON *player = cJSON_CreateObject(), *numbers = cJSON_CreateArray();
        cJSON *marked = cJSON_CreateArray(), *lines = cJSON_CreateArray();
        int values[TAMANHO_CARTELA], marks[TAMANHO_CARTELA];
        if (!player || !numbers || !marked || !lines) {
            cJSON_Delete(player); cJSON_Delete(numbers); cJSON_Delete(marked); cJSON_Delete(lines); goto error;
        }
        for (j = 0; j < TAMANHO_CARTELA; j++) {
            values[j] = p->cartela.numero[j]; marks[j] = p->cartela.marcado[j];
            cJSON_AddItemToArray(numbers, cJSON_CreateNumber(values[j]));
            if (marks[j]) cJSON_AddItemToArray(marked, cJSON_CreateNumber(values[j]));
        }
        {
            char prefix[32];
            snprintf(prefix, sizeof(prefix), "player-%d", p->id);
            add_lines(lines, values, marks, TAMANHO_CARTELA, 3, 5, prefix);
        }
        cJSON_AddNumberToObject(player, "id", p->id);
        cJSON_AddStringToObject(player, "name", p->nome);
        cJSON_AddItemToObject(player, "numbers", numbers);
        cJSON_AddItemToObject(player, "markedNumbers", marked);
        if (p->venceu) cJSON_AddNumberToObject(player, "wonRound", p->rodadaVitoria);
        else cJSON_AddNullToObject(player, "wonRound");
        cJSON_AddItemToObject(player, "lines", lines);
        cJSON_AddItemToArray(players, player);
    }
    cJSON_AddItemToObject(root, "calledNumbers", called);
    cJSON_AddItemToObject(root, "boardLines", boardLines);
    cJSON_AddItemToObject(root, "players", players);
    cJSON_AddBoolToObject(root, "autoMark", g->autoMark != 0);
    return root;
error:
    cJSON_Delete(root); cJSON_Delete(called); cJSON_Delete(boardLines); cJSON_Delete(players);
    return NULL;
}
static int query_cols(struct mg_connection *c, int *cols) {
    const struct mg_request_info *ri = mg_get_request_info(c);
    char value[16];
    int n;
    *cols = 10;
    if (!ri || !ri->query_string) return 1;
    if (mg_get_var(ri->query_string, strlen(ri->query_string), "boardCols", value, sizeof(value)) <= 0) return 1;
    if (sscanf(value, "%d", &n) != 1 || (n != 5 && n != 6 && n != 10)) return 0;
    *cols = n; return 1;
}
static int send_game(struct mg_connection *c, int cols, int drawn) {
    cJSON *json;
    int result;
    lock_state();
    if (!state.ready) { unlock_state(); return fail(c, 503, "ENGINE_NOT_READY", "Motor do bingo indisponivel."); }
    json = game_json(&state.game, cols);
    unlock_state();
    if (!json) return fail(c, 500, "SERIALIZATION_ERROR", "Nao foi possivel montar o estado da partida.");
    if (drawn > 0) cJSON_AddNumberToObject(json, "drawnNumber", drawn);
    result = respond(c, 200, json); cJSON_Delete(json); return result;
}
static int current_get(struct mg_connection *c, void *data) {
    int cols;
    const struct mg_request_info *ri = mg_get_request_info(c);
    (void)data;
    if (!ri || !ri->local_uri || strcmp(ri->local_uri, "/api/games/current") != 0)
        return 0;
    if (!method_is(c, "GET") && !method_is(c, "HEAD")) return fail(c, 405, "METHOD_NOT_ALLOWED", "Metodo nao permitido.");
    if (!query_cols(c, &cols)) return fail(c, 400, "INVALID_BOARD_COLUMNS", "boardCols deve ser 5, 6 ou 10.");
    return send_game(c, cols, 0);
}
static int mutate(struct mg_connection *c, void *data) {
    const char *action = (const char *)data;
    const struct mg_request_info *ri = mg_get_request_info(c);
    const char *expected = strcmp(action, "draw") == 0 ? "/api/games/current/draw" :
        strcmp(action, "toggle-board") == 0 ? "/api/games/current/toggle-board" :
        strcmp(action, "toggle-card") == 0 ? "/api/games/current/toggle-card" :
        strcmp(action, "sync") == 0 ? "/api/games/current/sync" :
        strcmp(action, "undo") == 0 ? "/api/games/current/undo" : "/api/games/current/reset";
    cJSON *body = NULL;
    int cols = 10, number = 0, player = 0, count = 0, flag = 0, ok = 0, drawn = 0;
    const cJSON *namesNode;
    const char *names[MAX_JOGADORES] = {0};
    char nameBuffers[MAX_JOGADORES][50];
    int i;
    if (!ri || !ri->local_uri || strcmp(ri->local_uri, expected) != 0)
        return 0;
    if (!method_is(c, "POST")) return fail(c, 405, "METHOD_NOT_ALLOWED", "Metodo nao permitido.");
    if (!read_body(c, &body)) return 400;
    if (!get_int(body, "boardCols", 10, 5, 10, &cols) || (cols != 5 && cols != 6 && cols != 10)) {
        cJSON_Delete(body); return fail(c, 400, "INVALID_BOARD_COLUMNS", "boardCols deve ser 5, 6 ou 10.");
    }
    lock_state();
    if (!state.ready) { unlock_state(); cJSON_Delete(body); return fail(c, 503, "ENGINE_NOT_READY", "Motor do bingo indisponivel."); }
    if (strcmp(action, "draw") == 0) {
        if (bingo_game_finished(&state.game)) {
            unlock_state(); cJSON_Delete(body);
            return fail(c, 409, "GAME_FINISHED", "A partida ja foi concluida.");
        }
        ok = bingo_game_draw(&state.game, &drawn);
    } else if (strcmp(action, "toggle-board") == 0) {
        if (!get_int(body, "number", 0, 1, 60, &number) ||
            !get_bool(body, "autoMark", state.game.autoMark, &flag)) goto invalid;
        ok = bingo_game_toggle_board(&state.game, number);
        if (ok && cJSON_GetObjectItemCaseSensitive(body, "autoMark")) {
            state.game.autoMark = flag;
            if (flag) {
                int k; for (k = 0; k < state.game.qtdJogadores; k++)
                    sincronizarMarcacoesAutomaticas(&state.game.players[k], 1,
                        state.game.numerosSorteados, state.game.qtdNumerosSorteados);
                atualizarVencedores(state.game.players, state.game.qtdJogadores, state.game.qtdNumerosSorteados);
            }
        }
    } else if (strcmp(action, "toggle-card") == 0) {
        if (!get_int(body, "playerId", 0, 1, MAX_JOGADORES, &player) ||
            !get_int(body, "number", 0, 1, 60, &number)) goto invalid;
        ok = bingo_game_toggle_card(&state.game, player, number);
    } else if (strcmp(action, "sync") == 0) {
        count = state.game.qtdJogadores;
        if (!get_int(body, "playerCount", count, MIN_JOGADORES, MAX_JOGADORES, &count) ||
            !get_bool(body, "regenerateCards", 0, &flag)) goto invalid;
        namesNode = cJSON_GetObjectItemCaseSensitive(body, "playerNames");
        if (namesNode && !cJSON_IsArray(namesNode)) goto invalid;
        for (i = 0; i < count; i++) {
            const cJSON *entry;
            if (i >= MAX_JOGADORES) goto invalid;
            entry = namesNode ? cJSON_GetArrayItem(namesNode, i) : NULL;
            if (entry && !cJSON_IsString(entry)) goto invalid;
            if (entry && entry->valuestring) {
                size_t len = strlen(entry->valuestring);
                if (len == 0 || len > 32) goto invalid;
                memcpy(nameBuffers[i], entry->valuestring, len + 1U);
                names[i] = nameBuffers[i];
            } else if (i < state.game.qtdJogadores) {
                size_t len = strlen(state.game.players[i].nome);
                if (len >= sizeof(nameBuffers[i])) goto invalid;
                memcpy(nameBuffers[i], state.game.players[i].nome, len + 1U);
                names[i] = nameBuffers[i];
            }
        }
        if (!get_bool(body, "autoMark", state.game.autoMark, &player)) goto invalid;
        ok = bingo_game_sync(&state.game, count, names, flag, player);
    } else if (strcmp(action, "undo") == 0) {
        ok = bingo_game_undo(&state.game);
    } else if (strcmp(action, "reset") == 0) {
        if (!get_bool(body, "newCards", 0, &flag)) goto invalid;
        bingo_game_reset(&state.game, flag); ok = 1;
    }
    unlock_state();
    cJSON_Delete(body);
    if (!ok) return fail(c, 409, "GAME_ACTION_REJECTED", "A operacao nao pode ser aplicada ao estado atual.");
    return send_game(c, cols, drawn);
invalid:
    unlock_state(); cJSON_Delete(body);
    return fail(c, 400, "INVALID_ARGUMENT", "Um ou mais campos da requisicao sao invalidos.");
}
int bingo_api_init(void) {
    static const char *const initial[] = {"Jogador 1", "Jogador 2"};
    lock_state(); state.ready = bingo_game_init(&state.game, 2, initial, 1); unlock_state();
    return state.ready;
}
void bingo_api_register(struct mg_context *server) {
    mg_set_request_handler(server, "/api/games/current", current_get, NULL);
    mg_set_request_handler(server, "/api/games/current/draw", mutate, (void *)"draw");
    mg_set_request_handler(server, "/api/games/current/toggle-board", mutate, (void *)"toggle-board");
    mg_set_request_handler(server, "/api/games/current/toggle-card", mutate, (void *)"toggle-card");
    mg_set_request_handler(server, "/api/games/current/sync", mutate, (void *)"sync");
    mg_set_request_handler(server, "/api/games/current/undo", mutate, (void *)"undo");
    mg_set_request_handler(server, "/api/games/current/reset", mutate, (void *)"reset");
}
