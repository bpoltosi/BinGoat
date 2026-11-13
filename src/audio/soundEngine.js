/**
 * Motor sonoro retrô (Web Audio API) sem dependências externas.
 * Gera efeitos sintetizados em estilo chiptune/arcade para a roleta,
 * marcação de números, linhas completas e vitória.
 */

const SOUND_PREF_KEY = "bingoat.sound.enabled";

function loadMutedPref() {
  try {
    const raw = window.localStorage.getItem(SOUND_PREF_KEY);
    if (raw === "false") return true;
  } catch {
    // Ignora erros de localStorage
  }
  return false;
}

function saveMutedPref(muted) {
  try {
    window.localStorage.setItem(SOUND_PREF_KEY, String(!muted));
  } catch {
    // Ignora erros de localStorage
  }
}

export function createSoundEngine() {
  let ctx = null;
  let muted = loadMutedPref();

  function getContext() {
    if (muted) return null;
    const AudioCtx = window.AudioContext || window.webkitAudioContext;
    if (!AudioCtx) return null;
    if (!ctx) {
      ctx = new AudioCtx();
    }
    if (ctx.state === "suspended") {
      ctx.resume().catch(() => {});
    }
    return ctx;
  }

  function playTone({
    freq = 440,
    endFreq = freq,
    duration = 0.08,
    type = "square",
    gain = 0.08,
    delay = 0,
  }) {
    const audio = getContext();
    if (!audio) return;

    const now = audio.currentTime + delay;
    const osc = audio.createOscillator();
    const amp = audio.createGain();

    osc.type = type;
    osc.frequency.setValueAtTime(freq, now);
    if (endFreq !== freq) {
      osc.frequency.exponentialRampToValueAtTime(
        Math.max(20, endFreq),
        now + duration
      );
    }

    amp.gain.setValueAtTime(0.0001, now);
    amp.gain.linearRampToValueAtTime(gain, now + 0.008);
    amp.gain.exponentialRampToValueAtTime(0.0001, now + duration);

    osc.connect(amp);
    amp.connect(audio.destination);

    osc.start(now);
    osc.stop(now + duration + 0.01);
  }

  return {
    isMuted() {
      return muted;
    },

    setMuted(nextMuted) {
      muted = Boolean(nextMuted);
      saveMutedPref(muted);
      if (!muted) {
        getContext();
      }
      return muted;
    },

    toggleMute() {
      return this.setMuted(!muted);
    },

    unlock() {
      getContext();
    },

    playSpinStart() {
      playTone({ freq: 220, endFreq: 520, duration: 0.12, type: "square", gain: 0.07 });
      playTone({ freq: 330, endFreq: 660, duration: 0.1, type: "triangle", gain: 0.08, delay: 0.06 });
    },

    playPegTick(speedRatio = 0.5) {
      const clamped = Math.max(0, Math.min(1, speedRatio));
      const freq = 420 + clamped * 380;
      playTone({
        freq,
        endFreq: freq * 0.55,
        duration: 0.024,
        type: "square",
        gain: 0.04 + clamped * 0.025,
      });
    },

    playBallBounce() {
      playTone({
        freq: 210,
        endFreq: 95,
        duration: 0.055,
        type: "triangle",
        gain: 0.09,
      });
    },

    playBallReveal() {
      playTone({ freq: 523.25, duration: 0.09, type: "square", gain: 0.07, delay: 0 });
      playTone({ freq: 659.25, duration: 0.09, type: "square", gain: 0.07, delay: 0.07 });
      playTone({ freq: 783.99, duration: 0.14, type: "square", gain: 0.08, delay: 0.14 });
      playTone({ freq: 1046.5, duration: 0.22, type: "triangle", gain: 0.1, delay: 0.2 });
    },

    playBoardMark(marked) {
      if (marked) {
        playTone({ freq: 520, endFreq: 740, duration: 0.065, type: "square", gain: 0.055 });
      } else {
        playTone({ freq: 380, endFreq: 250, duration: 0.065, type: "triangle", gain: 0.06 });
      }
    },

    playLineComplete(orientation) {
      const notes =
        orientation === "diagonal"
          ? [523.25, 659.25, 783.99, 1174.66]
          : [523.25, 659.25, 783.99, 1046.5];

      notes.forEach((freq, index) => {
        playTone({
          freq,
          duration: index === notes.length - 1 ? 0.24 : 0.1,
          type: "square",
          gain: 0.075,
          delay: index * 0.08,
        });
      });
    },

    playBingoWin() {
      const melody = [
        { freq: 523.25, dur: 0.1, delay: 0 },
        { freq: 659.25, dur: 0.1, delay: 0.09 },
        { freq: 783.99, dur: 0.1, delay: 0.18 },
        { freq: 1046.5, dur: 0.16, delay: 0.27 },
        { freq: 783.99, dur: 0.1, delay: 0.44 },
        { freq: 1046.5, dur: 0.32, delay: 0.54 },
      ];

      for (const note of melody) {
        playTone({
          freq: note.freq,
          duration: note.dur,
          type: "square",
          gain: 0.08,
          delay: note.delay,
        });
        playTone({
          freq: note.freq * 0.5,
          duration: note.dur,
          type: "triangle",
          gain: 0.07,
          delay: note.delay,
        });
      }
    },

    playUndo() {
      playTone({ freq: 440, endFreq: 280, duration: 0.08, type: "triangle", gain: 0.06 });
    },

    playReset() {
      playTone({ freq: 340, endFreq: 190, duration: 0.12, type: "triangle", gain: 0.06 });
    },
  };
}
