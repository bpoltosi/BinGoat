const HALL_WIDTH = 384;
const HALL_HEIGHT = 240;
const POCKET_COUNT = 12;
const POCKET_SPAN = (Math.PI * 2) / POCKET_COUNT;
const POINTER_ANGLE = -Math.PI / 2;
const WHEEL_SIZE = 46;
const WHEEL_CENTER = 23;
const WHEEL_CX = 192;
const WHEEL_CY = 109;

const formatBingoNumber = (value) => String(value).padStart(2, "0");

const PIXEL_FONT_4X6 = {
  "0": ["1111", "1001", "1001", "1001", "1001", "1111"],
  "1": ["0110", "1110", "0110", "0110", "0110", "1111"],
  "2": ["1111", "0001", "1111", "1000", "1000", "1111"],
  "3": ["1111", "0001", "0111", "0001", "0001", "1111"],
  "4": ["1001", "1001", "1111", "0001", "0001", "0001"],
  "5": ["1111", "1000", "1111", "0001", "0001", "1111"],
  "6": ["1111", "1000", "1111", "1001", "1001", "1111"],
  "7": ["1111", "0001", "0010", "0100", "0100", "0100"],
  "8": ["1111", "1001", "1111", "1001", "1001", "1111"],
  "9": ["1111", "1001", "1111", "0001", "0001", "1111"],
  "-": ["0000", "0000", "1111", "0000", "0000", "0000"],
  "?": ["1111", "0001", "0111", "0100", "0000", "0100"],
  "A": ["0110", "1001", "1111", "1001", "1001", "1001"],
  "B": ["1110", "1001", "1110", "1001", "1001", "1110"],
  "C": ["0111", "1000", "1000", "1000", "1000", "0111"],
  "D": ["1110", "1001", "1001", "1001", "1001", "1110"],
  "E": ["1111", "1000", "1110", "1000", "1000", "1111"],
  "F": ["1111", "1000", "1110", "1000", "1000", "1000"],
  "G": ["0111", "1000", "1011", "1001", "1001", "0111"],
  "H": ["1001", "1001", "1111", "1001", "1001", "1001"],
  "I": ["1110", "0100", "0100", "0100", "0100", "1110"],
  "J": ["0011", "0001", "0001", "1001", "1001", "0110"],
  "K": ["1001", "1010", "1100", "1010", "1001", "1001"],
  "L": ["1000", "1000", "1000", "1000", "1000", "1111"],
  "M": ["1001", "1111", "1111", "1001", "1001", "1001"],
  "N": ["1001", "1101", "1011", "1001", "1001", "1001"],
  "O": ["0110", "1001", "1001", "1001", "1001", "0110"],
  "P": ["1110", "1001", "1110", "1000", "1000", "1000"],
  "Q": ["0110", "1001", "1001", "1011", "0110", "0001"],
  "R": ["1110", "1001", "1110", "1010", "1001", "1001"],
  "S": ["0111", "1000", "0110", "0001", "0001", "1110"],
  "T": ["1111", "0100", "0100", "0100", "0100", "0100"],
  "U": ["1001", "1001", "1001", "1001", "1001", "0110"],
  "V": ["1001", "1001", "1001", "1001", "0110", "0110"],
  "W": ["1001", "1001", "1001", "1111", "1111", "1001"],
  "X": ["1001", "1001", "0110", "0110", "1001", "1001"],
  "Y": ["1001", "1001", "0110", "0100", "0100", "0100"],
  "Z": ["1111", "0001", "0010", "0100", "1000", "1111"],
  "/": ["0001", "0010", "0010", "0100", "0100", "1000"],
  "!": ["0100", "0100", "0100", "0100", "0000", "0100"],
  "+": ["0000", "0100", "1110", "0100", "0000", "0000"],
  " ": ["0000", "0000", "0000", "0000", "0000", "0000"],
};

const PLAYER_THEMES = [
  { hair: "#3d2314", skin: "#f0c29e", shirt: "#3d8ee6", accent: "#8ec5ff" },
  { hair: "#d9922b", skin: "#f6cfb0", shirt: "#d95b50", accent: "#ffaba3" },
  { hair: "#1f242d", skin: "#c98b63", shirt: "#35a874", accent: "#8ee0b5" },
  { hair: "#7c3b2a", skin: "#e6b08a", shirt: "#c98b2e", accent: "#f0c475" },
  { hair: "#687a8c", skin: "#f2c6a6", shirt: "#8d63d4", accent: "#c6afff" },
];

