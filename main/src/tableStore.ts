// IndexedDB-backed store for the generated pruning tables on web builds.
// The desktop/mobile builds keep tables as real files (see list_pruning_tables),
// but the WASM solver only has the in-memory Emscripten virtual filesystem, so
// persisting them across loads is done here instead.

export type TableEntry = { name: string; size: number };

const DB_NAME = "croissant-tables";
const DB_VERSION = 1;
const STORE_NAME = "tables";

let dbPromise: Promise<IDBDatabase | null> | undefined;

function openDb(): Promise<IDBDatabase | null> {
  if (dbPromise) return dbPromise;
  if (typeof indexedDB === "undefined") {
    dbPromise = Promise.resolve(null);
    return dbPromise;
  }
  dbPromise = new Promise((resolve) => {
    try {
      const request = indexedDB.open(DB_NAME, DB_VERSION);
      request.onupgradeneeded = () => {
        const db = request.result;
        if (!db.objectStoreNames.contains(STORE_NAME)) db.createObjectStore(STORE_NAME);
      };
      request.onsuccess = () => resolve(request.result);
      request.onerror = () => resolve(null);
      request.onblocked = () => resolve(null);
    } catch {
      resolve(null);
    }
  });
  return dbPromise;
}

export async function listTableEntries(): Promise<TableEntry[]> {
  const db = await openDb();
  if (!db) return [];
  return new Promise((resolve) => {
    try {
      const tx = db.transaction(STORE_NAME, "readonly");
      const out: TableEntry[] = [];
      const request = tx.objectStore(STORE_NAME).openCursor();
      request.onsuccess = () => {
        const cursor = request.result;
        if (cursor) {
          const value = cursor.value as ArrayBuffer;
          out.push({ name: String(cursor.key), size: value.byteLength });
          cursor.continue();
        } else resolve(out);
      };
      request.onerror = () => resolve(out);
      tx.onerror = () => resolve(out);
      tx.onabort = () => resolve(out);
    } catch {
      resolve([]);
    }
  });
}

export async function loadTableBlob(name: string): Promise<ArrayBuffer | null> {
  const db = await openDb();
  if (!db) return null;
  return new Promise((resolve) => {
    try {
      const tx = db.transaction(STORE_NAME, "readonly");
      const request = tx.objectStore(STORE_NAME).get(name) as IDBRequest<ArrayBuffer>;
      request.onsuccess = () => resolve(request.result ?? null);
      request.onerror = () => resolve(null);
      tx.onerror = () => resolve(null);
      tx.onabort = () => resolve(null);
    } catch {
      resolve(null);
    }
  });
}

export async function storeTableBlob(name: string, bytes: ArrayBuffer): Promise<void> {
  const db = await openDb();
  if (!db) return;
  try {
    const tx = db.transaction(STORE_NAME, "readwrite");
    tx.objectStore(STORE_NAME).put(bytes, name);
  } catch { /* storage unavailable */ }
}

export async function deleteTableBlob(name: string): Promise<void> {
  const db = await openDb();
  if (!db) return;
  try {
    const tx = db.transaction(STORE_NAME, "readwrite");
    tx.objectStore(STORE_NAME).delete(name);
  } catch { /* storage unavailable */ }
}

export async function clearTableBlobs(): Promise<void> {
  const db = await openDb();
  if (!db) return;
  try {
    const tx = db.transaction(STORE_NAME, "readwrite");
    tx.objectStore(STORE_NAME).clear();
  } catch { /* storage unavailable */ }
}
