#define _POSIX_C_SOURCE 200809L

#include "bingo_engine.h"
#include "civetweb.h"
#include "cJSON.h"

#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <sched.h>
#include <unistd.h>
#endif

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT "8080"
#define DEFAULT_DIST "dist"
#define MAX_HOST_LENGTH 128
#define MAX_PORT_LENGTH 6
#define MAX_PATH_LENGTH 1024

typedef struct {
    BingoGame game;
    int ready;
    atomic_flag mutex;
} ServerState;

static ServerState g_state = {
    .ready = 0,
    .mutex = ATOMIC_FLAG_INIT
};
static volatile sig_atomic_t g_stop_requested = 0;

static void state_lock(void)
{
    while (atomic_flag_test_and_set_explicit(&g_state.mutex, memory_order_acquire)) {
#ifdef _WIN32
        SwitchToThread();
#else
        (void)sched_yield();
#endif
    }
}

static void state_unlock(void)
{
    atomic_flag_clear_explicit(&g_state.mutex, memory_order_release);
}

static void request_shutdown(int signal_number)
{
    (void)signal_number;
    g_stop_requested = 1;
}

static int valid_port(const char *value)
{
    char *end = NULL;
    long port;

    if (value == NULL || *value == '\0') {
        return 0;
    }

    port = strtol(value, &end, 10);
    return end != value && *end == '\0' && port >= 1 && port <= 65535;
}

static int copy_option(char *destination, size_t capacity, const char *value)
{
    size_t length;

    if (value == NULL || capacity == 0) {
        return 0;
    }
    length = strlen(value);
    if (length == 0 || length >= capacity) {
        return 0;
    }
    memcpy(destination, value, length + 1);
    return 1;
}

static int parse_arguments(
    int argc,
    char **argv,
    char *host,
    size_t host_capacity,
    char *port,
    size_t port_capacity,
    char *dist,
    size_t dist_capacity
)
{
    int i;

    if (!copy_option(host, host_capacity, getenv("BINGOAT_HOST")) &&
        !copy_option(host, host_capacity, DEFAULT_HOST)) {
        return 0;
    }
    if (!copy_option(port, port_capacity, getenv("BINGOAT_PORT")) &&
        !copy_option(port, port_capacity, DEFAULT_PORT)) {
        return 0;
    }
    if (!copy_option(dist, dist_capacity, getenv("BINGOAT_DIST")) &&
        !copy_option(dist, dist_capacity, DEFAULT_DIST)) {
        return 0;
    }

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--host") == 0 && i + 1 < argc) {
            if (!copy_option(host, host_capacity, argv[++i])) return 0;
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            if (!copy_option(port, port_capacity, argv[++i])) return 0;
        } else if (strcmp(argv[i], "--dist") == 0 && i + 1 < argc) {
            if (!copy_option(dist, dist_capacity, argv[++i])) return 0;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Uso: bingo-server [--host HOST] [--port PORT] [--dist DIRETORIO]\n");
            printf("Variaveis: BINGOAT_HOST, BINGOAT_PORT e BINGOAT_DIST.\n");
            return 2;
        } else {
            fprintf(stderr, "Argumento invalido: %s\n", argv[i]);
            return 0;
        }
    }

    if (!valid_port(port)) {
        fprintf(stderr, "Porta invalida: %s (use um numero entre 1 e 65535).\n", port);
        return 0;
    }
    if (strchr(host, ':') != NULL || strchr(host, ',') != NULL) {
        fprintf(stderr, "Host invalido: informe um hostname ou endereco IPv4 sem porta.\n");
        return 0;
    }
    return 1;
}

static int send_json(struct mg_connection *connection, int status, cJSON *json)
{
    char *body;
    size_t length;
    const char *reason;
    const struct mg_request_info *request;

    body = cJSON_PrintUnformatted(json);
    if (body == NULL) {
        return mg_send_http_error(connection, 500, "Falha ao serializar resposta JSON");
    }

    length = strlen(body);
    reason = status == 200 ? "OK" : (status == 503 ? "Service Unavailable" : "Not Found");
    request = mg_get_request_info(connection);
    if (mg_printf(
            connection,
            "HTTP/1.1 %d %s\\r\\n"
            "Content-Type: application/json; charset=utf-8\\r\\n"
            "Cache-Control: no-store\\r\\n"
            "X-Content-Type-Options: nosniff\\r\\n"
            "Content-Length: %lu\\r\\n"
            "Connection: close\\r\\n\\r\\n",
            status,
            reason,
            (unsigned long)length) < 0) {
        cJSON_free(body);
        return status;
    }

    if (request == NULL || strcmp(request->request_method, "HEAD") != 0) {
        (void)mg_write(connection, body, length);
    }
    cJSON_free(body);
    return status;
}

static int health_handler(struct mg_connection *connection, void *user_data)
{
    cJSON *json;
    int ready;

    (void)user_data;
    if (strcmp(mg_get_request_info(connection)->request_method, "GET") != 0 &&
        strcmp(mg_get_request_info(connection)->request_method, "HEAD") != 0) {
        return mg_send_http_error(connection, 405, "Metodo nao permitido");
    }

    state_lock();
    ready = g_state.ready;
    state_unlock();

    json = cJSON_CreateObject();
    if (json == NULL) {
        return mg_send_http_error(connection, 500, "Falha ao alocar resposta");
    }
    cJSON_AddStringToObject(json, "status", ready ? "ok" : "not_ready");
    cJSON_AddBoolToObject(json, "service", 1);
    cJSON_AddStringToObject(json, "engine", ready ? "ready" : "not_ready");

    {
        int status = send_json(connection, ready ? 200 : 503, json);
        cJSON_Delete(json);
        return status;
    }
}

