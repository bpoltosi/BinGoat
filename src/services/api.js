const API_BASE_URL = "/api";

/**
 * Cliente HTTP do frontend.
 *
 * Na implementação atual, as rotas são atendidas pela camada Node.js/Vite
 * em src/server/cEngineRunner.js, que executa o motor TrabalhoBingo.c.
 * A migração para um servidor HTTP nativo em C está documentada em
 * docs/BACKEND-C-MIGRATION.md.
 *
 * O frontend usa caminhos relativos para manter o mesmo contrato durante
 * a migração do backend.
 */
export async function apiRequest(path, options = {}) {
  const normalizedPath = path.startsWith("/") ? path : `/${path}`;

  const response = await fetch(`${API_BASE_URL}${normalizedPath}`, {
    ...options,
    headers: {
      Accept: "application/json",
      ...(options.body ? { "Content-Type": "application/json" } : {}),
      ...options.headers,
    },
  });

  const contentType = response.headers.get("content-type") ?? "";
  const payload = contentType.includes("application/json")
    ? await response.json()
    : await response.text();

  if (!response.ok) {
    const message =
      typeof payload === "object" && payload !== null && "error" in payload
        ? payload.error
        : `Erro HTTP ${response.status}`;

    throw new Error(message);
  }

  return payload;
}
