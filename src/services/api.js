const API_BASE_URL = "/api";

/**
 * Cliente HTTP do frontend.
 *
 * Os endpoints serão disponibilizados pelo servidor C (CivetWeb).
 * Não há dados simulados: chamadas só devem ser feitas quando o backend
 * estiver em execução e os endpoints correspondentes estiverem implementados.
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
