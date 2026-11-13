import { defineConfig } from "vite";
import { handleApiRequest } from "./src/server/cEngineRunner.js";

function bingoCPlugin() {
  return {
    name: "bingoat-c-engine-api",
    configureServer(server) {
      server.middlewares.use("/api", (req, res, next) => {
        const pathname = `/api${(req.url || "/").split("?")[0]}`;
        handleApiRequest(req, res, pathname).catch(next);
      });
    },
    configurePreviewServer(server) {
      server.middlewares.use("/api", (req, res, next) => {
        const pathname = `/api${(req.url || "/").split("?")[0]}`;
        handleApiRequest(req, res, pathname).catch(next);
      });
    },
  };
}

export default defineConfig({
  plugins: [bingoCPlugin()],
  server: {
    host: "0.0.0.0",
    port: 3000,
    allowedHosts: true,
  },
  preview: {
    host: "0.0.0.0",
    port: 3000,
    allowedHosts: true,
  },
});
