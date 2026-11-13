import http from "node:http";
import fs from "node:fs/promises";
import path from "node:path";
import { handleApiRequest } from "./src/server/cEngineRunner.js";

const PORT = Number(process.env.PORT) || 3000;
const HOST = "0.0.0.0";
const DIST_DIR = path.resolve(process.cwd(), "dist");

const MIME_TYPES = {
  ".html": "text/html; charset=utf-8",
  ".js": "application/javascript; charset=utf-8",
  ".css": "text/css; charset=utf-8",
  ".json": "application/json; charset=utf-8",
  ".svg": "image/svg+xml",
  ".png": "image/png",
  ".ico": "image/x-icon",
};

const server = http.createServer(async (req, res) => {
  const url = new URL(req.url || "/", `http://${HOST}:${PORT}`);

  if (url.pathname.startsWith("/api/")) {
    await handleApiRequest(req, res, url.pathname);
    return;
  }

  let filePath = path.join(
    DIST_DIR,
    url.pathname === "/" ? "index.html" : url.pathname
  );

  try {
    const stat = await fs.stat(filePath);
    if (stat.isDirectory()) {
      filePath = path.join(filePath, "index.html");
    }
  } catch {
    filePath = path.join(DIST_DIR, "index.html");
  }

  try {
    const content = await fs.readFile(filePath);
    const ext = path.extname(filePath);
    res.statusCode = 200;
    res.setHeader("Content-Type", MIME_TYPES[ext] || "application/octet-stream");
    res.end(content);
  } catch {
    res.statusCode = 404;
    res.end("Not found");
  }
});

server.listen(PORT, HOST, () => {
  console.log(`BinGoat server (TrabalhoBingo.c) listening on http://${HOST}:${PORT}`);
});
