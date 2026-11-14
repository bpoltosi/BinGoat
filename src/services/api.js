const API_BASE_URL = "/api";

/**
 * Cliente HTTP do frontend para a API nativa do servidor C (CivetWeb).
 * O backend mantém o estado autoritativo da partida; o frontend apenas o
 * consulta e envia comandos por meio dos endpoints /api.
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
