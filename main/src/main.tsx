import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import { Channel, invoke } from "@tauri-apps/api/core";
import { listen } from "@tauri-apps/api/event";
import App from "./App";
import "./styles.css";
import "./inputbar.css";

(window as Window & { __SQ1_NATIVE__?: unknown }).__SQ1_NATIVE__ = {
  core: { invoke },
  event: { listen },
  Channel,
};

createRoot(document.getElementById("root")!).render(<StrictMode><App /></StrictMode>);
