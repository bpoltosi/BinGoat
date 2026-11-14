#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD_DIR="${ROOT_DIR}/build"
SERVER="${BUILD_DIR}/bingo-server"
PORT="${BINGOAT_TEST_PORT:-18080}"
BASE_URL="http://127.0.0.1:${PORT}"
LOG_FILE="${BUILD_DIR}/server-test.log"
PID_FILE="${BUILD_DIR}/server-test.pid"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" --config Release

"${SERVER}" --host 127.0.0.1 --port "${PORT}" --dist "${ROOT_DIR}/tests/fixtures/static" >"${LOG_FILE}" 2>&1 &
SERVER_PID=$!
printf '%s\n' "${SERVER_PID}" > "${PID_FILE}"

cleanup() {
    kill "${SERVER_PID}" 2>/dev/null || true
    wait "${SERVER_PID}" 2>/dev/null || true
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

curl --fail --silent "${BASE_URL}/" | grep -q 'BinGoat static fixture'
curl --fail --silent "${BASE_URL}/partida/123" | grep -q 'BinGoat static fixture'

STATUS=$(curl --silent --output /dev/null --write-out '%{http_code}' "${BASE_URL}/api/rota-inexistente")
[ "${STATUS}" = "404" ]

echo "Todos os testes HTTP do servidor passaram."
