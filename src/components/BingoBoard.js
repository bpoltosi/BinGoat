const MAX_BINGO_NUMBER = 60;
const formatBingoNumber = (value) => String(value).padStart(2, "0");

const CHECK_ICON_SVG = `
  <svg class="cell-check-icon" viewBox="0 0 16 16" fill="none" aria-hidden="true">
    <path d="M3.5 8.5L6.5 11.5L12.5 4.5" stroke="currentColor" stroke-width="2.1" stroke-linecap="round" stroke-linejoin="round" />
  </svg>
`;

/**
 * Componente de tabuleiro responsivo (grade 1 a 60) que exibe todos os números
 * do bingo e permite ao usuário marcar ou desmarcar números chamados.
 */
export function createBingoBoard(container, callbacks) {
  const { onToggleNumber, onFilterChange } = callbacks;

  let currentFilter = "all";

  const root = document.createElement("section");
  root.className = "board-panel";
  root.id = "tabuleiro";
  root.setAttribute("aria-labelledby", "board-heading");

  const header = document.createElement("div");
  header.className = "panel-header";

  const titleGroup = document.createElement("div");
  titleGroup.className = "panel-title-group";

  const heading = document.createElement("h2");
  heading.id = "board-heading";
  heading.textContent = "Tabuleiro de números";

  const summary = document.createElement("p");
  summary.className = "panel-meta tabular-nums";
  summary.setAttribute("aria-live", "polite");

  titleGroup.append(heading, summary);

  const filterBar = document.createElement("div");
  filterBar.className = "segmented-control";
  filterBar.setAttribute("role", "group");
  filterBar.setAttribute("aria-label", "Filtrar números do tabuleiro");

  const filterOptions = [
    { id: "all", label: "Todos" },
    { id: "called", label: "Chamados" },
    { id: "remaining", label: "Restantes" },
  ];

  const filterButtons = new Map();

  for (const option of filterOptions) {
    const btn = document.createElement("button");
    btn.type = "button";
    btn.className = "segmented-button";
    btn.textContent = option.label;
    btn.dataset.filter = option.id;
    btn.setAttribute("aria-pressed", String(option.id === currentFilter));
    btn.addEventListener("click", () => {
      currentFilter = option.id;
      onFilterChange?.(option.id);
    });
    filterButtons.set(option.id, btn);
    filterBar.appendChild(btn);
  }

  header.append(titleGroup, filterBar);

  const legend = document.createElement("div");
  legend.className = "board-legend";
  legend.innerHTML = `
    <span class="legend-item">
      <span class="legend-swatch legend-swatch-idle" aria-hidden="true"></span>
      <span>Disponível</span>
    </span>
    <span class="legend-item">
      <span class="legend-swatch legend-swatch-called" aria-hidden="true">${CHECK_ICON_SVG}</span>
      <span>Chamado</span>
    </span>
    <span class="legend-item">
      <span class="legend-swatch legend-swatch-latest" aria-hidden="true"></span>
      <span>Último sorteado</span>
    </span>
  `;

  const grid = document.createElement("div");
  grid.className = "bingo-board-grid";
  grid.setAttribute("role", "grid");
  grid.setAttribute("aria-label", "Grade de números do bingo de 1 a 60");

  const emptyFilterNotice = document.createElement("p");
  emptyFilterNotice.className = "empty-filter-notice";
  emptyFilterNotice.hidden = true;

  const cellButtons = new Map();

  for (let num = 1; num <= MAX_BINGO_NUMBER; num++) {
    const button = document.createElement("button");
    button.type = "button";
    button.className = "board-cell";
    button.dataset.number = String(num);
    button.setAttribute("aria-pressed", "false");
    button.setAttribute("aria-label", `Número ${num}, disponível`);

    const numberSpan = document.createElement("span");
    numberSpan.className = "board-cell-number tabular-nums";
    numberSpan.textContent = formatBingoNumber(num);

    const stateIndicator = document.createElement("span");
    stateIndicator.className = "board-cell-state";
    stateIndicator.setAttribute("aria-hidden", "true");
    stateIndicator.innerHTML = CHECK_ICON_SVG;

    button.append(numberSpan, stateIndicator);

    button.addEventListener("click", () => {
      onToggleNumber(num);
    });

    button.addEventListener("keydown", (event) => {
      handleGridKeydown(event, num, grid);
    });

    cellButtons.set(num, button);
    grid.appendChild(button);
  }

  root.append(header, legend, grid, emptyFilterNotice);
  container.replaceChildren(root);

  function getColumnCount() {
    if (window.matchMedia("(max-width: 560px)").matches) return 5;
    if (window.matchMedia("(max-width: 780px)").matches) return 6;
    return 10;
  }

  function handleGridKeydown(event, num, gridEl) {
    const style = window.getComputedStyle(gridEl);
    const cols =
      style.gridTemplateColumns.split(" ").filter(Boolean).length ||
      getColumnCount();

    let nextNum = null;
    if (event.key === "ArrowRight") nextNum = num + 1;
    else if (event.key === "ArrowLeft") nextNum = num - 1;
    else if (event.key === "ArrowDown") nextNum = num + cols;
    else if (event.key === "ArrowUp") nextNum = num - cols;
    else if (event.key === "Home") nextNum = 1;
    else if (event.key === "End") nextNum = MAX_BINGO_NUMBER;

    if (nextNum === null) return;

    const clamped = Math.max(1, Math.min(MAX_BINGO_NUMBER, nextNum));
    const target = cellButtons.get(clamped);
    if (target && !target.hidden) {
      event.preventDefault();
      target.focus();
    }
  }

  function update({
    calledNumbers,
    filter = currentFilter,
    completedLines = [],
    celebratingLineKeys = new Set(),
  }) {
    currentFilter = filter;
    const calledSet = new Set(calledNumbers);
    const latestNumber =
      calledNumbers.length > 0 ? calledNumbers[calledNumbers.length - 1] : null;
    const calledCount = calledSet.size;
    const remainingCount = MAX_BINGO_NUMBER - calledCount;

    const lineNumberSet = new Set();
    const celebratingOrder = new Map();

    for (const line of completedLines) {
      const isCelebrating = celebratingLineKeys.has(line.key);
      line.numbers.forEach((n, idx) => {
        lineNumberSet.add(n);
        if (isCelebrating && !celebratingOrder.has(n)) {
          celebratingOrder.set(n, idx);
        }
      });
    }

    const linesSummary =
      completedLines.length > 0
        ? ` · ${completedLines.length} ${completedLines.length === 1 ? "linha completa" : "linhas completas"}`
        : "";

    summary.textContent = `${calledCount} de ${MAX_BINGO_NUMBER} chamados · ${remainingCount} restantes${linesSummary}`;

    for (const [id, btn] of filterButtons.entries()) {
      const active = id === currentFilter;
      btn.classList.toggle("is-active", active);
      btn.setAttribute("aria-pressed", String(active));
    }

    let visibleCount = 0;

    for (let num = 1; num <= MAX_BINGO_NUMBER; num++) {
      const btn = cellButtons.get(num);
      const isCalled = calledSet.has(num);
      const isLatest = num === latestNumber;
      const isInLine = lineNumberSet.has(num);
      const isCelebrating = celebratingOrder.has(num);

      const shouldShow =
        currentFilter === "all" ||
        (currentFilter === "called" && isCalled) ||
        (currentFilter === "remaining" && !isCalled);

      btn.hidden = !shouldShow;
      if (shouldShow) visibleCount++;

      btn.classList.toggle("is-called", isCalled);
      btn.classList.toggle("is-latest", isLatest);
      btn.classList.toggle("is-in-line", isInLine);
      btn.classList.toggle("is-line-celebrating", isCelebrating);

      if (isCelebrating) {
        btn.style.setProperty("--line-order", String(celebratingOrder.get(num)));
      } else {
        btn.style.removeProperty("--line-order");
      }

      btn.setAttribute("aria-pressed", String(isCalled));

      const statusParts = [];
      if (isLatest) statusParts.push("último número chamado");
      else if (isCalled) statusParts.push("chamado");
      else statusParts.push("disponível");
      if (isInLine) statusParts.push("linha completa");
      btn.setAttribute("aria-label", `Número ${num}, ${statusParts.join(", ")}`);
    }

    if (visibleCount === 0) {
      emptyFilterNotice.hidden = false;
      emptyFilterNotice.textContent =
        currentFilter === "called"
          ? "Nenhum número foi marcado como chamado ainda. Clique em qualquer número em Todos ou use Sortear número."
          : "Todos os 60 números já foram chamados.";
    } else {
      emptyFilterNotice.hidden = true;
    }
  }

  return { update, getColumnCount };
}
