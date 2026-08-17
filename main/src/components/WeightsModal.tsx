import { useRef } from "react";
import { t } from "../i18n";
import { Icon } from "./Icon";
import { DEFAULT_WEIGHTS, DEFAULT_MOVE_VALUES } from "../utils";
import type { RatingWeights } from "../utils";

type WeightKey = keyof RatingWeights;

const WEIGHT_FIELDS: { key: WeightKey; labelKey: string; tooltipKey: string }[] = [
  { key: "w1", labelKey: "modal.weights.labels.w1", tooltipKey: "tooltips.weightW1" },
  { key: "w2", labelKey: "modal.weights.labels.w2", tooltipKey: "tooltips.weightW2" },
  { key: "w3", labelKey: "modal.weights.labels.w3", tooltipKey: "tooltips.weightW3" },
  { key: "w4", labelKey: "modal.weights.labels.w4", tooltipKey: "tooltips.weightW4" },
];

export function WeightsModal({
  onClose,
  weightOverrides,
  setWeightOverrides,
  moveValueOverrides,
  setMoveValueOverrides,
}: {
  onClose: () => void;
  weightOverrides: Partial<RatingWeights>;
  setWeightOverrides: (value: Partial<RatingWeights>) => void;
  moveValueOverrides: Record<string, number>;
  setMoveValueOverrides: (value: Record<string, number>) => void;
}) {
  const shadeStartRef = useRef<EventTarget | null>(null);
  const shadeEndRef = useRef<EventTarget | null>(null);

  const updateWeight = (key: WeightKey, raw: string) => {
    if (raw.trim() === "") {
      const next = { ...weightOverrides };
      delete next[key];
      setWeightOverrides(next);
      return;
    }
    const value = Number(raw);
    if (!Number.isFinite(value)) return;
    setWeightOverrides({ ...weightOverrides, [key]: value });
  };

  const updateMoveValue = (key: string, raw: string) => {
    if (raw.trim() === "") {
      const next = { ...moveValueOverrides };
      delete next[key];
      setMoveValueOverrides(next);
      return;
    }
    const value = Math.trunc(Number(raw));
    if (!Number.isFinite(value)) return;
    setMoveValueOverrides({ ...moveValueOverrides, [key]: value });
  };

  return (
    <div
      className="modal-shade modal-shade-top"
      onPointerDown={(e) => { shadeStartRef.current = e.target; }}
      onPointerUp={(e) => { shadeEndRef.current = e.target; }}
      onClick={() => {
        const startOutside = !shadeStartRef.current || !(shadeStartRef.current as Element).closest(".modal");
        const endOutside = !shadeEndRef.current || !(shadeEndRef.current as Element).closest(".modal");
        if (startOutside && endOutside) onClose();
        shadeStartRef.current = null;
        shadeEndRef.current = null;
      }}
    >
      <div className="modal weights-modal" onClick={(e) => e.stopPropagation()}>
        <button className="modal-close" onClick={onClose}><Icon name="close" /></button>
        <h2>{t('modal.weights.title')}</h2>
        <div className="weights-grid">
          {WEIGHT_FIELDS.map(({ key, labelKey, tooltipKey }) => {
            const changed = weightOverrides[key] !== undefined;
            return (
              <label key={key} className="weights-field" title={t(tooltipKey)}>
                <span className={"weights-eyebrow weights-eyebrow-label" + (changed ? " weights-eyebrow-changed" : "")}>
                  {t(labelKey)}{changed ? "*" : ""}
                </span>
                <input
                  type="number"
                  step="any"
                  placeholder={String(DEFAULT_WEIGHTS[key])}
                  value={weightOverrides[key] ?? ""}
                  onChange={(e) => updateWeight(key, e.target.value)}
                />
              </label>
            );
          })}
        </div>
        <hr className="weights-separator" />
        <p className="weights-table-caption">{t('modal.weights.tableCaption')}</p>
        <div className="weights-grid weights-grid-moves">
          {Object.entries(DEFAULT_MOVE_VALUES).map(([key, defaultValue]) => {
            const changed = moveValueOverrides[key] !== undefined;
            return (
              <label key={key} className="weights-field">
                <span className={"weights-eyebrow" + (changed ? " weights-eyebrow-changed" : "")}>
                  {key}{changed ? "*" : ""}
                </span>
                <input
                  type="number"
                  step="1"
                  placeholder={String(defaultValue)}
                  value={moveValueOverrides[key] ?? ""}
                  onChange={(e) => updateMoveValue(key, e.target.value)}
                />
              </label>
            );
          })}
        </div>
      </div>
    </div>
  );
}
