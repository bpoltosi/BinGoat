const CARD_NUMBER_COUNT = 15;
const MIN_PLAYERS = 2;
const MAX_PLAYERS = 5;
const formatBingoNumber = (value) => String(value).padStart(2, "0");

/**
 * Painel responsivo de cartelas dos jogadores (formato 3x5 de TrabalhoBingo.c).
 * Permite marcar números manualmente nas cartelas ou sincronizar automaticamente
 * com os números chamados no tabuleiro principal.
 */
export function createBingoCardsPanel(container, callbacks) {
  const {
    onPlayerCountChange,
    onPlayerNameChange,
    onToggleCardCell,
    onToggleAutoMark,
    onRegenerateCards,
  } = callbacks;

  const section = document.createElement("section");
  section.className = "cards-panel";
  section.id = "cartelas";
  section.setAttribute("aria-labelledby", "cards-heading");

  const header = document.createElement("div");
  header.className = "panel-header";

  const titleGroup = document.createElement("div");
  titleGroup.className = "panel-title-group";

  const heading = document.createElement("h2");
  heading.id = "cards-heading";
  heading.textContent = "Cartelas dos jogadores";

  const subtitle = document.createElement("p");
  subtitle.className = "panel-meta";
  subtitle.textContent = "Grade 3 × 5 · 15 números não consecutivos por cartela";

  titleGroup.append(heading, subtitle);

  const toolbar = document.createElement("div");
  toolbar.className = "cards-toolbar";

  const playerCountLabel = document.createElement("label");
  playerCountLabel.className = "select-field";
  playerCountLabel.innerHTML = `<span class="field-label">Jogadores</span>`;

  const playerCountSelect = document.createElement("select");
  playerCountSelect.className = "select-input";
  playerCountSelect.setAttribute("aria-label", "Quantidade de jogadores");
  for (let i = MIN_PLAYERS; i <= MAX_PLAYERS; i++) {
    const opt = document.createElement("option");
    opt.value = String(i);
    opt.textContent = `${i} jogadores`;
    playerCountSelect.appendChild(opt);
  }
  playerCountSelect.addEventListener("change", (event) => {
    onPlayerCountChange(Number(event.target.value));
  });
  playerCountLabel.appendChild(playerCountSelect);

  const autoMarkBtn = document.createElement("button");
  autoMarkBtn.type = "button";
  autoMarkBtn.className = "secondary-button";
  autoMarkBtn.addEventListener("click", () => {
    onToggleAutoMark();
  });

  const regenerateBtn = document.createElement("button");
  regenerateBtn.type = "button";
  regenerateBtn.className = "secondary-button";
  regenerateBtn.textContent = "Gerar novas cartelas";
  regenerateBtn.addEventListener("click", () => {
    onRegenerateCards();
  });

  toolbar.append(playerCountLabel, autoMarkBtn, regenerateBtn);
  header.append(titleGroup, toolbar);

  const cardsGrid = document.createElement("div");
  cardsGrid.className = "player-cards-grid";

  section.append(header, cardsGrid);
  container.replaceChildren(section);

  function update({
    players,
    calledNumbers,
    autoMark,
    cardLinesByPlayer = new Map(),
    celebratingLineKeys = new Set(),
  }) {
    const calledSet = new Set(calledNumbers);

    playerCountSelect.value = String(
      Math.max(MIN_PLAYERS, Math.min(MAX_PLAYERS, players.length || MIN_PLAYERS))
    );

    autoMarkBtn.setAttribute("aria-pressed", String(autoMark));
    autoMarkBtn.classList.toggle("is-active", autoMark);
    autoMarkBtn.textContent = autoMark
      ? "Marcação automática: ligada"
      : "Marcação automática: desligada";

    // Preserva foco se o usuário estiver editando um nome ou navegando nas células
    const activeEl = document.activeElement;
    const activePlayerId = activeEl?.dataset?.playerId;
    const activeCellNumber = activeEl?.dataset?.cardNumber;
    const activeFieldType = activeEl?.dataset?.fieldType;

    const fragment = document.createDocumentFragment();

    for (const player of players) {
      const effectiveMarked = new Set(player.markedNumbers);
      if (autoMark) {
        for (const num of player.numbers) {
          if (calledSet.has(num)) {
            effectiveMarked.add(num);
          }
        }
      }

      const markedCount = player.numbers.filter((num) =>
        effectiveMarked.has(num)
      ).length;
      const hasWon = markedCount === CARD_NUMBER_COUNT;

      const playerLines = cardLinesByPlayer.get(player.id) || [];
      const lineNumberSet = new Set();
      const celebratingOrder = new Map();

      for (const line of playerLines) {
        const isCelebrating = celebratingLineKeys.has(line.key);
        line.numbers.forEach((n, idx) => {
          lineNumberSet.add(n);
          if (isCelebrating && !celebratingOrder.has(n)) {
            celebratingOrder.set(n, idx);
          }
        });
      }

      const card = document.createElement("article");
      card.className = "player-card";
      card.dataset.cardPlayerId = String(player.id);
      if (hasWon) {
        card.classList.add("is-winner");
      } else if (playerLines.length > 0) {
        card.classList.add("has-line");
      }

      const cardHeader = document.createElement("div");
      cardHeader.className = "player-card-header";

      const nameInput = document.createElement("input");
      nameInput.type = "text";
      nameInput.className = "player-name-input";
      nameInput.value = player.name;
      nameInput.maxLength = 32;
      nameInput.setAttribute("aria-label", `Nome do Jogador ${player.id}`);
      nameInput.dataset.playerId = String(player.id);
      nameInput.dataset.fieldType = "name";
      nameInput.addEventListener("change", (event) => {
        onPlayerNameChange(player.id, event.target.value);
      });

      const progress = document.createElement("span");
      progress.className = "player-progress tabular-nums";
      if (hasWon) {
        progress.textContent = player.wonRound
          ? `Bingo! · Rodada ${player.wonRound}`
          : "Bingo! · 15/15";
      } else if (playerLines.length > 0) {
        progress.textContent = `${markedCount}/${CARD_NUMBER_COUNT} · ${playerLines.length} ${playerLines.length === 1 ? "linha" : "linhas"}`;
      } else {
        progress.textContent = `${markedCount}/${CARD_NUMBER_COUNT} marcados`;
      }

      cardHeader.append(nameInput, progress);

      const grid = document.createElement("div");
      grid.className = "player-card-grid";
      grid.setAttribute("role", "group");
      grid.setAttribute("aria-label", `Cartela de ${player.name}`);

      for (const num of player.numbers) {
        const isCalledOnBoard = calledSet.has(num);
        const isMarked = effectiveMarked.has(num);
        const isInLine = lineNumberSet.has(num);
        const isCelebrating = celebratingOrder.has(num);

        const cell = document.createElement("button");
        cell.type = "button";
        cell.className = "card-cell";
        cell.dataset.playerId = String(player.id);
        cell.dataset.cardNumber = String(num);
        cell.dataset.fieldType = "cell";
        cell.setAttribute("aria-pressed", String(isMarked));

        if (isMarked) {
          cell.classList.add("is-marked");
        }
        if (isCalledOnBoard) {
          cell.classList.add("is-called-on-board");
        }
        if (isInLine) {
          cell.classList.add("is-in-line");
        }
        if (isCelebrating) {
          cell.classList.add("is-line-celebrating");
          cell.style.setProperty("--line-order", String(celebratingOrder.get(num)));
        }

        const stateLabel = isMarked
          ? isInLine
            ? "marcado, linha completa"
            : "marcado"
          : isCalledOnBoard
            ? "chamado no tabuleiro, clique para marcar"
            : "não chamado";
        cell.setAttribute(
          "aria-label",
          `Cartela de ${player.name}, número ${num}, ${stateLabel}`
        );

        const valueSpan = document.createElement("span");
        valueSpan.className = "card-cell-value tabular-nums";
        valueSpan.textContent = formatBingoNumber(num);

        const markIndicator = document.createElement("span");
        markIndicator.className = "card-cell-marker";
        markIndicator.setAttribute("aria-hidden", "true");
        markIndicator.textContent = isMarked
          ? "✓"
          : isCalledOnBoard
            ? "•"
            : "";

        cell.append(valueSpan, markIndicator);
        cell.addEventListener("click", () => {
          onToggleCardCell(player.id, num);
        });

        grid.appendChild(cell);
      }

      card.append(cardHeader, grid);
      fragment.appendChild(card);
    }

    cardsGrid.replaceChildren(fragment);

    if (activePlayerId && activeFieldType === "cell" && activeCellNumber) {
      const target = cardsGrid.querySelector(
        `button[data-player-id="${activePlayerId}"][data-card-number="${activeCellNumber}"]`
      );
      target?.focus();
    }
  }

  return { update };
}
