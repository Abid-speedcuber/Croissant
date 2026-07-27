type StorageBackend = {
  getItem(key: string): Promise<string | null>;
  setItem(key: string, value: string): Promise<void>;
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
