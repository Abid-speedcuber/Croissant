import type { Solution } from "./utils/types";

type StorageBackend = {
  getItem(key: string): Promise<string | null>;
  setItem(key: string, value: string): Promise<void>;
  removeItem(key: string): Promise<void>;
  keys(): Promise<string[]>;
};

let backend: StorageBackend | null = null;

function isTauri(): boolean {
  const w = window as Window & { __SQ1_NATIVE__?: unknown; __TAURI__?: unknown };
  return !!(w.__SQ1_NATIVE__ || w.__TAURI__);
}

const localStorageBackend: StorageBackend = {
  getItem(key) {
    try {
      return Promise.resolve(localStorage.getItem(key));
    } catch {
      return Promise.resolve(null);
    }
  },
  setItem(key, value) {
    try {
      localStorage.setItem(key, value);
    } catch { /* storage full or unavailable */ }
    return Promise.resolve();
  },
  removeItem(key) {
    try {
      localStorage.removeItem(key);
    } catch { /* storage unavailable */ }
    return Promise.resolve();
  },
  keys() {
    try {
      return Promise.resolve(Object.keys(localStorage));
    } catch {
      return Promise.resolve([]);
    }
  },
};

async function getBackend(): Promise<StorageBackend> {
  if (backend) return backend;
  if (isTauri()) {
    try {
      const { LazyStore } = await import("@tauri-apps/plugin-store");
      const store = new LazyStore("croissant.json", { autoSave: 200 });
      await store.init();
      backend = {
        async getItem(key) {
          const val = await store.get<string>(key);
          return val ?? null;
        },
        async setItem(key, value) {
          await store.set(key, value);
        },
        async removeItem(key) {
          await store.delete(key);
        },
        async keys() {
          return store.keys();
        },
      };
      return backend;
    } catch { /* plugin-store unavailable, fall through */ }
  }
  backend = localStorageBackend;
  return backend;
}

export async function loadSettings(): Promise<Record<string, unknown> | null> {
  try {
    const store = await getBackend();
    const raw = await store.getItem("croissant-settings");
    if (!raw) return null;
    return JSON.parse(raw) as Record<string, unknown>;
  } catch {
    return null;
  }
}

export async function saveSettings(settings: Record<string, unknown>): Promise<void> {
  try {
    const store = await getBackend();
    await store.setItem("croissant-settings", JSON.stringify(settings));
  } catch { /* best effort */ }
}

export async function loadFavorites(): Promise<Record<string, { name: string; algorithms: string[] }> | null> {
  try {
    const store = await getBackend();
    const raw = await store.getItem("croissant-favorites");
    if (!raw) return null;
    return JSON.parse(raw) as Record<string, { name: string; algorithms: string[] }>;
  } catch {
    return null;
  }
}

export async function saveFavorites(favorites: Record<string, { name: string; algorithms: string[] }>): Promise<void> {
  try {
    const store = await getBackend();
    await store.setItem("croissant-favorites", JSON.stringify(favorites));
  } catch { /* best effort */ }
}

const OFFLOAD_PREFIX = "croissant-offload-";

export async function writeOffloadedChunk(startIndex: number, rows: Solution[]): Promise<void> {
  try {
    const store = await getBackend();
    await store.setItem(`${OFFLOAD_PREFIX}${startIndex}`, JSON.stringify(rows));
  } catch { /* best effort */ }
}

export async function readOffloadedChunk(startIndex: number): Promise<Solution[] | null> {
  try {
    const store = await getBackend();
    const raw = await store.getItem(`${OFFLOAD_PREFIX}${startIndex}`);
    if (!raw) return null;
    return JSON.parse(raw) as Solution[];
  } catch {
    return null;
  }
}

export async function removeOffloadedChunk(startIndex: number): Promise<void> {
  try {
    const store = await getBackend();
    await store.removeItem(`${OFFLOAD_PREFIX}${startIndex}`);
  } catch { /* best effort */ }
}

export async function clearOffloadedSolutions(): Promise<void> {
  try {
    const store = await getBackend();
    const keys = await store.keys();
    for (const key of keys) {
      if (key.startsWith(OFFLOAD_PREFIX)) await store.removeItem(key);
    }
  } catch { /* best effort */ }
}
