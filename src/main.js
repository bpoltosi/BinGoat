import "./style.css";

const app = document.querySelector("#app");

app.innerHTML = `
  <main class="page-shell">
    <header class="topbar">
      <a class="brand" href="/" aria-label="BinGoat — início">
        <span class="brand-mark" aria-hidden="true">B</span>
        <span>BinGoat</span>
      </a>
      <span class="environment-tag">Prévia de desenvolvimento</span>
    </header>

    <section class="hero" aria-labelledby="page-title">
      <p class="eyebrow">BINGO • EXPERIÊNCIA WEB</p>
      <h1 id="page-title">O jogo está prestes a começar.</h1>
      <p class="intro">
        A estrutura inicial da interface web está pronta. O próximo passo é
        conectar esta aplicação ao motor do bingo em C por meio da API HTTP.
      </p>

      <section class="status-panel" aria-live="polite">
        <span class="status-indicator" aria-hidden="true"></span>
        <div>
          <strong>Backend ainda não conectado</strong>
          <p>
            Nenhuma partida ou cartela fictícia está sendo exibida. Os dados
            reais virão do backend em C quando a API estiver implementada.
          </p>
        </div>
      </section>

      <div class="next-steps">
        <h2>Base do projeto</h2>
        <ul>
          <li>Vite configurado para desenvolvimento e build.</li>
          <li>Frontend em HTML, CSS e JavaScript, sem framework.</li>
          <li>Cliente HTTP preparado para a futura API em <code>/api</code>.</li>
          <li>Motor e regras do jogo continuam sob responsabilidade do C.</li>
        </ul>
      </div>
    </section>

    <footer class="footer">
      <span>BinGoat</span>
      <span>Interface em construção</span>
    </footer>
  </main>
`;
