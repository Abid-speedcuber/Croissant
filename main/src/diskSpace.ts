// Platform-agnostic disk-space reporting used by the Manage Disk Space modal.
// Desktop/mobile reports real table files via Tauri commands; the browser build
// reports the pruning tables that were persisted to IndexedDB plus the size of
// the downloaded assets.

import { tauri } from "./utils/solver";
import { isNativePlatform, solutionBytes } from "./storage";
import { listTableEntries, deleteTableBlob, clearTableBlobs } from "./tableStore";
import { t } from "./i18n";
import type { TableEntry } from "./tableStore";
import type { Solution } from "./utils/types";

export type { TableEntry };
export type DiskSpaceReport = {
  appSize: number;
  solutionBytes: number;
  tables: TableEntry[];
  total: number;
};

const TABLE_ORDER = [
  "sq1stt.dat",
  "sq1scte.dat",
  "sq1sctc.dat",
  "sq1p1u.dat",
  "sq1p2u.dat",
  "sq1p1a.dat",
  "sq1p2a.dat",
  "sq1p1w.dat",
  "sq1p2w.dat",
];

export function tableLabel(name: string): string {
  const key = `modal.disk.tables.${name.replace(/\./g, "_")}`;
  const label = t(key);
  return label === key ? name : label;
}

function sortTables(tables: TableEntry[]): TableEntry[] {
  const rank = (name: string) => {
    const i = TABLE_ORDER.indexOf(name);
    return i === -1 ? TABLE_ORDER.length : i;
  };
  return [...tables].sort((a, b) => rank(a.name) - rank(b.name) || a.name.localeCompare(b.name));
}

export function formatBytes(bytes: number): string {
  const n = Math.max(0, bytes);
  if (n < 1000) return `${Math.round(n)} B`;
  const units = ["KB", "MB", "GB", "TB"];
  let value = n / 1000;
  let unit = 0;
  while (value >= 1000 && unit < units.length - 1) {
    value /= 1000;
    unit++;
  }
  const decimals = value < 10 ? 1 : 0;
  return `${value.toFixed(decimals)} ${units[unit]}`;
}

function webAppSize(): number {
  let total = 0;
  try {
    for (const entry of performance.getEntriesByType("resource") as PerformanceResourceTiming[]) {
      total += entry.transferSize > 0 ? entry.transferSize : entry.decodedBodySize;
    }
  } catch {
    // resource timing unavailable
  }
  return total;
}

// Approximate in-memory size of the currently generated solutions.  The exact
// bytes are not measurable from JS, so we sum the stored strings plus a small
// per-solution overhead.
export function estimateSolutionBytes(solutions: Solution[]): number {
  return solutions.reduce(
    (sum, s) => sum + s.raw.length + s.rawDisplay.length + s.karnDisplay.length + s.algRaw.length + 48,
    0,
  );
}

async function nativeTables(): Promise<TableEntry[]> {
  const native = tauri();
  if (!native?.core?.invoke) return [];
  try {
    return (await native.core.invoke<TableEntry[]>("list_pruning_tables")) || [];
  } catch {
    return [];
  }
}

export async function getDiskSpaceReport(solutions: Solution[]): Promise<DiskSpaceReport> {
  const solBytes = (await solutionBytes()) + estimateSolutionBytes(solutions);
  let appSize = 0;
  let tables: TableEntry[] = [];
  if (isNativePlatform()) {
    const native = tauri();
    if (native?.core?.invoke) {
      try { appSize = await native.core.invoke<number>("app_size"); } catch { appSize = 0; }
      tables = await nativeTables();
    }
  } else {
    appSize = webAppSize();
    tables = await listTableEntries();
  }
  tables = sortTables(tables);
  const total = appSize + solBytes + tables.reduce((sum, table) => sum + table.size, 0);
  return { appSize, solutionBytes: solBytes, tables, total };
}

export async function deleteTable(name: string): Promise<void> {
  if (isNativePlatform()) {
    const native = tauri();
    if (native?.core?.invoke) {
      try { await native.core.invoke("delete_pruning_table", { name }); } catch { /* best effort */ }
    }
  } else {
    await deleteTableBlob(name);
    const { deleteSolverTable } = await import("./webNative");
    deleteSolverTable(name);
  }
}

export async function clearAllTables(): Promise<void> {
  if (isNativePlatform()) {
    const native = tauri();
    if (native?.core?.invoke) {
      try { await native.core.invoke("clear_pruning_tables"); } catch { /* best effort */ }
    }
  } else {
    await clearTableBlobs();
  }
}
