import fs from "node:fs/promises";
import path from "node:path";
import { runC } from "picoc-js";

const C_SOURCE_PATH = path.resolve(process.cwd(), "TrabalhoBingo.c");
const MAX_PLAYERS = 5;
const MIN_PLAYERS = 2;
const CARD_SIZE = 15;
const MAX_NUMBER = 60;

const CMD = {
  CREATE_CARDS: 1,
  DRAW: 2,
  TOGGLE_BOARD: 3,
  TOGGLE_CARD: 4,
  SYNC: 5,
  UNDO: 6,
  RESET: 7,
};

function sanitizeCString(value, fallback) {
  const cleaned = String(value ?? "")
    .replace(/[\r\n\t"\\]/g, " ")
    .replace(/\s+/g, " ")
    .trim()
    .slice(0, 48);
  return cleaned || fallback;
}

function padIntArray(arr, targetLen, fill = 0) {
  const out = new Array(targetLen).fill(fill);
  for (let i = 0; i < Math.min(arr.length, targetLen); i++) {
    const val = Number(arr[i]);
    out[i] = Number.isInteger(val) ? val : fill;
  }
  return out;
}

function buildPreamble({
  cmd,
  state,
  targetNum = 0,
  targetPlayer = 0,
  resetCalled = false,
}) {
  const seed = Math.floor(Math.random() * 1_000_000_000);
  const called = state.calledNumbers.slice(0, MAX_NUMBER);
  const calledPadded = padIntArray(called, MAX_NUMBER, 0);

  const playerCount = Math.max(
    MIN_PLAYERS,
    Math.min(MAX_PLAYERS, state.players.length)
  );
  const existingPlayers = state.players.filter(
    (p) => Array.isArray(p.numbers) && p.numbers.length === CARD_SIZE
  ).length;

  const names = [];
  const won = [];
  const winRound = [];
  const cards = [];
  const marks = [];

  for (let i = 0; i < MAX_PLAYERS; i++) {
    const p = state.players[i];
    names.push(`"${sanitizeCString(p?.name, `Jogador ${i + 1}`)}"`);
    won.push(p?.wonRound ? 1 : 0);
    winRound.push(Number.isInteger(p?.wonRound) ? p.wonRound : 0);

    const cardNums =
      p && Array.isArray(p.numbers) && p.numbers.length === CARD_SIZE
        ? padIntArray(p.numbers, CARD_SIZE, 0)
        : new Array(CARD_SIZE).fill(0);

    const markedSet = new Set(Array.isArray(p?.markedNumbers) ? p.markedNumbers : []);
    const cardMarks = cardNums.map((n) => (markedSet.has(n) ? 1 : 0));

    cards.push(`{${cardNums.join(",")}}`);
    marks.push(`{${cardMarks.join(",")}}`);
  }

  return [
    "#define BINGOAT_WEB_API 1",
    `#define BINGOAT_CMD ${cmd}`,
    `#define BINGOAT_SEED ${seed}`,
    `#define BINGOAT_PLAYER_COUNT ${playerCount}`,
    `#define BINGOAT_CALLED_COUNT ${called.length}`,
    `#define BINGOAT_BOARD_COLS ${Number(state.boardCols) || 10}`,
    `#define BINGOAT_AUTO_MARK ${state.autoMark ? 1 : 0}`,
    `#define BINGOAT_TARGET_NUM ${Number(targetNum) || 0}`,
    `#define BINGOAT_TARGET_PLAYER ${Number(targetPlayer) || 0}`,
    `#define BINGOAT_EXISTING_PLAYERS ${existingPlayers}`,
    `#define BINGOAT_RESET_CALLED ${resetCalled ? 1 : 0}`,
    `int BINGOAT_CALLED_INIT[${MAX_NUMBER}] = {${calledPadded.join(",")}};`,
    `char BINGOAT_PLAYER_NAMES[${MAX_PLAYERS}][50] = {${names.join(",")}};`,
    `int BINGOAT_PLAYER_WON[${MAX_PLAYERS}] = {${won.join(",")}};`,
    `int BINGOAT_PLAYER_WIN_ROUND[${MAX_PLAYERS}] = {${winRound.join(",")}};`,
    `int BINGOAT_PLAYER_CARDS[${MAX_PLAYERS}][${CARD_SIZE}] = {${cards.join(",")}};`,
    `int BINGOAT_PLAYER_MARKS[${MAX_PLAYERS}][${CARD_SIZE}] = {${marks.join(",")}};`,
  ].join("\n");
}

async function executeCProgram(options) {
  const cSource = await fs.readFile(C_SOURCE_PATH, "utf8");
  const preamble = buildPreamble(options);
  const fullProgram = `${preamble}\n${cSource}`;

  return new Promise((resolve, reject) => {
    const outputLines = [];
    let settled = false;

    const timer = setTimeout(() => {
      if (!settled) {
        settled = true;
        reject(new Error("Tempo esgotado ao executar TrabalhoBingo.c"));
      }
    }, 5000);

    try {
      runC(fullProgram, (line) => {
        if (settled) return;
        const text = String(line ?? "");

        if (text.trim() === "__BINGOAT_END__") {
          settled = true;
          clearTimeout(timer);
          const rawJson = outputLines.join("").trim();
          try {
            const parsed = JSON.parse(rawJson);
            resolve(parsed);
          } catch (err) {
            reject(
              new Error(
                `Falha ao interpretar saída JSON de TrabalhoBingo.c: ${err.message}`
              )
            );
          }
          return;
        }

        if (text.includes("error") || text.includes("Assertion failed")) {
          settled = true;
          clearTimeout(timer);
          reject(new Error(`Erro em TrabalhoBingo.c: ${text}`));
          return;
        }

        outputLines.push(text);
      });
    } catch (err) {
      if (!settled) {
        settled = true;
        clearTimeout(timer);
        reject(err);
      }
    }
  });
}

let currentState = null;
let executionQueue = Promise.resolve();

function enqueue(task) {
  const next = executionQueue.then(task, task);
  executionQueue = next.catch(() => {});
  return next;
}

async function ensureState(boardCols = 10) {
  if (currentState) return currentState;

  const initial = {
    calledNumbers: [],
    boardCols,
    autoMark: true,
    players: [{ name: "Jogador 1" }, { name: "Jogador 2" }],
  };

  const result = await executeCProgram({
    cmd: CMD.CREATE_CARDS,
    state: initial,
    resetCalled: true,
  });

  currentState = {
    ...result,
    boardCols,
  };
  return currentState;
}

function readJsonBody(req) {
  return new Promise((resolve, reject) => {
    const chunks = [];
    req.on("data", (chunk) => chunks.push(chunk));
    req.on("end", () => {
      if (chunks.length === 0) {
        resolve({});
        return;
      }
      try {
        resolve(JSON.parse(Buffer.concat(chunks).toString("utf8")));
      } catch {
        reject(new Error("JSON inválido na requisição."));
      }
    });
    req.on("error", reject);
  });
}

function sendJson(res, statusCode, payload) {
  const body = JSON.stringify(payload);
  res.statusCode = statusCode;
  res.setHeader("Content-Type", "application/json; charset=utf-8");
  res.end(body);
}

export async function handleApiRequest(req, res, pathname) {
  try {
    const url = new URL(req.url || "/", "http://localhost");
    const route = pathname || url.pathname;

    if (req.method === "GET" && route === "/api/health") {
      await fs.access(C_SOURCE_PATH);
      return sendJson(res, 200, {
        status: "ok",
        engine: "TrabalhoBingo.c",
      });
    }

    if (req.method === "GET" && route === "/api/games/current") {
      const boardCols = Number(url.searchParams.get("boardCols")) || 10;
      const game = await enqueue(async () => {
        const existing = await ensureState(boardCols);
        if (existing.boardCols !== boardCols) {
          const synced = await executeCProgram({
            cmd: CMD.SYNC,
            state: { ...existing, boardCols },
          });
          currentState = { ...synced, boardCols };
        }
        return currentState;
      });
      return sendJson(res, 200, game);
    }

    const body = req.method === "POST" ? await readJsonBody(req) : {};
    const boardCols = Number(body.boardCols) || currentState?.boardCols || 10;

    if (req.method === "POST" && route === "/api/games") {
      const count = Math.max(
        MIN_PLAYERS,
        Math.min(MAX_PLAYERS, Number(body.playerCount) || body.players?.length || 2)
      );
      const players = Array.from({ length: count }, (_, idx) => ({
        name:
          typeof body.players?.[idx] === "string"
            ? body.players[idx]
            : `Jogador ${idx + 1}`,
      }));

      const game = await enqueue(async () => {
        const result = await executeCProgram({
          cmd: CMD.CREATE_CARDS,
          state: {
            calledNumbers: [],
            boardCols,
            autoMark: body.autoMark !== undefined ? Boolean(body.autoMark) : true,
            players,
          },
          resetCalled: true,
        });
        currentState = { ...result, boardCols };
        return currentState;
      });

      return sendJson(res, 201, game);
    }

    if (req.method === "POST" && route === "/api/games/current/draw") {
      const game = await enqueue(async () => {
        const base = await ensureState(boardCols);
        const autoMark =
          body.autoMark !== undefined ? Boolean(body.autoMark) : base.autoMark;
        const result = await executeCProgram({
          cmd: CMD.DRAW,
          state: { ...base, boardCols, autoMark },
        });
        currentState = { ...result, boardCols };
        return currentState;
      });
      return sendJson(res, 200, game);
    }

    if (req.method === "POST" && route === "/api/games/current/toggle-board") {
      const number = Number(body.number);
      if (!Number.isInteger(number) || number < 1 || number > MAX_NUMBER) {
        return sendJson(res, 400, { error: "Número inválido para o tabuleiro." });
      }
      const game = await enqueue(async () => {
        const base = await ensureState(boardCols);
        const autoMark =
          body.autoMark !== undefined ? Boolean(body.autoMark) : base.autoMark;
        const result = await executeCProgram({
          cmd: CMD.TOGGLE_BOARD,
          state: { ...base, boardCols, autoMark },
          targetNum: number,
        });
        currentState = { ...result, boardCols };
        return currentState;
      });
      return sendJson(res, 200, game);
    }

    if (req.method === "POST" && route === "/api/games/current/toggle-card") {
      const playerId = Number(body.playerId);
      const number = Number(body.number);
      if (!Number.isInteger(playerId) || !Number.isInteger(number)) {
        return sendJson(res, 400, { error: "Jogador ou número inválido." });
      }
      const game = await enqueue(async () => {
        const base = await ensureState(boardCols);
        // Ao marcar manualmente na cartela, preserva marcações atuais se autoMark estava ativo
        const preparedPlayers = base.players.map((p) => {
          if (!base.autoMark) return p;
          const calledSet = new Set(base.calledNumbers);
          const merged = new Set(p.markedNumbers);
          for (const n of p.numbers) {
            if (calledSet.has(n)) merged.add(n);
          }
          return { ...p, markedNumbers: Array.from(merged) };
        });

        const result = await executeCProgram({
          cmd: CMD.TOGGLE_CARD,
          state: {
            ...base,
            players: preparedPlayers,
            boardCols,
            autoMark: false,
          },
          targetPlayer: playerId,
          targetNum: number,
        });
        currentState = { ...result, boardCols };
        return currentState;
      });
      return sendJson(res, 200, game);
    }

    if (req.method === "POST" && route === "/api/games/current/sync") {
      const game = await enqueue(async () => {
        const base = await ensureState(boardCols);
        const nextAutoMark =
          body.autoMark !== undefined ? Boolean(body.autoMark) : base.autoMark;
        const regenerateCards = Boolean(body.regenerateCards);

        let nextPlayers = base.players;
        if (Number.isInteger(Number(body.playerCount))) {
          const targetCount = Math.max(
            MIN_PLAYERS,
            Math.min(MAX_PLAYERS, Number(body.playerCount))
          );
          nextPlayers = Array.from({ length: targetCount }, (_, idx) => {
            const existing = base.players[idx];
            if (existing && !regenerateCards) return existing;
            return {
              name: existing?.name || `Jogador ${idx + 1}`,
            };
          });
        }

        if (Array.isArray(body.playerNames)) {
          nextPlayers = nextPlayers.map((p, idx) => ({
            ...p,
            name:
              typeof body.playerNames[idx] === "string" && body.playerNames[idx].trim()
                ? body.playerNames[idx].trim()
                : p.name || `Jogador ${idx + 1}`,
          }));
        }

        const result = await executeCProgram({
          cmd: regenerateCards ? CMD.CREATE_CARDS : CMD.SYNC,
          state: {
            ...base,
            players: nextPlayers,
            boardCols,
            autoMark: nextAutoMark,
          },
          resetCalled: false,
        });
        currentState = { ...result, boardCols };
        return currentState;
      });
      return sendJson(res, 200, game);
    }

    if (req.method === "POST" && route === "/api/games/current/undo") {
      const game = await enqueue(async () => {
        const base = await ensureState(boardCols);
        const result = await executeCProgram({
          cmd: CMD.UNDO,
          state: { ...base, boardCols },
        });
        currentState = { ...result, boardCols };
        return currentState;
      });
      return sendJson(res, 200, game);
    }

    if (req.method === "POST" && route === "/api/games/current/reset") {
      const newCards = Boolean(body.newCards);
      const game = await enqueue(async () => {
        const base = await ensureState(boardCols);
        const result = await executeCProgram({
          cmd: newCards ? CMD.CREATE_CARDS : CMD.RESET,
          state: { ...base, boardCols },
          resetCalled: true,
        });
        currentState = { ...result, boardCols };
        return currentState;
      });
      return sendJson(res, 200, game);
    }

    return sendJson(res, 404, { error: "Endpoint não encontrado." });
  } catch (err) {
    return sendJson(res, 500, {
      error: err instanceof Error ? err.message : "Erro ao executar motor C.",
    });
  }
}
