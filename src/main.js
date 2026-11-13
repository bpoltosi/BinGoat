import "./style.css";
import { createSoundEngine } from "./audio/soundEngine.js";
import { createBingoBoard } from "./components/BingoBoard.js";
import { createBingoCardsPanel } from "./components/BingoCardsPanel.js";
import { createBingoHallCanvas } from "./components/BingoHallCanvas.js";
import { apiRequest } from "./services/api.js";

const MAX_BINGO_NUMBER = 60;
const formatBingoNumber = (value) => String(value).padStart(2, "0");

const state = {
  calledNumbers: [],
  boardLines: [],
  players: [],
  autoMark: true,
  filter: "all",
  loading: true,
  errorMessage: "",
};

const app = document.querySelector("#app");
app.innerHTML = `
  <div class="page-shell">
    <header class="topbar">
      <a class="brand" href="#salao" aria-label="BinGoat — início">
        <span class="brand-mark" aria-hidden="true">B</span>
        <span>BinGoat</span>
      </a>

      <nav class="topnav" aria-label="Navegação principal">
        <a class="nav-link" href="#salao">Salão</a>
        <a class="nav-link" href="#tabuleiro">Tabuleiro</a>
        <a class="nav-link" href="#cartelas">Cartelas</a>
      </nav>

      <div class="topbar-actions">
        <button type="button" id="new-game-btn" class="primary-button">
          Nova partida
        </button>
      </div>
    </header>

    <main class="main-content">
      <section class="caller-bar" aria-labelledby="page-title">
        <div class="caller-intro">
          <h1 id="page-title">Salão de Bingo</h1>
          <p class="caller-subtitle">
            Gire a roleta na mesa do sorteador ou marque os números no tabuleiro 01–60.
          </p>
        </div>

        <div class="caller-controls">
          <div class="latest-call" aria-live="polite">
            <span class="latest-call-label">Telão atual</span>
            <strong id="latest-number-display" class="latest-call-value tabular-nums">--</strong>
          </div>

          <div class="caller-buttons">
            <button type="button" id="spin-roulette-btn" class="primary-button">
              Girar roleta
            </button>
            <button type="button" id="undo-number-btn" class="secondary-button">
              Desfazer
            </button>
            <button type="button" id="clear-board-btn" class="secondary-button">
              Limpar
            </button>
            <button
              type="button"
              id="sound-toggle-btn"
              class="secondary-button"
              aria-pressed="true"
            >
              Som: ligado
            </button>
          </div>
        </div>
      </section>

      <div id="api-error-banner" class="winner-banner is-error" hidden aria-live="assertive"></div>
      <div id="winner-banner" class="winner-banner" hidden aria-live="polite"></div>

      <div id="hall-mount"></div>

      <section class="history-strip" aria-labelledby="history-heading">
        <div class="history-header">
          <h2 id="history-heading" class="history-title">Sequência de chamados</h2>
          <span id="round-counter" class="history-counter tabular-nums">Rodada 0 de 60</span>
        </div>
        <div id="recent-calls-list" class="recent-calls-list" aria-live="polite"></div>
      </section>

      <div class="workspace-grid">
        <div id="board-mount" class="workspace-primary"></div>
        <div id="cards-mount" class="workspace-secondary"></div>
      </div>
    </main>

    <footer class="footer">
      <span>BinGoat · Motor em C (TrabalhoBingo.c) · Cartelas 3 × 5</span>
      <span><a class="footer-link" href="#salao">Voltar ao topo</a></span>
    </footer>

    <aside
      id="line-toast"
      class="line-toast"
      role="status"
      aria-live="polite"
      aria-atomic="true"
      hidden
    >
      <div class="line-toast-body">
        <strong id="line-toast-title" class="line-toast-title"></strong>
        <p id="line-toast-detail" class="line-toast-detail tabular-nums"></p>
      </div>
      <button
        type="button"
        id="line-toast-close"
        class="line-toast-close"
        aria-label="Fechar notificação"
      >
        Fechar
      </button>
    </aside>
  </div>
`;