static int api_not_found_handler(struct mg_connection *connection, void *user_data)
{
    cJSON *json;
    cJSON *error;
    int status;

    (void)user_data;
    json = cJSON_CreateObject();
    error = cJSON_CreateObject();
    if (json == NULL || error == NULL) {
        cJSON_Delete(json);
        cJSON_Delete(error);
        return mg_send_http_error(connection, 500, "Falha ao alocar resposta");
    }
    cJSON_AddStringToObject(error, "code", "NOT_FOUND");
    cJSON_AddStringToObject(error, "message", "Rota da API nao encontrada.");
    cJSON_AddItemToObject(json, "error", error);
    status = send_json(connection, 404, json);
    cJSON_Delete(json);
    return status;
}

/* CivetWeb serves existing files relative to document_root and applies its
 * own canonical path checks. This handler only provides SPA navigation
 * fallback for extensionless, non-API GET/HEAD routes. */
static int spa_fallback_handler(struct mg_connection *connection, void *user_data)
{
    const struct mg_request_info *request = mg_get_request_info(connection);
    const char *uri;
    const char *method;
    size_t i;
    size_t length;
    char index_path[MAX_PATH_LENGTH];

    if (request == NULL || request->local_uri == NULL) return 0;
    uri = request->local_uri;
    method = request->request_method;

    if (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0) return 0;
    if (strncmp(uri, "/api", 4) == 0) return 0;

    length = strlen(uri);
    for (i = 0; i < length; i++) {
        unsigned char ch = (unsigned char)uri[i];
        if (ch == '%' || ch == '\\' || ch == ':' || ch < 0x20) {
            return mg_send_http_error(connection, 400, "Caminho invalido");
        }
        if (uri[i] == '.' && (i == 0 || uri[i - 1] == '/') &&
            (uri[i + 1] == '.' || uri[i + 1] == '/' || uri[i + 1] == '\0')) {
            return mg_send_http_error(connection, 400, "Caminho invalido");
        }
    }

    /* Paths containing a dot are files/assets; let CivetWeb return 404. */
    if (strrchr(uri, '.') != NULL) return 0;

    if (!copy_option(index_path, sizeof(index_path), (const char *)user_data)) {
        return mg_send_http_error(connection, 500, "Diretorio estatico invalido");
    }
    mg_send_file(connection, index_path);
    return 200;
}

static int initialize_engine(void)
{
    static const char *const initial_players[] = {"Jogador 1", "Jogador 2"};

    bingoat_seed((unsigned int)time(NULL));
    state_lock();
    g_state.ready = bingo_game_init(&g_state.game, 2, initial_players, 1);
    state_unlock();
    return g_state.ready;
}

static void wait_for_shutdown(void)
{
    while (!g_stop_requested) {
#ifdef _WIN32
        Sleep(100);
#else
        struct timespec delay = {0, 100000000L};
        while (nanosleep(&delay, &delay) != 0 && errno == EINTR && !g_stop_requested) {
        }
#endif
    }
}

int main(int argc, char **argv)
{
    char host[MAX_HOST_LENGTH];
    char port[MAX_PORT_LENGTH];
    char dist[MAX_PATH_LENGTH];
    char listening_port[MAX_HOST_LENGTH + MAX_PORT_LENGTH + 2];
    char index_path[MAX_PATH_LENGTH];
    const char *options[11];
    struct mg_context *server;
    int parse_result;

    parse_result = parse_arguments(argc, argv, host, sizeof(host), port, sizeof(port), dist, sizeof(dist));
    if (parse_result == 2) return EXIT_SUCCESS;
    if (parse_result == 0) return EXIT_FAILURE;

    if (snprintf(listening_port, sizeof(listening_port), "%s:%s", host, port) >= (int)sizeof(listening_port) ||
        snprintf(index_path, sizeof(index_path), "%s/%s", dist, "index.html") >= (int)sizeof(index_path)) {
        fprintf(stderr, "Configuracao excede o tamanho maximo permitido.\n");
        return EXIT_FAILURE;
    }

    if (!initialize_engine()) {
        fprintf(stderr, "Nao foi possivel inicializar o motor do bingo.\n");
        return EXIT_FAILURE;
    }

    if (mg_init_library(0) == 0) {
        fprintf(stderr, "Nao foi possivel inicializar a biblioteca CivetWeb.\n");
        return EXIT_FAILURE;
    }

    options[0] = "listening_ports";
    options[1] = listening_port;
    options[2] = "document_root";
    options[3] = dist;
    options[4] = "index_files";
    options[5] = "index.html";
    options[6] = "enable_directory_listing";
    options[7] = "no";
    options[8] = "num_threads";
    options[9] = "8";
    options[10] = NULL;

    server = mg_start(NULL, 0, options);
    if (server == NULL) {
        fprintf(stderr, "Nao foi possivel iniciar o servidor HTTP em %s.\n", listening_port);
        (void)mg_exit_library();
        return EXIT_FAILURE;
    }

    mg_set_request_handler(server, "/api/health", health_handler, NULL);
    mg_set_request_handler(server, "/api", api_not_found_handler, NULL);
    mg_set_request_handler(server, "/", spa_fallback_handler, index_path);

    (void)signal(SIGINT, request_shutdown);
    (void)signal(SIGTERM, request_shutdown);

    printf("BinGoat HTTP pronto em http://%s:%s\n", host, port);
    printf("Arquivos estaticos: %s\n", dist);
    printf("Health check: http://%s:%s/api/health\n", host, port);
    fflush(stdout);

    wait_for_shutdown();

    mg_stop(server);
    (void)mg_exit_library();
    state_lock();
    g_state.ready = 0;
    state_unlock();
    return EXIT_SUCCESS;
}
