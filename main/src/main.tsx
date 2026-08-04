import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import App from "./App";
import "./styles.css";
import "./inputbar.css";

async function installNativeBridge() {
  const target = window as Window & { __SQ1_NATIVE__?: unknown };
  if (import.meta.env.MODE === "web") {
    const { createWebNative } = await import("./webNative");
    target.__SQ1_NATIVE__ = createWebNative();
    return;
  }
  const [core, event] = await Promise.all([
    import("@tauri-apps/api/core"),
    import("@tauri-apps/api/event"),
  ]);
  target.__SQ1_NATIVE__ = {
    core: { invoke: core.invoke },
    event: { listen: event.listen },
    Channel: core.Channel,
  };
}

void installNativeBridge().finally(() => {
  createRoot(document.getElementById("root")!).render(<StrictMode><App /></StrictMode>);
});
