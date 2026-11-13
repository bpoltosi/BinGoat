#!/usr/bin/env sh
set -eu
ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT_DIR"
BUILD_DIR=$(mktemp -d "${TMPDIR:-/tmp}/bingoat-c-tests.XXXXXX")
cleanup(){ rm -rf "$BUILD_DIR"; rm -f tests/.players-input.tmp tests/.cli-input.tmp tests/.cli-output.tmp; }
trap cleanup EXIT HUP INT TERM
CC=${CC:-gcc}; CFLAGS=${CFLAGS:--std=c11 -Wall -Wextra -Wpedantic -O2}; EXE_SUFFIX=""
case "$(uname -s)" in MINGW*|MSYS*|CYGWIN*) EXE_SUFFIX=".exe";; esac
# shellcheck disable=SC2086
$CC $CFLAGS tests/test_engine.c bingo_engine.c -o "$BUILD_DIR/test-engine$EXE_SUFFIX"
"$BUILD_DIR/test-engine$EXE_SUFFIX"
# shellcheck disable=SC2086
$CC $CFLAGS tests/test_engine_state.c bingo_engine.c -o "$BUILD_DIR/test-engine-state$EXE_SUFFIX"
"$BUILD_DIR/test-engine-state$EXE_SUFFIX"
printf '\nSuite C concluida com sucesso.\n'
