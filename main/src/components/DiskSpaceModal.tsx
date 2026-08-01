import { useEffect, useRef, useState } from "react";
import { t } from "../i18n";
import {
  getDiskSpaceReport, deleteTable, formatBytes, tableLabel,
} from "../diskSpace";
import type { DiskSpaceReport } from "../diskSpace";
import type { Solution } from "../utils/types";

export function DiskSpaceModal({
  onClose,
  deleteOnQuit,
  setDeleteOnQuit,
  solutions,
  onClearSolutions,
}: {
  onClose: () => void;
  deleteOnQuit: boolean;
  setDeleteOnQuit: (value: boolean) => void;
  solutions: Solution[];
  onClearSolutions: () => void;
}) {
  const [report, setReport] = useState<DiskSpaceReport | null>(null);
  const [busy, setBusy] = useState(false);
  const shadeStartRef = useRef<EventTarget | null>(null);
  const shadeEndRef = useRef<EventTarget | null>(null);

  const refresh = () => {
    void getDiskSpaceReport(solutions).then(setReport);
  };
  useEffect(refresh, [solutions.length]);
  useEffect(() => {
    if (!deleteOnQuit) refresh();
  }, [deleteOnQuit]);

  const act = async (run: () => Promise<void>) => {
    setBusy(true);
    try {
      await run();
    } finally {
      setBusy(false);
      refresh();
    }
  };

  return (
    <div className="modal-shade modal-shade-top" onPointerDown={(e) => { shadeStartRef.current = e.target; }} onPointerUp={(e) => { shadeEndRef.current = e.target; }} onClick={() => {
      const startOutside = !shadeStartRef.current || !(shadeStartRef.current as Element).closest(".modal");
      const endOutside = !shadeEndRef.current || !(shadeEndRef.current as Element).closest(".modal");
      if (startOutside && endOutside) onClose();
      shadeStartRef.current = null;
      shadeEndRef.current = null;
    }}>
      <div className="modal disk-modal" onClick={(e) => e.stopPropagation()}>
        <button className="modal-close" onClick={onClose}>✕</button>
        <div className="modal-article">
          <h2>{t('modal.disk.title')}</h2>
          <div className="disk-total">
            <span>{t('modal.disk.total')}</span>
            <strong>{formatBytes(report?.total ?? 0)}</strong>
          </div>
          {report && report.appSize > 0 && (
            <div className="disk-row">
              <span>{t('modal.disk.appSize')}</span>
              <span className="disk-size">{formatBytes(report.appSize)}</span>
            </div>
          )}
          <div className="disk-row">
            <span>{t('modal.disk.solutions')}</span>
            <span className="disk-size">{formatBytes(report?.solutionBytes ?? 0)}</span>
            <button className="disk-action" title={t('modal.disk.clearSolutions')} disabled={busy || (report?.solutionBytes ?? 0) === 0} onClick={() => void act(async () => onClearSolutions())}>×</button>
          </div>
          <div className="disk-section-title">{t('modal.disk.tablesTitle')}</div>
          {report?.tables.length === 0 && <div className="disk-empty">{t('modal.disk.noTables')}</div>}
          {report?.tables.map((table) => (
            <div className="disk-row" key={table.name}>
              <span>{tableLabel(table.name)}</span>
              <span className="disk-size">{formatBytes(table.size)}</span>
              <button className="disk-action" title={t('modal.disk.deleteTable')} disabled={busy} onClick={() => void act(() => deleteTable(table.name))}>🗑</button>
            </div>
          ))}
          <label className="modal-check disk-check">
            <input type="checkbox" checked={deleteOnQuit} onChange={(e) => setDeleteOnQuit(e.target.checked)} />
            <span>{t('modal.disk.deleteOnQuit')}</span>
          </label>
        </div>
      </div>
    </div>
  );
}