const WEDGE_PALETTE = [
  [30, 122, 82],
  [31, 68, 92],
  [176, 62, 50],
  [196, 138, 46],
];

function easeOutQuart(t) {
  return 1 - Math.pow(1 - t, 4);
}

function easeOutCubic(t) {
  return 1 - Math.pow(1 - t, 3);
}

function normalizeAngle(angle) {
  const full = Math.PI * 2;
  return ((angle % full) + full) % full;
}

function getTableXPositions(count) {
  if (count <= 2) return [114, 270];
  if (count === 3) return [84, 192, 300];
  if (count === 4) return [64, 149, 235, 320];
  return [50, 121, 192, 263, 334];
}

/**
 * Salão de Bingo em Pixel Art 2D top-down inspirado em PixelAgentsHQ:
 * - Telão na parede com o último número e matriz LED de 01 a 60
 * - Mesa principal do personagem sorteador com roleta top-down giratória
 * - Mesas individuais para 2 a 5 jogadores com personagens animados
 */
export function createBingoHallCanvas(
  container,
  { onSpinRequest, onSelectPlayer, soundEngine }
) {
  const section = document.createElement("section");
  section.className = "hall-panel";
  section.id = "salao";
  section.setAttribute("aria-labelledby", "hall-heading");

  const header = document.createElement("div");
  header.className = "panel-header hall-header";

  const titleGroup = document.createElement("div");
  titleGroup.className = "panel-title-group";

  const heading = document.createElement("h2");
  heading.id = "hall-heading";
  heading.textContent = "Salão de Bingo";

  const subtitle = document.createElement("p");
  subtitle.className = "panel-meta tabular-nums";
  subtitle.setAttribute("aria-live", "polite");
  subtitle.textContent = "Telão na parede · Sorteador e mesas dos jogadores";

  titleGroup.append(heading, subtitle);

  const legend = document.createElement("div");
  legend.className = "hall-status-readout tabular-nums";
  legend.textContent = "Clique na mesa principal para girar a roleta";

  header.append(titleGroup, legend);

  const stageWrap = document.createElement("div");
  stageWrap.className = "hall-stage";

  const canvas = document.createElement("canvas");
  canvas.width = HALL_WIDTH;
  canvas.height = HALL_HEIGHT;
  canvas.className = "hall-canvas";
  canvas.tabIndex = 0;
  canvas.setAttribute("role", "img");
  canvas.setAttribute(
    "aria-label",
    "Salão de bingo em pixel art com telão na parede, sorteador na roleta principal e mesas dos jogadores. Pressione Enter para girar a roleta."
  );

  stageWrap.appendChild(canvas);
  section.append(header, stageWrap);
  container.replaceChildren(section);

  const ctx = canvas.getContext("2d");
  ctx.imageSmoothingEnabled = false;

  // Offscreen buffer pixel-exact para a roleta top-down na mesa do sorteador
  const wheelCanvas = document.createElement("canvas");
  wheelCanvas.width = WHEEL_SIZE;
  wheelCanvas.height = WHEEL_SIZE;
  const wheelCtx = wheelCanvas.getContext("2d");
  wheelCtx.imageSmoothingEnabled = false;
  const wheelImageData = wheelCtx.createImageData(WHEEL_SIZE, WHEEL_SIZE);
  const wheelPixels = wheelImageData.data;

  const distGrid = new Float32Array(WHEEL_SIZE * WHEEL_SIZE);
  const angleGrid = new Float32Array(WHEEL_SIZE * WHEEL_SIZE);
  for (let y = 0; y < WHEEL_SIZE; y++) {
    for (let x = 0; x < WHEEL_SIZE; x++) {
      const idx = y * WHEEL_SIZE + x;
      const dx = x + 0.5 - WHEEL_CENTER;
      const dy = y + 0.5 - WHEEL_CENTER;
      distGrid[idx] = Math.hypot(dx, dy);
      angleGrid[idx] = Math.atan2(dy, dx);
    }
  }

  let currentPlayers = [];
  let currentCalled = [];
  let latestNumber = null;
  let wheelAngle = 0;
  let ballAngle = POINTER_ANGLE;
  let ballRadius = 12;
  let pointerKick = 0;
  let spinning = false;
  let callerCelebrationUntil = 0;
  let wheelDirty = true;
  let lastIdleDrawAt = 0;
  let particles = [];
  let playerBubbles = new Map(); // playerId -> { text, until }
  let prevMarkedCounts = new Map();
  let prevLineCounts = new Map();
  let prevWinnerIds = new Set();

  function drawRect(x, y, w, h, color) {
    ctx.fillStyle = color;
    ctx.fillRect(Math.round(x), Math.round(y), Math.round(w), Math.round(h));
  }

  function drawPixelText(text, x, y, scale, color, shadowColor = null) {
    const upper = String(text ?? "").toUpperCase();
    let cursorX = Math.round(x);
    const startY = Math.round(y);

    for (let i = 0; i < upper.length; i++) {
      const glyph = PIXEL_FONT_4X6[upper[i]] || PIXEL_FONT_4X6[" "];
      for (let r = 0; r < glyph.length; r++) {
        for (let c = 0; c < glyph[r].length; c++) {
          if (glyph[r][c] === "1") {
            if (shadowColor) {
              ctx.fillStyle = shadowColor;
              ctx.fillRect(cursorX + c * scale + 1, startY + r * scale + 1, scale, scale);
            }
            ctx.fillStyle = color;
            ctx.fillRect(cursorX + c * scale, startY + r * scale, scale, scale);
          }
        }
      }
      cursorX += 5 * scale;
    }
  }

  function measurePixelTextWidth(text, scale) {
    if (!text) return 0;
    return (text.length * 5 - 1) * scale;
  }

  function drawPlant(x, y) {
    drawRect(x - 5, y + 6, 10, 7, "#6b3e24");
    drawRect(x - 6, y + 4, 12, 3, "#8a5232");
    drawRect(x - 7, y - 2, 6, 6, "#25784f");
    drawRect(x + 1, y - 3, 6, 7, "#2d8f5f");
    drawRect(x - 3, y - 6, 6, 8, "#47b57d");
  }

  function drawWallSconce(x, y, now) {
    const flicker = Math.sin(now * 0.008 + x) > 0.2 ? 1 : 0;
    drawRect(x - 3, y, 6, 8, "#463123");
    drawRect(x - 2, y + 1, 4, 5, flicker ? "#f0c475" : "#d9a757");
    ctx.fillStyle = "rgba(240, 196, 117, 0.14)";
    ctx.fillRect(x - 8, y - 4, 16, 16);
  }

  function drawRoomBackground(now) {
    // Piso quadriculado do salão (estilo top-down PixelAgentsHQ)
    const tileSize = 16;
    for (let ty = 56; ty < HALL_HEIGHT; ty += tileSize) {
      for (let tx = 0; tx < HALL_WIDTH; tx += tileSize) {
        const checker = ((tx / tileSize + ty / tileSize) & 1) === 1;
        drawRect(tx, ty, tileSize, tileSize, checker ? "#131e27" : "#17242f");
        drawRect(tx, ty, tileSize, 1, "#101921");
      }
    }

    // Tapete central conduzindo ao palco do sorteador
    drawRect(142, 64, 100, 78, "#223b38");
    drawRect(144, 66, 96, 74, "#19473a");
    drawRect(146, 68, 92, 2, "#8ee0b5");
    drawRect(146, 136, 92, 2, "#8ee0b5");

    // Parede superior e rodapé de madeira
    drawRect(0, 0, HALL_WIDTH, 48, "#1b2733");
    drawRect(0, 48, HALL_WIDTH, 8, "#472e1f");
    drawRect(0, 48, HALL_WIDTH, 2, "#6c4832");
    drawRect(0, 56, HALL_WIDTH, 2, "#0c1218");

    // Estantes/quadros laterais e luminárias
    drawWallSconce(64, 20, now);
    drawWallSconce(320, 20, now);
    drawPlant(22, 46);
    drawPlant(362, 46);
  }

  function drawWallScreen(now) {
    // Telão na parede (x=88..296, y=4..50)
    const sx = 88;
    const sy = 4;
    const sw = 208;
    const sh = 46;

    drawRect(sx - 2, sy - 2, sw + 4, sh + 4, "#0a0f14");
    drawRect(sx, sy, sw, sh, "#384754");
    drawRect(sx + 2, sy + 2, sw - 4, sh - 4, "#0b141b");

    // Caixa do último número sorteado (esquerda do telão)
    drawRect(sx + 6, sy + 5, 46, 36, "#14222d");
    drawRect(sx + 7, sy + 6, 44, 1, "#2a4152");

    const bigText = spinning
      ? "??"
      : latestNumber !== null
        ? formatBingoNumber(latestNumber)
        : "--";
    const bigColor = spinning
      ? "#f0c475"
      : latestNumber !== null
        ? "#8ee0b5"
        : "#5c7080";
    drawPixelText(bigText, sx + 14, sy + 10, 3, bigColor, "#05090d");
    drawPixelText("TELAO", sx + 17, sy + 32, 1, "#b4c2cb");

    // Matriz LED 10x6 de números chamados (direita do telão)
    const calledSet = new Set(currentCalled);
    const gridX = sx + 58;
    const gridY = sy + 6;
    const cellW = 12;
    const cellH = 4;
    const gapX = 2;
    const gapY = 2;
    const pulseOn = Math.floor(now / 320) % 2 === 0;

    for (let n = 1; n <= 60; n++) {
      const idx = n - 1;
      const col = idx % 10;
      const row = Math.floor(idx / 10);
      const cx = gridX + col * (cellW + gapX);
      const cy = gridY + row * (cellH + gapY);

      const isLatest = n === latestNumber && !spinning;
      const isCalled = calledSet.has(n);

      let color = "#1c2b38";
      if (isLatest) {
        color = pulseOn ? "#f0c475" : "#ffdca0";
      } else if (isCalled) {
        color = "#8ee0b5";
      }

      drawRect(cx, cy, cellW, cellH, color);
    }
  }

  function renderWheelBuffer() {
    const activePocket =
      Math.floor(normalizeAngle(POINTER_ANGLE - wheelAngle) / POCKET_SPAN) %
      POCKET_COUNT;

    for (let y = 0; y < WHEEL_SIZE; y++) {
      for (let x = 0; x < WHEEL_SIZE; x++) {
        const idx = y * WHEEL_SIZE + x;
        const dist = distGrid[idx];
        const rawAngle = angleGrid[idx];
        const off = idx * 4;

        if (dist > 22) {
          wheelPixels[off + 3] = 0;
          continue;
        }

        let r = 20;
        let g = 28;
        let b = 36;

        if (dist > 20.5) {
          r = 14;
          g = 10;
          b = 8;
        } else if (dist > 17.5) {
          r = 120;
          g = 72;
          b = 40;
        } else if (dist > 14.5) {
          r = 34;
          g = 24;
          b = 20;
        } else if (dist > 6) {
          const relAngle = normalizeAngle(rawAngle - wheelAngle);
          const wedgeFloat = relAngle / POCKET_SPAN;
          const wedgeIdx = Math.floor(wedgeFloat) % POCKET_COUNT;
          const frac = wedgeFloat - Math.floor(wedgeFloat);
          if (frac < 0.08 || frac > 0.92 || dist > 13.8) {
            r = 16;
            g = 22;
            b = 28;
          } else {
            const base = WEDGE_PALETTE[wedgeIdx % WEDGE_PALETTE.length];
            const lit =
              !spinning && latestNumber !== null && wedgeIdx === activePocket
                ? 36
                : 0;
            r = Math.min(255, base[0] + lit);
            g = Math.min(255, base[1] + lit);
            b = Math.min(255, base[2] + lit);
          }
        } else if (dist > 4.5) {
          r = 228;
          g = 182;
          b = 92;
        } else {
          r = 18;
          g = 28;
          b = 36;
        }

        wheelPixels[off] = r;
        wheelPixels[off + 1] = g;
        wheelPixels[off + 2] = b;
        wheelPixels[off + 3] = 255;
      }
    }

    // 12 pinos dourados
    for (let i = 0; i < POCKET_COUNT; i++) {
      const pegAngle = wheelAngle + i * POCKET_SPAN;
      const px = Math.round(WHEEL_CENTER + Math.cos(pegAngle) * 14);
      const py = Math.round(WHEEL_CENTER + Math.sin(pegAngle) * 14);
      if (px >= 0 && px < WHEEL_SIZE && py >= 0 && py < WHEEL_SIZE) {
        const off = (py * WHEEL_SIZE + px) * 4;
        wheelPixels[off] = 248;
        wheelPixels[off + 1] = 220;
        wheelPixels[off + 2] = 140;
        wheelPixels[off + 3] = 255;
      }
    }

    wheelCtx.putImageData(wheelImageData, 0, 0);

    // Bola da roleta vista de cima
    const bx = Math.round(WHEEL_CENTER + Math.cos(ballAngle) * ballRadius);
    const by = Math.round(WHEEL_CENTER + Math.sin(ballAngle) * ballRadius);
    wheelCtx.fillStyle = "#ffffff";
    wheelCtx.fillRect(bx - 1, by - 1, 3, 3);
    wheelCtx.fillStyle = "#9db0bc";
    wheelCtx.fillRect(bx + 1, by + 1, 1, 1);

    // Catraca no topo (12h)
    const kick = Math.round(pointerKick * 2);
    wheelCtx.fillStyle = "#f0c475";
    wheelCtx.fillRect(WHEEL_CENTER - 2 + kick, 0, 4, 3);
    wheelCtx.fillStyle = "#ffffff";
    wheelCtx.fillRect(WHEEL_CENTER - 1 + kick, 3, 2, 2);
  }

  function drawPixelCharacter({
    x,
    y,
    theme,
    bob = 0,
    armsUp = false,
    armReach = false,
    isCaller = false,
  }) {
    const cy = Math.round(y + bob);

    // Sombra no chão
    drawRect(x - 6, cy + 7, 12, 3, "rgba(8, 12, 18, 0.45)");

    // Braços
    if (armsUp) {
      drawRect(x - 8, cy - 3, 3, 6, theme.shirt);
      drawRect(x + 5, cy - 3, 3, 6, theme.shirt);
      drawRect(x - 8, cy - 5, 3, 2, theme.skin);
      drawRect(x + 5, cy - 5, 3, 2, theme.skin);
    } else if (armReach) {
      drawRect(x - 7, cy + 1, 3, 5, theme.shirt);
      drawRect(x + 4, cy + 2, 4, 6, theme.shirt);
      drawRect(x + 5, cy + 8, 3, 2, theme.skin);
    } else {
      drawRect(x - 7, cy + 1, 3, 5, theme.shirt);
      drawRect(x + 4, cy + 1, 3, 5, theme.shirt);
    }

    // Tronco / Roupa
    drawRect(x - 5, cy, 10, 7, theme.shirt);
    if (isCaller) {
      // Colete e gravata-borboleta do sorteador
      drawRect(x - 3, cy + 1, 6, 5, "#192632");
      drawRect(x - 1, cy + 1, 2, 1, "#e05a4d");
    } else {
      drawRect(x - 4, cy + 1, 8, 1, theme.accent);
    }

    // Cabeça e cabelo (vista top-down)
    drawRect(x - 4, cy - 7, 8, 7, theme.skin);
    drawRect(x - 5, cy - 8, 10, 4, theme.hair);
    if (isCaller) {
      // Boné / viseira do sorteador
      drawRect(x - 5, cy - 9, 10, 3, "#f0c475");
      drawRect(x - 4, cy - 7, 8, 1, "#a6792b");
    }

    // Olhos voltados para a mesa / telão
    drawRect(x - 2, cy - 3, 1, 2, "#131c24");
    drawRect(x + 1, cy - 3, 1, 2, "#131c24");
  }

  function drawCallerStage(now) {
    // Palco elevado
    drawRect(148, 66, 88, 72, "#2f1e14");
    drawRect(150, 68, 84, 68, "#472e1f");
    drawRect(150, 68, 84, 2, "#6c4832");

    // Personagem Sorteador atrás da mesa principal
    const bob = spinning
      ? Math.sin(now * 0.024) * 1.2
      : Math.sin(now * 0.004) * 0.6;
    const armsUp = now < callerCelebrationUntil;

    drawPixelCharacter({
      x: 192,
      y: 78,
      theme: { hair: "#2a1b12", skin: "#f2c6a6", shirt: "#8ee0b5", accent: "#ffffff" },
      bob,
      armsUp,
      armReach: spinning,
      isCaller: true,
    });

    // Mesa principal do sorteador
    drawRect(156, 86, 72, 46, "#1f130d");
    drawRect(158, 88, 68, 42, "#633c25");
    drawRect(160, 90, 64, 38, "#164436");
    drawRect(160, 90, 64, 2, "#256652");

    // Roleta pixel art top-down sobre a mesa principal
    if (wheelDirty) {
      renderWheelBuffer();
      wheelDirty = false;
    }
    ctx.drawImage(
      wheelCanvas,
      WHEEL_CX - WHEEL_CENTER,
      WHEEL_CY - WHEEL_CENTER
    );

    // Partículas da roleta
    for (const p of particles) {
      ctx.globalAlpha = Math.max(0, p.life);
      drawRect(p.x, p.y, p.size, p.size, p.color);
    }
    ctx.globalAlpha = 1;

    // Etiqueta discreta da mesa principal
    const stageLabel = spinning ? "GIRANDO" : "SORTEADOR";
    const labelW = measurePixelTextWidth(stageLabel, 1);
    drawPixelText(stageLabel, 192 - labelW / 2, 131, 1, "#f0c475", "#090f14");
  }

  function drawSpeechBubble(x, y, text, color) {
    const textW = measurePixelTextWidth(text, 1);
    const bw = textW + 8;
    const bh = 12;
    const bx = Math.round(x - bw / 2);
    const by = Math.round(y - bh);

    drawRect(bx, by, bw, bh, "#0d151c");
    drawRect(bx + 1, by + 1, bw - 2, bh - 2, "#f1f5f7");
    drawRect(x - 1, by + bh, 3, 2, "#0d151c");
    drawRect(x, by + bh, 1, 1, "#f1f5f7");
    drawPixelText(text, bx + 4, by + 3, 1, color);
  }

  function drawPlayerStation(player, x, y, index, now) {
    const theme = PLAYER_THEMES[index % PLAYER_THEMES.length];
    const hasWon = player.wonRound !== null;
    const hasLine = (player.lines?.length ?? 0) > 0;
    const bubble = playerBubbles.get(player.id);
    const isCelebrating = hasWon || (bubble && now < bubble.until);

    // Mesa do jogador (44x24)
    const tx = x - 22;
    const ty = y - 8;
    drawRect(tx - 1, ty - 1, 46, 26, "#0b1117");
    drawRect(tx, ty, 44, 24, hasWon ? "#5a7c5a" : "#613c26");
    drawRect(tx + 2, ty + 2, 40, 20, "#1c3342");
    drawRect(tx + 2, ty + 2, 40, 2, hasWon ? "#8ee0b5" : "#2d4e63");

    // Mini cartela 3x5 sobre a mesa do jogador
    const cardX = x - 11;
    const cardY = ty + 5;
    drawRect(cardX - 1, cardY - 1, 22, 13, "#0d151c");
    drawRect(cardX, cardY, 20, 11, "#e4ebf0");

    const markedSet =
      player.markedNumbers instanceof Set
        ? player.markedNumbers
        : new Set(player.markedNumbers || []);

    for (let i = 0; i < (player.numbers?.length ?? 0); i++) {
      const col = i % 5;
      const row = Math.floor(i / 5);
      const px = cardX + 2 + col * 3 + (col > 0 ? col * 0.5 : 0);
      const py = cardY + 2 + row * 3;
      const isMarked = markedSet.has(player.numbers[i]);
      drawRect(
        Math.round(px),
        py,
        2,
        2,
        isMarked ? (hasLine ? "#c98b2e" : "#1f7a52") : "#8a9aa6"
      );
    }

    // Fichas de bingo ao lado da cartela
    drawRect(x + 12, ty + 7, 4, 4, "#f0c475");
    drawRect(x + 13, ty + 13, 4, 4, "#e05a4d");

    // Cadeira e personagem sentado abaixo da mesa voltado para cima
    drawRect(x - 6, y + 20, 12, 7, "#3a2518");
    const bob = isCelebrating
      ? Math.abs(Math.sin(now * 0.018 + index)) * -3
      : Math.sin(now * 0.004 + index * 1.3) * 0.6;

    drawPixelCharacter({
      x,
      y: y + 22,
      theme,
      bob,
      armsUp: isCelebrating,
      isCaller: false,
    });

    // Placa de nome e pontuação abaixo do personagem
    const shortName = (player.name || `J${player.id}`).slice(0, 9);
    const labelColor = hasWon ? "#8ee0b5" : hasLine ? "#f0c475" : "#d8e2e8";
    const nameW = measurePixelTextWidth(shortName, 1);
    drawPixelText(shortName, x - nameW / 2, y + 34, 1, labelColor, "#090f14");

    const scoreText = hasWon
      ? "BINGO!"
      : `${markedSet.size}/15`;
    const scoreW = measurePixelTextWidth(scoreText, 1);
    drawPixelText(scoreText, x - scoreW / 2, y + 42, 1, "#8ee0b5", "#090f14");

    // Balão de reação em pixel art
    if (bubble && now < bubble.until) {
      drawSpeechBubble(x, y + 10 + bob, bubble.text, bubble.color);
    }
  }

  function renderScene(now) {
    ctx.clearRect(0, 0, HALL_WIDTH, HALL_HEIGHT);
    drawRoomBackground(now);
    drawWallScreen(now);
    drawCallerStage(now);

    const positions = getTableXPositions(currentPlayers.length || 2);
    for (let i = 0; i < currentPlayers.length; i++) {
      drawPlayerStation(currentPlayers[i], positions[i], 172, i, now);
    }
  }

  function spawnSparkBurst() {
    const colors = ["#8ee0b5", "#f0c475", "#f4f6f8", "#ff8c6b"];
    particles = [];
    for (let i = 0; i < 18; i++) {
      const angle = (Math.PI * 2 * i) / 18 + (Math.random() - 0.5) * 0.25;
      const speed = 0.55 + Math.random() * 0.9;
      particles.push({
        x: WHEEL_CX,
        y: WHEEL_CY,
        vx: Math.cos(angle) * speed,
        vy: Math.sin(angle) * speed,
        life: 1,
        size: i % 3 === 0 ? 3 : 2,
        color: colors[i % colors.length],
      });
    }
  }

  function stepParticles() {
    if (particles.length === 0) return;
    const alive = [];
    for (const p of particles) {
      p.x += p.vx;
      p.y += p.vy;
      p.vx *= 0.95;
      p.vy *= 0.95;
      p.life -= 0.03;
      if (p.life > 0) alive.push(p);
    }
    particles = alive;
  }

  let spinFrameState = null;

  function loop(now) {
    if (spinFrameState) {
      const s = spinFrameState;
      const elapsed = now - s.startTime;
      const t = Math.min(1, elapsed / s.duration);
      const wheelEase = easeOutQuart(t);
      wheelAngle = s.startWheel + (s.finalWheel - s.startWheel) * wheelEase;

      const currentPegIdx = Math.floor(
        normalizeAngle(POINTER_ANGLE - wheelAngle) / POCKET_SPAN
      );
      if (!s.reducedMotion && currentPegIdx !== s.lastPegIdx) {
        s.lastPegIdx = currentPegIdx;
        pointerKick = 1;
        soundEngine?.playPegTick(Math.max(0.05, 1 - t));
      }
      pointerKick *= 0.78;

      const ballEase = easeOutCubic(t);
      ballAngle = s.startBall + (s.finalBall - s.startBall) * ballEase;

      if (t < 0.68) {
        ballRadius = 15.5;
      } else if (t < 0.88) {
        const dropT = (t - 0.68) / 0.2;
        const bounce = Math.abs(Math.sin(dropT * Math.PI * 2)) * (1 - dropT) * 2.5;
        ballRadius = 15.5 - (15.5 - 11.5) * easeOutCubic(dropT) + bounce;
        if (!s.bouncePlayed && dropT > 0.35) {
          s.bouncePlayed = true;
          soundEngine?.playBallBounce();
        }
      } else {
        const settleT = (t - 0.88) / 0.12;
        ballRadius = 11.5 + Math.abs(Math.sin(settleT * Math.PI)) * (1 - settleT) * 1;
      }

      wheelDirty = true;

      if (t >= 1) {
        const finishedCallback = s.onComplete;
        const target = s.targetNumber;
        spinFrameState = null;
        spinning = false;
        wheelAngle = s.finalWheel;
        ballAngle = POINTER_ANGLE;
        ballRadius = 11.5;
        pointerKick = 0;
        latestNumber = target;
        callerCelebrationUntil = now + 1200;
        wheelDirty = true;

        spawnSparkBurst();
        soundEngine?.playBallReveal();

        subtitle.textContent = `Telão: ${formatBingoNumber(target)} · Clique na mesa principal para girar`;
        legend.textContent = `Último sorteio: ${formatBingoNumber(target)}`;
        finishedCallback?.(target);
      }
    }

    const hasActiveAnimation =
      spinning ||
      particles.length > 0 ||
      now < callerCelebrationUntil ||
      Array.from(playerBubbles.values()).some((b) => now < b.until);

    if (hasActiveAnimation || now - lastIdleDrawAt >= 90) {
      lastIdleDrawAt = now;
      stepParticles();
      renderScene(now);
    }
    window.requestAnimationFrame(loop);
  }

  window.requestAnimationFrame(loop);

  function spinTo(targetNumber, onComplete) {
    if (spinning) return;
    spinning = true;
    particles = [];

    const reducedMotion = window.matchMedia(
      "(prefers-reduced-motion: reduce)"
    ).matches;

    const targetPocket = targetNumber % POCKET_COUNT;
    const startWheel = wheelAngle;
    const desiredFinalMod = normalizeAngle(
      POINTER_ANGLE - (targetPocket + 0.5) * POCKET_SPAN
    );
    const currentMod = normalizeAngle(startWheel);
    let deltaToTarget = desiredFinalMod - currentMod;
    if (deltaToTarget <= 0) deltaToTarget += Math.PI * 2;

    const fullTurns = reducedMotion ? 0 : 5 + Math.floor(Math.random() * 2);
    const finalWheel = startWheel + fullTurns * Math.PI * 2 + deltaToTarget;

    soundEngine?.playSpinStart();
    subtitle.textContent = "Sorteador girando a roleta...";
    legend.textContent = "Sorteando...";

    spinFrameState = {
      targetNumber,
      onComplete,
      reducedMotion,
      startTime: performance.now(),
      duration: reducedMotion ? 160 : 2350,
      startWheel,
      finalWheel,
      startBall: ballAngle,
      finalBall: POINTER_ANGLE - (reducedMotion ? 0 : Math.PI * 2 * 4),
      lastPegIdx: Math.floor(
        normalizeAngle(POINTER_ANGLE - startWheel) / POCKET_SPAN
      ),
      bouncePlayed: false,
    };
  }

  // Clique direto no canvas: mesa do sorteador gira a roleta; mesas de jogadores selecionam cartela
  canvas.addEventListener("click", (event) => {
    const rect = canvas.getBoundingClientRect();
    const cx = ((event.clientX - rect.left) / rect.width) * HALL_WIDTH;
    const cy = ((event.clientY - rect.top) / rect.height) * HALL_HEIGHT;

    if (cx >= 148 && cx <= 236 && cy >= 66 && cy <= 138) {
      onSpinRequest?.();
      return;
    }

    const positions = getTableXPositions(currentPlayers.length || 2);
    for (let i = 0; i < currentPlayers.length; i++) {
      const px = positions[i];
      if (cx >= px - 26 && cx <= px + 26 && cy >= 156 && cy <= 228) {
        onSelectPlayer?.(currentPlayers[i].id);
        return;
      }
    }
  });

  canvas.addEventListener("keydown", (event) => {
    if (event.key === "Enter" || event.key === " ") {
      event.preventDefault();
      onSpinRequest?.();
    }
  });

  function syncState({ players = [], calledNumbers = [], latest = null, resetReactions = false }) {
    const now = performance.now();
    currentPlayers = players;
    currentCalled = calledNumbers;

    if (!spinning) {
      latestNumber = latest;
      if (latest !== null) {
        const pocket = latest % POCKET_COUNT;
        wheelAngle = POINTER_ANGLE - (pocket + 0.5) * POCKET_SPAN;
        ballAngle = POINTER_ANGLE;
        ballRadius = 11.5;
        wheelDirty = true;
        subtitle.textContent = `Telão: ${formatBingoNumber(latest)} · ${calledNumbers.length}/60 chamados`;
        legend.textContent = "Clique na roleta para sortear o próximo";
      } else {
        wheelDirty = true;
        subtitle.textContent = "Telão na parede · Sorteador e mesas dos jogadores";
        legend.textContent = "Clique na mesa principal para girar a roleta";
      }
    }

    if (resetReactions) {
      playerBubbles.clear();
      prevMarkedCounts.clear();
      prevLineCounts.clear();
      prevWinnerIds.clear();
    }

    for (const p of players) {
      const markedSize =
        p.markedNumbers instanceof Set
          ? p.markedNumbers.size
          : Array.isArray(p.markedNumbers)
            ? p.markedNumbers.length
            : 0;
      const lineCount = p.lines?.length ?? 0;
      const won = p.wonRound !== null;

      const prevMarked = prevMarkedCounts.get(p.id);
      const prevLines = prevLineCounts.get(p.id);
      const hadWon = prevWinnerIds.has(p.id);

      if (!resetReactions && prevMarked !== undefined) {
        if (won && !hadWon) {
          playerBubbles.set(p.id, {
            text: "BINGO!",
            color: "#157a4e",
            until: now + 3400,
          });
        } else if (lineCount > (prevLines ?? 0)) {
          playerBubbles.set(p.id, {
            text: "LINHA!",
            color: "#b06e14",
            until: now + 2600,
          });
        } else if (markedSize > prevMarked) {
          playerBubbles.set(p.id, {
            text: "+1",
            color: "#1f4e78",
            until: now + 1500,
          });
        }
      }

      prevMarkedCounts.set(p.id, markedSize);
      prevLineCounts.set(p.id, lineCount);
      if (won) prevWinnerIds.add(p.id);
      else prevWinnerIds.delete(p.id);
    }
  }

  return {
    spinTo,
    syncState,
    isSpinning: () => spinning,
  };
}