const soundEngine = createSoundEngine();
const latestNumberDisplay = document.querySelector("#latest-number-display");
const roundCounter = document.querySelector("#round-counter");
const recentCallsList = document.querySelector("#recent-calls-list");
const winnerBanner = document.querySelector("#winner-banner");
const apiErrorBanner = document.querySelector("#api-error-banner");
const spinBtn = document.querySelector("#spin-roulette-btn");
const undoBtn = document.querySelector("#undo-number-btn");
const clearBtn = document.querySelector("#clear-board-btn");
const soundToggleBtn = document.querySelector("#sound-toggle-btn");
const newGameBtn = document.querySelector("#new-game-btn");
const lineToast = document.querySelector("#line-toast");
const lineToastTitle = document.querySelector("#line-toast-title");
const lineToastDetail = document.querySelector("#line-toast-detail");
const lineToastCloseBtn = document.querySelector("#line-toast-close");

const knownLineKeys = new Set();
const celebratingLineKeys = new Set();
const knownWinnerIds = new Set();
let toastTimeoutId = null;
let celebrationTimeoutId = null;
let requestBusy = false;

function hideLineToast() {
  if (toastTimeoutId !== null) {
    window.clearTimeout(toastTimeoutId);
    toastTimeoutId = null;
  }
  lineToast.classList.remove("is-visible");
  lineToast.hidden = true;
}

lineToastCloseBtn.addEventListener("click", hideLineToast);

function orientationHeading(orientation) {
  if (orientation === "horizontal") return "Linha horizontal completa!";
  if (orientation === "vertical") return "Linha vertical completa!";
  return "Linha diagonal completa!";
}

function showLineToast(newCompletions) {
  if (newCompletions.length === 0) return;

  if (toastTimeoutId !== null) {
    window.clearTimeout(toastTimeoutId);
  }

  const primary = newCompletions[0];
  lineToastTitle.textContent =
    newCompletions.length === 1
      ? orientationHeading(primary.orientation)
      : `${newCompletions.length} linhas completadas!`;

  const numsPreview = primary.numbers.map(formatBingoNumber).join(" · ");
  const extra =
    newCompletions.length > 1
      ? ` (+${newCompletions.length - 1} ${newCompletions.length - 1 === 1 ? "outra" : "outras"})`
      : "";
  lineToastDetail.textContent = `${primary.source} · ${primary.label} (${numsPreview})${extra}`;

  lineToast.dataset.orientation = primary.orientation;
  lineToast.hidden = false;
  window.requestAnimationFrame(() => {
    lineToast.classList.add("is-visible");
  });

  toastTimeoutId = window.setTimeout(() => {
    hideLineToast();
  }, 4800);
}

const hallComponent = createBingoHallCanvas(
  document.querySelector("#hall-mount"),
  {
    soundEngine,
    onSpinRequest() {
      triggerRouletteDraw();
    },
    onSelectPlayer(playerId) {
      const cardEl = document.querySelector(
        `[data-card-player-id="${playerId}"]`
      );
      if (!cardEl) return;
      cardEl.scrollIntoView({ behavior: "smooth", block: "nearest" });
      cardEl.classList.add("is-highlighted");
      window.setTimeout(() => {
        cardEl.classList.remove("is-highlighted");
      }, 900);
    },
  }
);

const boardComponent = createBingoBoard(document.querySelector("#board-mount"), {
  async onToggleNumber(num) {
    if (requestBusy || hallComponent.isSpinning()) return;
    soundEngine.unlock();
    const wasCalled = state.calledNumbers.includes(num);
    await runApiAction(async () => {
      const nextGame = await apiRequest("/games/current/toggle-board", {
        method: "POST",
        body: JSON.stringify({
          number: num,
          boardCols: boardComponent.getColumnCount(),
          autoMark: state.autoMark,
        }),
      });
      soundEngine.playBoardMark(!wasCalled);
      applyServerState(nextGame, { triggerNotifications: true });
    });
  },
  onFilterChange(nextFilter) {
    state.filter = nextFilter;
    renderUI();
  },
});

