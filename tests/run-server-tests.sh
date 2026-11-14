#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD_DIR="${ROOT_DIR}/build"
SERVER="${BUILD_DIR}/bingo-server"
STATIC_ROOT="${ROOT_DIR}/dist"
if [ ! -f "${STATIC_ROOT}/index.html" ]; then STATIC_ROOT="${ROOT_DIR}/tests/fixtures/static"; fi
PORT="${BINGOAT_TEST_PORT:-18080}"
BASE_URL="http://127.0.0.1:${PORT}"
LOG_FILE="${BUILD_DIR}/server-test.log"
PID_FILE="${BUILD_DIR}/server-test.pid"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" --config Release

"${SERVER}" --host 127.0.0.1 --port "${PORT}" --dist "${STATIC_ROOT}" >"${LOG_FILE}" 2>&1 &
SERVER_PID=$!
printf '%s\n' "${SERVER_PID}" > "${PID_FILE}"

cleanup() {
    if kill -0 "${SERVER_PID}" 2>/dev/null; then
        kill -TERM "${SERVER_PID}" 2>/dev/null || true
        if ! wait "${SERVER_PID}"; then
            echo "Servidor nao encerrou corretamente apos SIGTERM." >&2
            rm -f "${PID_FILE}"
            exit 1
        fi
    else
        wait "${SERVER_PID}" 2>/dev/null || true
    fi
    rm -f "${PID_FILE}"
}
trap cleanup EXIT INT TERM

attempt=0
until curl --fail --silent "${BASE_URL}/api/health" >/dev/null; do
    attempt=$((attempt + 1))
    if [ "${attempt}" -ge 30 ]; then
        cat "${LOG_FILE}"
        echo "Servidor nao ficou pronto." >&2
        exit 1
    fi
    sleep 1
done

HEALTH=$(curl --fail --silent "${BASE_URL}/api/health")
printf '%s' "${HEALTH}" | grep -q '"status":"ok"'
printf '%s' "${HEALTH}" | grep -q '"engine":"ready"'

curl --fail --silent "${BASE_URL}/" > "${BUILD_DIR}/root-response.html"
cmp "${STATIC_ROOT}/index.html" "${BUILD_DIR}/root-response.html"
curl --fail --silent "${BASE_URL}/partida/123" > "${BUILD_DIR}/spa-response.html"
cmp "${STATIC_ROOT}/index.html" "${BUILD_DIR}/spa-response.html"

ASSET=$(find "${STATIC_ROOT}" -type f ! -name index.html | head -n 1 || true)
if [ -n "${ASSET}" ]; then
    ASSET_PATH=${ASSET#"$STATIC_ROOT"/}
    ASSET_PATH=$(printf '%s' "$ASSET_PATH" | sed 's#\\#/#g')
    curl --fail --silent "${BASE_URL}/${ASSET_PATH}" > "${BUILD_DIR}/asset-response"
    cmp "${ASSET}" "${BUILD_DIR}/asset-response"
fi

STATUS=$(curl --silent --output /dev/null --write-out '%{http_code}' "${BASE_URL}/api/rota-inexistente")
[ "${STATUS}" = "404" ]

TRAVERSAL_STATUS=$(curl --path-as-is --silent --output /dev/null --write-out '%{http_code}' "${BASE_URL}/../README.md")
[ "${TRAVERSAL_STATUS}" != "200" ]

echo "Todos os testes HTTP do servidor passaram."
