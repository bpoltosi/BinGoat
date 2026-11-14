#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD_DIR="${ROOT_DIR}/build"
SERVER="${BUILD_DIR}/bingo-server"
SERVER_PORT="${BINGOAT_PROXY_TEST_SERVER_PORT:-18081}"
VITE_PORT="${BINGOAT_PROXY_TEST_VITE_PORT:-3001}"
SERVER_URL="http://127.0.0.1:${SERVER_PORT}"
VITE_URL="http://127.0.0.1:${VITE_PORT}"
SERVER_LOG="${BUILD_DIR}/vite-proxy-server.log"
VITE_LOG="${BUILD_DIR}/vite-proxy-vite.log"
SERVER_PID=""
VITE_PID=""

if [ ! -x "${SERVER}" ]; then
    echo "Servidor C nao encontrado. Execute o build C antes deste teste." >&2
    exit 1
fi
if [ ! -d "${ROOT_DIR}/node_modules" ]; then
    echo "Dependencias npm ausentes. Execute npm install antes deste teste." >&2
    exit 1
fi
if [ ! -f "${ROOT_DIR}/dist/index.html" ]; then
    echo "Build frontend ausente. Execute npm run build antes deste teste." >&2
    exit 1
fi

cleanup() {
    if [ -n "${VITE_PID}" ] && kill -0 "${VITE_PID}" 2>/dev/null; then
        kill -TERM "${VITE_PID}" 2>/dev/null || true
        wait "${VITE_PID}" 2>/dev/null || true
    fi
    if [ -n "${SERVER_PID}" ] && kill -0 "${SERVER_PID}" 2>/dev/null; then
        kill -TERM "${SERVER_PID}" 2>/dev/null || true
        wait "${SERVER_PID}" 2>/dev/null || true
    fi
}
trap cleanup EXIT INT TERM

"${SERVER}" --host 127.0.0.1 --port "${SERVER_PORT}" --dist "${ROOT_DIR}/dist" >"${SERVER_LOG}" 2>&1 &
SERVER_PID=$!

attempt=0
until curl --fail --silent "${SERVER_URL}/api/health" >/dev/null; do
    attempt=$((attempt + 1))
    if [ "${attempt}" -ge 30 ]; then
        cat "${SERVER_LOG}" >&2
        echo "Servidor C nao ficou pronto para o teste do proxy." >&2
        exit 1
    fi
    sleep 1
done

BINGOAT_API_TARGET="${SERVER_URL}" npm run dev -- --host 127.0.0.1 --port "${VITE_PORT}" --strictPort >"${VITE_LOG}" 2>&1 &
VITE_PID=$!

attempt=0
until curl --fail --silent "${VITE_URL}/api/health" >"${BUILD_DIR}/vite-proxy-health.json"; do
    attempt=$((attempt + 1))
    if [ "${attempt}" -ge 30 ]; then
        cat "${VITE_LOG}" >&2
        echo "Vite nao encaminhou /api/health ao servidor C." >&2
        exit 1
    fi
    sleep 1
done

grep -q '"status":"ok"' "${BUILD_DIR}/vite-proxy-health.json"
curl --fail --silent "${VITE_URL}/api/games/current?boardCols=10" >"${BUILD_DIR}/vite-proxy-game.json"
grep -q '"calledNumbers":\[\]' "${BUILD_DIR}/vite-proxy-game.json"
grep -q '"players":\[' "${BUILD_DIR}/vite-proxy-game.json"

echo "Proxy Vite -> servidor HTTP C validado com health check e estado da partida."