const cardsComponent = createBingoCardsPanel(document.querySelector("#cards-mount"), {
  async onPlayerCountChange(nextCount) {
    if (requestBusy || hallComponent.isSpinning()) return;
    await runApiAction(async () => {
      const nextGame = await apiRequest("/games/current/sync", {
        method: "POST",
        body: JSON.stringify({
          playerCount: nextCount,
          boardCols: boardComponent.getColumnCount(),
          autoMark: state.autoMark,
        }),
      });
      applyServerState(nextGame, {
        triggerNotifications: false,
        resetKnownLines: true,
        resetHallReactions: true,
      });
    });
  },
  async onPlayerNameChange(playerId, rawName) {
    const names = state.players.map((p) =>
      p.id === playerId ? rawName.trim() || `Jogador ${playerId}` : p.name
    );
    await runApiAction(async () => {
      const nextGame = await apiRequest("/games/current/sync", {
        method: "POST",
        body: JSON.stringify({
          playerNames: names,
          boardCols: boardComponent.getColumnCount(),
          autoMark: state.autoMark,
        }),
      });
      applyServerState(nextGame, { triggerNotifications: false });
    });
  },
  async onToggleCardCell(playerId, num) {
    if (requestBusy || hallComponent.isSpinning()) return;
    soundEngine.unlock();
    const targetPlayer = state.players.find((p) => p.id === playerId);
    const wasMarked = targetPlayer?.markedNumbers?.has(num) ?? false;

    await runApiAction(async () => {
      const nextGame = await apiRequest("/games/current/toggle-card", {
        method: "POST",
        body: JSON.stringify({
          playerId,
          number: num,
          boardCols: boardComponent.getColumnCount(),
        }),
      });
      soundEngine.playBoardMark(!wasMarked);
      applyServerState(nextGame, { triggerNotifications: true });
    });
  },
  async onToggleAutoMark() {
    if (requestBusy || hallComponent.isSpinning()) return;
    await runApiAction(async () => {
      const nextGame = await apiRequest("/games/current/sync", {
        method: "POST",
        body: JSON.stringify({
          autoMark: !state.autoMark,
          boardCols: boardComponent.getColumnCount(),
        }),
      });
      applyServerState(nextGame, { triggerNotifications: false });
    });
  },
  async onRegenerateCards() {
    if (requestBusy || hallComponent.isSpinning()) return;
    soundEngine.unlock();
    await runApiAction(async () => {
      const nextGame = await apiRequest("/games/current/sync", {
        method: "POST",
        body: JSON.stringify({
          regenerateCards: true,
          boardCols: boardComponent.getColumnCount(),
          autoMark: state.autoMark,
        }),
      });
      soundEngine.playReset();
      applyServerState(nextGame, {
        triggerNotifications: false,
        resetKnownLines: true,
        resetHallReactions: true,
      });
    });
  },
});

let lastBoardCols = boardComponent.getColumnCount();

async function runApiAction(actionFn) {
  requestBusy = true;
  state.errorMessage = "";
  renderHistoryStrip();
  try {
    await actionFn();
  } catch (err) {
    state.errorMessage =
      err instanceof Error ? err.message : "Falha ao comunicar com TrabalhoBingo.c.";
    renderUI();
  } finally {
    requestBusy = false;
    renderHistoryStrip();
  }
}

function updateSoundToggleUI() {
  const muted = soundEngine.isMuted();
  soundToggleBtn.setAttribute("aria-pressed", String(!muted));
  soundToggleBtn.classList.toggle("is-active", !muted);
  soundToggleBtn.textContent = muted ? "Som: mudo" : "Som: ligado";
}

soundToggleBtn.addEventListener("click", () => {
  soundEngine.toggleMute();
  updateSoundToggleUI();
  if (!soundEngine.isMuted()) {
    soundEngine.playBoardMark(true);
  }
});

async function triggerRouletteDraw() {
  if (requestBusy || hallComponent.isSpinning()) return;
  if (state.calledNumbers.length >= MAX_BINGO_NUMBER) return;

  soundEngine.unlock();
  requestBusy = true;
  state.errorMessage = "";
  renderHistoryStrip();
  newGameBtn.disabled = true;

  try {
    const nextGame = await apiRequest("/games/current/draw", {
      method: "POST",
      body: JSON.stringify({
        boardCols: boardComponent.getColumnCount(),
        autoMark: state.autoMark,
      }),
    });

    if (!nextGame.drawnNumber) {
      requestBusy = false;
      newGameBtn.disabled = false;
      applyServerState(nextGame, { triggerNotifications: false });
      return;
    }

    hallComponent.spinTo(nextGame.drawnNumber, () => {
      requestBusy = false;
      newGameBtn.disabled = false;
      applyServerState(nextGame, { triggerNotifications: true });
    });
  } catch (err) {
    requestBusy = false;
    newGameBtn.disabled = false;
    state.errorMessage =
      err instanceof Error ? err.message : "Erro ao sortear número em C.";
    renderUI();
  }
}

spinBtn.addEventListener("click", () => {
  triggerRouletteDraw();
});

undoBtn.addEventListener("click", async () => {
  if (requestBusy || hallComponent.isSpinning()) return;
  if (state.calledNumbers.length === 0) return;
  soundEngine.unlock();
  await runApiAction(async () => {
    const nextGame = await apiRequest("/games/current/undo", {
      method: "POST",
      body: JSON.stringify({
        boardCols: boardComponent.getColumnCount(),
      }),
    });
    soundEngine.playUndo();
    applyServerState(nextGame, { triggerNotifications: false });
  });
});

clearBtn.addEventListener("click", async () => {
  if (requestBusy || hallComponent.isSpinning()) return;
  if (
    state.calledNumbers.length === 0 &&
    state.players.every((p) => p.markedNumbers.size === 0)
  ) {
    return;
  }
  soundEngine.unlock();
  await runApiAction(async () => {
    const nextGame = await apiRequest("/games/current/reset", {
      method: "POST",
      body: JSON.stringify({
        newCards: false,
        boardCols: boardComponent.getColumnCount(),
      }),
    });
    soundEngine.playReset();
    hideLineToast();
    applyServerState(nextGame, {
      triggerNotifications: false,
      resetKnownLines: true,
      resetHallReactions: true,
    });
  });
});

newGameBtn.addEventListener("click", async () => {
  if (requestBusy || hallComponent.isSpinning()) return;
  soundEngine.unlock();
  await runApiAction(async () => {
    const nextGame = await apiRequest("/games/current/reset", {
      method: "POST",
      body: JSON.stringify({
        newCards: true,
        boardCols: boardComponent.getColumnCount(),
      }),
    });
    state.filter = "all";
    soundEngine.playReset();
    hideLineToast();
    applyServerState(nextGame, {
      triggerNotifications: false,
      resetKnownLines: true,
      resetHallReactions: true,
    });
  });
});

window.addEventListener("resize", () => {
  const nextCols = boardComponent.getColumnCount();
  if (nextCols !== lastBoardCols) {
    lastBoardCols = nextCols;
    apiRequest(`/games/current?boardCols=${nextCols}`)
      .then((nextGame) => {
        applyServerState(nextGame, {
          triggerNotifications: false,
          resetKnownLines: true,
        });
      })
      .catch(() => {});
  }
});

function applyServerState(
  serverGame,
  {
    triggerNotifications = true,
    resetKnownLines = false,
    resetHallReactions = false,
  } = {}
) {
  state.loading = false;
  state.calledNumbers = Array.isArray(serverGame.calledNumbers)
    ? serverGame.calledNumbers
    : [];
  state.boardLines = Array.isArray(serverGame.boardLines)
    ? serverGame.boardLines
    : [];
  state.autoMark =
    typeof serverGame.autoMark === "boolean" ? serverGame.autoMark : true;

  state.players = Array.isArray(serverGame.players)
    ? serverGame.players.map((p) => ({
        id: p.id,
        name: p.name,
        numbers: Array.isArray(p.numbers) ? p.numbers : [],
        markedNumbers: new Set(
          Array.isArray(p.markedNumbers) ? p.markedNumbers : []
        ),
        wonRound: Number.isInteger(p.wonRound) ? p.wonRound : null,
        lines: Array.isArray(p.lines) ? p.lines : [],
      }))
    : [];

  const allActiveLines = state.boardLines.map((line) => ({
    ...line,
    source: "Tabuleiro",
  }));

  for (const player of state.players) {
    for (const line of player.lines) {
      allActiveLines.push({
        ...line,
        source: player.name,
      });
    }
  }

  const activeKeys = new Set(allActiveLines.map((line) => line.key));

  if (resetKnownLines) {
    knownLineKeys.clear();
    celebratingLineKeys.clear();
  }

  for (const key of Array.from(knownLineKeys)) {
    if (!activeKeys.has(key)) {
      knownLineKeys.delete(key);
      celebratingLineKeys.delete(key);
    }
  }

  const newCompletions = [];
  for (const line of allActiveLines) {
    if (!knownLineKeys.has(line.key)) {
      knownLineKeys.add(line.key);
      if (triggerNotifications) {
        newCompletions.push(line);
        celebratingLineKeys.add(line.key);
      }
    }
  }

  let newlyWon = false;
  for (const id of Array.from(knownWinnerIds)) {
    const p = state.players.find((pl) => pl.id === id);
    if (!p || p.wonRound === null) {
      knownWinnerIds.delete(id);
    }
  }
  for (const p of state.players) {
    if (p.wonRound !== null && !knownWinnerIds.has(p.id)) {
      knownWinnerIds.add(p.id);
      if (triggerNotifications) {
        newlyWon = true;
      }
    }
  }

  if (newCompletions.length > 0) {
    showLineToast(newCompletions);
    if (celebrationTimeoutId !== null) {
      window.clearTimeout(celebrationTimeoutId);
    }
    celebrationTimeoutId = window.setTimeout(() => {
      celebratingLineKeys.clear();
      renderUI();
    }, 1400);
  }

  if (triggerNotifications) {
    if (newlyWon) {
      soundEngine.playBingoWin();
    } else if (newCompletions.length > 0) {
      soundEngine.playLineComplete(newCompletions[0].orientation);
    }
  }

  renderUI({ resetHallReactions });
}

function renderHistoryStrip() {
  const count = state.calledNumbers.length;
  const latest = count > 0 ? state.calledNumbers[count - 1] : null;
  const isSpinning = hallComponent.isSpinning() || requestBusy;

  latestNumberDisplay.textContent = latest !== null ? formatBingoNumber(latest) : "--";
  latestNumberDisplay.classList.toggle("has-value", latest !== null);

  roundCounter.textContent = `Rodada ${count} de ${MAX_BINGO_NUMBER}`;

  spinBtn.disabled = state.loading || isSpinning || count >= MAX_BINGO_NUMBER;
  spinBtn.textContent = hallComponent.isSpinning()
    ? "Girando..."
    : count >= MAX_BINGO_NUMBER
      ? "Todos sorteados"
      : "Girar roleta";
  undoBtn.disabled = state.loading || isSpinning || count === 0;
  clearBtn.disabled =
    state.loading ||
    isSpinning ||
    (count === 0 && state.players.every((p) => p.markedNumbers.size === 0));

  if (count === 0) {
    recentCallsList.innerHTML =
      '<span class="history-empty">Nenhum número chamado nesta partida.</span>';
    return;
  }

  const recent = state.calledNumbers.slice(-12).reverse();
  const fragment = document.createDocumentFragment();

  recent.forEach((num, idx) => {
    const item = document.createElement("span");
    item.className = "history-number tabular-nums";
    if (idx === 0) {
      item.classList.add("is-latest");
    }
    item.textContent = formatBingoNumber(num);
    fragment.appendChild(item);
  });

  if (count > 12) {
    const more = document.createElement("span");
    more.className = "history-overflow tabular-nums";
    more.textContent = `+${count - 12} anteriores`;
    fragment.appendChild(more);
  }

  recentCallsList.replaceChildren(fragment);
}

function renderWinnerBanner() {
  if (state.errorMessage) {
    apiErrorBanner.hidden = false;
    apiErrorBanner.textContent = state.errorMessage;
  } else {
    apiErrorBanner.hidden = true;
    apiErrorBanner.textContent = "";
  }

  const winners = state.players
    .filter((p) => p.wonRound !== null)
    .sort((a, b) => (a.wonRound ?? 0) - (b.wonRound ?? 0));

  if (winners.length === 0) {
    winnerBanner.hidden = true;
    winnerBanner.textContent = "";
    return;
  }

  winnerBanner.hidden = false;
  const list = winners
    .map((w) => `${w.name} (rodada ${w.wonRound})`)
    .join(" · ");
  winnerBanner.innerHTML = `
    <strong>Bingo completo!</strong>
    <span>${list}</span>
  `;
}

function renderUI({ resetHallReactions = false } = {}) {
  const latestNumber =
    state.calledNumbers.length > 0
      ? state.calledNumbers[state.calledNumbers.length - 1]
      : null;

  const cardLinesByPlayer = new Map();
  for (const player of state.players) {
    cardLinesByPlayer.set(player.id, player.lines || []);
  }

  updateSoundToggleUI();
  renderHistoryStrip();
  renderWinnerBanner();
  hallComponent.syncState({
    players: state.players,
    calledNumbers: state.calledNumbers,
    latest: latestNumber,
    resetReactions: resetHallReactions,
  });
  boardComponent.update({
    calledNumbers: state.calledNumbers,
    filter: state.filter,
    completedLines: state.boardLines,
    celebratingLineKeys,
  });
  cardsComponent.update({
    players: state.players,
    calledNumbers: state.calledNumbers,
    autoMark: state.autoMark,
    cardLinesByPlayer,
    celebratingLineKeys,
  });
}

renderUI({ resetHallReactions: true });

runApiAction(async () => {
  const initialGame = await apiRequest(
    `/games/current?boardCols=${boardComponent.getColumnCount()}`
  );
  applyServerState(initialGame, {
    triggerNotifications: false,
    resetKnownLines: true,
    resetHallReactions: true,
  });
});
