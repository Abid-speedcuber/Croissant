import { useEffect, useLayoutEffect, useRef, useState } from "react";
import { createPortal } from "react-dom";
import { loadSettings, saveSettings, loadFavorites, saveFavorites, writeOffloadedChunk, readOffloadedChunk, removeOffloadedChunk, clearOffloadedSolutions, isNativePlatform } from "./storage";
import { clearTableBlobs } from "./tableStore";
import { clearAllTables } from "./diskSpace";
import {
  CubeState, Modal as ModalType, FavoriteBin, Solution, OutputLine, RatingResult, TwoGenStatus,
  DisplaySolution, DropdownProps, CubeActions, TauriGlobal,
  twistable, getLayerR, getParityOdd, isGoodSquares, inCubeshape, doMove, tauri, validDepths, solverFlags,
  positionString, rawPosition, parsePosition, invertScramble, addCommas, applyNumericAlgorithm,
  abidify, abidSpacing, notationStyle, isKarnMode, OutputMode, OUTPUT_MODES, injectSliceIndicator, lineAlg, lineWithoutBracket, parseSolutionCounts,
  ratingScore, ratingSliceStart, solutionErgo, medianNormalize, normalizeLine, tooltips,
} from "./utils";
import { Modal } from './components/Modal';
import { Icon } from './components/Icon';
import { DiskSpaceModal } from './components/DiskSpaceModal';
import { t, LangCode, getLang, setLang } from './i18n';

const PAGE_SIZE_OPTIONS = [250, 500, 1000, 2000, 5000, 8000, 10000, 15000, 20000];
const OFFLOAD_CHUNK = 5000;
const TABLE_COL_WIDTHS = {
  wide: { hash: 72, num: 48, ergo: 52, minSol: 150 },
  compact: { hash: 34, num: 42, ergo: 50, minSol: 88 },
};
const computeTableCols = (
  width: number, metric: string, showErgo: boolean, compact: boolean,
): { hash: boolean; angle: boolean; move: boolean; slices: boolean; ergo: boolean; template: string } => {
  const sizes = compact ? TABLE_COL_WIDTHS.compact : TABLE_COL_WIDTHS.wide;
  const w = (key: string) => (key === "hash" ? sizes.hash : key === "ergo" ? sizes.ergo : sizes.num);
  const want = new Set<string>(["hash"]);
  if (metric === "Angle") want.add("angle");
  if (metric !== "Slice") want.add("move");
  want.add("slices");
  if (showErgo) want.add("ergo");
  const keepPriority = ["ergo", "angle", "move", "slices", "hash"];
  const cols = [...want];
  const total = (list: string[]) => list.reduce((sum, key) => sum + w(key), 0) + sizes.minSol;
  while (cols.length > 1 && total(cols) > width) {
    const drop = keepPriority.slice().reverse().find((key) => cols.includes(key));
    if (!drop) break;
    cols.splice(cols.indexOf(drop), 1);
  }
  const template = ["hash", "solution", "angle", "move", "slices", "ergo"]
    .map((key) => (key === "solution" ? "minmax(0, 1fr)" : cols.includes(key) ? `${w(key)}px` : null))
    .filter((part): part is string => !!part)
    .join(" ");
  return {
    hash: cols.includes("hash"),
    angle: cols.includes("angle"),
    move: cols.includes("move"),
    slices: cols.includes("slices"),
    ergo: cols.includes("ergo"),
    template,
  };
};
const computeTotalPages = (total: number, pageSize: number) => {
  const raw = Math.max(1, Math.ceil(total / pageSize));
  const remainder = total % pageSize;
  return remainder > 0 && remainder < pageSize * 0.1 ? Math.max(1, raw - 1) : raw;
};
const readBreakpoints = () => {
  const cs = getComputedStyle(document.documentElement);
  const read = (name: string) => {
    const value = parseFloat(cs.getPropertyValue(name));
    return Number.isFinite(value) ? value : 0;
  };
  return {
    tall: read("--bp-tall"),
    wide: read("--bp-wide"),
    semi: read("--bp-semi"),
    narrow: read("--bp-narrow"),
    panel: read("--bp-panel"),
    panelTiny: read("--bp-panel-tiny"),
  };
};
const solved = [
  0, 0, 8, 1, 1, 9, 2, 2, 10, 3, 3, 11, 12, 4, 4, 13, 5, 5, 14, 6, 6, 15, 7, 7,
];
const side = [
  [2, 3],
  [3, 4],
  [4, 5],
  [5, 2],
  [2, 5],
  [5, 4],
  [4, 3],
  [3, 2],
  [3],
  [4],
  [5],
  [2],
  [2],
  [5],
  [4],
  [3],
];
const colors = ["#333", "#fff", "#f00", "#00f", "#ff8600", "#0f0", "#888"];

function OptionDropdown({ id, label, title, value, options, disabled, open, setOpen, onChange }: DropdownProps) {
  return (
    <label className="option-dropdown-label" title={title}>
      {label}
      <div className="option-dropdown">
        <button
          type="button"
          disabled={disabled}
          aria-haspopup="listbox"
          aria-expanded={open}
          onMouseDown={(event) => event.preventDefault()}
          onClick={() => !disabled && setOpen(open ? null : id)}
        >
          <span>{value}</span>
          <span className="option-dropdown-arrow"><Icon name="chevronDown" size={10} /></span>
        </button>
        {open && !disabled && (
          <div className="option-dropdown-menu" role="listbox">
            {options.map((option) => (
              <button
                key={option}
                type="button"
                role="option"
                aria-selected={option === value}
                className={option === value ? "selected" : ""}
                onMouseDown={(event) => {
                  event.preventDefault();
                  onChange(option);
                  setOpen(null);
                }}
              >
                {option}
              </button>
            ))}
          </div>
        )}
      </div>
    </label>
  );
}


function Cube({
  onChange,
  actionsRef,
  onOptions,
}: {
  onChange: (s: CubeState, action?: string) => void;
  actionsRef: React.MutableRefObject<CubeActions | undefined>;
  onOptions: () => void;
}) {
  const [s, setS] = useState<CubeState>({
    position: [...solved],
    partial: Array(24).fill(0),
    middle: 1,
    middlePartial: 0,
  });
  const canvas = useRef<HTMLCanvasElement>(null);
  const [hovered, setHovered] = useState(-1);
  const [hoverProgress, setHoverProgress] = useState<Record<number, number>>(
    {},
  );
  const [selected, setSelected] = useState(-1);
  const update = (n: CubeState, action = "edit") => {
    setS(n);
    onChange(n, action);
  };
  const polar = (
    cx: number,
    cy: number,
    a: number,
    r: number,
  ): [number, number] => [
    cx + Math.cos((a * Math.PI) / 180) * r,
    cy - Math.sin((a * Math.PI) / 180) * r,
  ];
  const hulls = (
    start: number,
    end: number,
    cy: number,
    startAngle: number,
  ) => {
    const found: { piece: number; pts: number[][] }[] = [];
    let angle = startAngle;
    for (let i = start; i < end; ) {
      const x = s.position[i],
        corner = x < 8,
        pts = corner
          ? [
              [150, cy],
              polar(150, cy, angle, 75),
              polar(150, cy, angle - 30, 75 / 0.73205080756),
              polar(150, cy, angle - 60, 75),
            ]
          : [
              [150, cy],
              polar(150, cy, angle, 75),
              polar(150, cy, angle - 30, 75),
            ];
      found.push({ piece: i, pts });
      if (corner) {
        i++;
        angle -= 60;
      } else angle -= 30;
      i++;
    }
    return found;
  };
  const hit = (x: number, y: number) => {
    if (y < 235) {
      for (const h of hulls(0, 12, 125, -105)) {
        const c = canvas.current!.getContext("2d")!;
        c.beginPath();
        c.moveTo(h.pts[0][0], h.pts[0][1]);
        h.pts.slice(1).forEach((p) => c.lineTo(p[0], p[1]));
        c.closePath();
        if (c.isPointInPath(x, y)) return h.piece;
      }
      return -1;
    }
    if (y < 265) return x >= 77.25 && x <= 222.75 ? -2 : -1;
    for (const h of hulls(12, 24, 375, 105)) {
      const c = canvas.current!.getContext("2d")!;
      c.beginPath();
      c.moveTo(h.pts[0][0], h.pts[0][1]);
      h.pts.slice(1).forEach((p) => c.lineTo(p[0], p[1]));
      c.closePath();
      if (c.isPointInPath(x, y)) return h.piece;
    }
    return -1;
  };
  const draw = () => {
    const c = canvas.current?.getContext("2d");
    if (!c) return;
    c.clearRect(0, 0, 300, 500);
    const poly = (pts: number[][], fill: string, hover = 0) => {
      c.beginPath();
      c.moveTo(pts[0][0], pts[0][1]);
      pts.slice(1).forEach((p) => c.lineTo(p[0], p[1]));
      c.closePath();
      c.fillStyle = fill;
      c.fill();
      c.strokeStyle = "#000";
      c.lineWidth = 1;
      c.stroke();
      if (hover > 0) {
        const eased = hover * hover * (3 - 2 * hover);
        c.fillStyle = `rgba(255,255,255,${eased * (60 / 255)})`;
        c.fill();
      }
    };
    const layer = (
      start: number,
      end: number,
      cy: number,
      startAngle: number,
    ) => {
      let angle = startAngle;
      let sel: number[][] | undefined;
      for (let i = start; i < end; ) {
        const x = s.position[i],
          corner = x < 8,
          h = hoverProgress[i] || 0,
          p1 = polar(150, cy, angle, 50),
          p2 = polar(150, cy, angle - 30, 50 / (corner ? 0.73205080756 : 1)),
          p3 = corner ? polar(150, cy, angle - 60, 50) : p2,
          q1 = polar(150, cy, angle, 75),
          q2 = polar(150, cy, angle - 30, 75 / (corner ? 0.73205080756 : 1)),
          q3 = corner ? polar(150, cy, angle - 60, 75) : q2;
        const faceColor = corner
          ? colors[x < 4 ? 0 : 1]
          : colors[x < 12 ? 0 : 1];
        poly(
          corner ? [[150, cy], p1, p2, p3] : [[150, cy], p1, p2],
          s.partial[i] > 1 ? colors[6] : faceColor,
          h,
        );
        poly(
          [p1, q1, q2, p2],
          s.partial[i] > 0 ? colors[6] : colors[side[x][0]],
          h,
        );
        if (corner)
          poly(
            [p2, q2, q3, p3],
            s.partial[i] > 0 ? colors[6] : colors[side[x][1]],
            h,
          );
        if (selected === i)
          sel = corner ? [[150, cy], q1, q2, q3] : [[150, cy], q1, q2];
        if (corner) {
          i++;
          angle -= 60;
        } else angle -= 30;
        i++;
      }
      if (sel) {
        c.beginPath();
        c.moveTo(sel[0][0], sel[0][1]);
        sel.slice(1).forEach((p) => c.lineTo(p[0], p[1]));
        c.closePath();
        c.strokeStyle = "#ffff00";
        c.lineWidth = 3;
        c.stroke();
      }
    };
    layer(0, 12, 125, -105);
    layer(12, 24, 375, 105);
    const eqHover = hoverProgress[-2] || 0;
    poly(
      [
        [77.25, 235],
        [129, 235],
        [129, 265],
        [77.25, 265],
      ],
      colors[2],
      eqHover,
    );
    poly(
      [
        [129, 235],
        [s.middle === -1 ? 171 : 222.75, 235],
        [s.middle === -1 ? 171 : 222.75, 265],
        [129, 265],
      ],
      s.middle === 0 ? colors[6] : colors[s.middle === 1 ? 2 : 4],
      eqHover,
    );
  };
  useEffect(() => {
    draw();
  }, [s, hoverProgress, selected]);
  useEffect(() => {
    let frame = 0, stopped = false, frames = 0;
    const tick = () => {
      setHoverProgress((old) => {
        const next: { [key: number]: number } = { ...old };
        let changed = false;
        for (const key of new Set([...Object.keys(next).map(Number), hovered])) {
          const target = key === hovered ? 1 : 0, value = next[key] || 0;
          const valueNext = target ? Math.min(1, value + 0.08) : Math.max(0, value - 0.08);
          if (valueNext !== value) { next[key] = valueNext; changed = true; }
        }
        return changed ? next : old;
      });
      if (!stopped && ++frames < 16) frame = requestAnimationFrame(tick);
    };
    frame = requestAnimationFrame(tick);
    return () => { stopped = true; cancelAnimationFrame(frame); };
  }, [hovered]);
  const invoke = (key: keyof CubeActions, actionOverride?: string) => {
    if (key === "reset") {
      setSelected(-1);
      update({
        position: [...solved],
        partial: Array(24).fill(0),
        middle: 1,
        middlePartial: 0,
      }, actionOverride ?? "reset");
      return;
    }
    setSelected(-1);
    update(doMove(s, key), key);
  };
  actionsRef.current = {
    u: () => invoke("u"),
    up: () => invoke("up"),
    d: () => invoke("d"),
    dp: () => invoke("dp"),
    slice: () => invoke("slice"),
    reset: () => invoke("reset"),
    set: (next) => {
      setSelected(-1);
      update(next, "set");
    },
  };
  useEffect(() => {
    const key = (e: KeyboardEvent) => {
      const target = e.target as HTMLElement | null;
      if (target?.matches("input, textarea, select, [contenteditable=true]")) return;
      const map: Record<string, keyof CubeActions> = {
        i: "slice",
        k: "slice",
        j: "u",
        f: "up",
        s: "d",
        l: "dp",
        h: "up",
        g: "u",
        w: "dp",
        o: "d",
        escape: "reset",
      };
      const action = map[e.key.toLowerCase()];
      if (action) {
        e.preventDefault();
        invoke(action, action === "reset" ? "escape" : undefined);
      }
    };
    window.addEventListener("keydown", key);
    return () => window.removeEventListener("keydown", key);
  }, [s]);
  const pointer = (e: React.MouseEvent) => {
    const r = canvas.current!.getBoundingClientRect();
    return [
      ((e.clientX - r.left) * 300) / r.width,
      ((e.clientY - r.top) * 500) / r.height,
    ];
  };
  const click = (e: React.MouseEvent) => {
    const [x, y] = pointer(e),
      piece = hit(x, y);
    if (piece === -2) {
      const middle =
        e.button === 2
          ? s.middle === 1
            ? 0
            : s.middle === 0
              ? -1
              : 1
          : s.middle === 1
            ? -1
            : s.middle === -1
              ? 0
              : 1;
      update({ ...s, middle }, "edit");
      return;
    }
    if (piece < 0) return;
    if (e.button === 2) {
      const partial = [...s.partial];
      partial[piece] = (partial[piece] + 1) % 3;
      if (piece < 23 && s.position[piece] === s.position[piece + 1])
        partial[piece + 1] = partial[piece];
      update({ ...s, partial }, "edit");
      return;
    }
    if (selected < 0) {
      setSelected(piece);
      return;
    }
    const p = [...s.position],
      q = [...s.partial],
      selCorner = selected < 23 && p[selected] === p[selected + 1],
      pieCorner = piece < 23 && p[piece] === p[piece + 1];
    if (selCorner !== pieCorner) {
      setSelected(piece);
      return;
    }
    if (selected === piece) {
      setSelected(-1);
      return;
    }
    if (selCorner) {
      [p[selected + 1], p[piece + 1]] = [p[piece + 1], p[selected + 1]];
      [q[selected + 1], q[piece + 1]] = [q[piece + 1], q[selected + 1]];
    }
    [p[selected], p[piece]] = [p[piece], p[selected]];
    [q[selected], q[piece]] = [q[piece], q[selected]];
    setSelected(-1);
    update({ ...s, position: p, partial: q }, "edit");
  };
  return (
    <div className="cube-holder">
      <canvas
        ref={canvas}
        width={300}
        height={500}
        style={{
          cursor: hovered !== -1 ? "pointer" : "default",
        }}
        onMouseMove={(e) => {
          const [x, y] = pointer(e);
          setHovered(hit(x, y));
        }}
        onMouseLeave={() => setHovered(-1)}
        onMouseDown={click}
        onContextMenu={(e) => e.preventDefault()}
      />
      <button className="cube-reset" onClick={() => invoke("reset")}>
        Reset
      </button>
      <button className="cube-options" onClick={onOptions}>
        Options
      </button>
    </div>
  );
}

function Pill({
  label,
  items,
  value,
  set,
}: {
  label: string;
  items: string[];
  value: string;
  set: (v: string) => void;
}) {
  return (
    <div className="option-row">
      <span>{label}</span>
      <div className="pill">
        {items.map((x) => (
          <button
            className={x === value ? "active" : ""}
            key={x}
            onClick={() => set(x)}
          >
            {x}
          </button>
        ))}
      </div>
    </div>
  );
}
export default function App() {
  const cubeActions = useRef<CubeActions | undefined>(undefined);
  const modeControlRef = useRef<HTMLDivElement | null>(null);
  const stateRef = useRef<CubeState>({ position: [...solved], partial: Array(24).fill(0), middle: 1, middlePartial: 0 });
  const ignoreHistory = useRef(false), seenRaw = useRef(new Set<string>()), seenDisplay = useRef(new Set<string>());
  const lastSolvePosition = useRef("A1B2C3D45E6F7G8H-");
  const lastSolveCubeShape = useRef(false);
  const solutionsRef = useRef<Solution[]>([]);
  const outputLinesRef = useRef<OutputLine[]>([]);
  const followTerminalRef = useRef(true);
  const firstSolutionAt = useRef(0);
  const solveStartTimeRef = useRef(0);
  const solveStopTimeRef = useRef(0);
  const debugStatsRef = useRef<{ solutionTimestamps: number[] }>({ solutionTimestamps: [] });
  const rateHistoryRef = useRef<{ t: number; sol: number; node: number }[]>([]);
  const progressNodesRef = useRef(0);
  const progressRateRef = useRef(0);
  const lastProgressNodesRef = useRef(0);
  const lastProgressAtRef = useRef(0);
  const lineQueue = useRef<Promise<void>>(Promise.resolve());
  const outputIdleTimer = useRef<number | undefined>(undefined);
  const renderFrame = useRef<number | undefined>(undefined);
  const runningRef = useRef(false);
  const solveRunId = useRef(0);
  const preIgnoreMiddle = useRef(1);
  const slicePending = useRef<string[]>([]), sliceTimer = useRef<number | undefined>(undefined);
  const stopped = useRef(false);
  const settingsReady = useRef(false);
  const favoritesReady = useRef(false);
  const favShadeStartRef = useRef<EventTarget | null>(null);
  const favShadeEndRef = useRef<EventTarget | null>(null);
  const toastTimerRef = useRef<number | undefined>(undefined);
  const [menu, setMenu] = useState(false),
    [lang, setLangState] = useState<LangCode>(getLang()),
    [toast, setToast] = useState<string | null>(null),
    [modal, setModal] = useState<ModalType>(null),
    [modeMenu, setModeMenu] = useState(false),
    [openDropdown, setOpenDropdown] = useState<string | null>(null),
    [input, setInput] = useState(""),
    [mode, setMode] = useState("SCRAMBLE"),
    [cubeState, setCubeState] = useState<CubeState>({ position: [...solved], partial: Array(24).fill(0), middle: 1, middlePartial: 0 }),
    [metric, setMetric] = useState("Slice"),
    [two, setTwo] = useState("None"),
    [angle, setAngle] = useState("None"),
    [normalize, setNormalize] = useState("None"),
    [all, setAll] = useState(true),
    [generator, setGenerator] = useState(false),
    [cubeShapeMemory, setCubeShapeMemory] = useState(false),
    [ignoreMiddle, setIgnoreMiddle] = useState(false),
    [outputMode, setOutputMode] = useState<OutputMode>("cskarn"),
    [outputLines, setOutputLines] = useState<OutputLine[]>([]),
    [statusLines, setStatusLines] = useState<string[]>([]),
    [solutions, setSolutions] = useState<Solution[]>([]),
    [runCubeShape, setRunCubeShape] = useState(false),
    [running, setRunning] = useState(false),
    [suboptimal, setSuboptimal] = useState(0),
    [depths, setDepths] = useState(""),
    [maxX, setMaxX] = useState(false),
    [maxXValue, setMaxXValue] = useState(3),
    [maxY, setMaxY] = useState(false),
    [maxYValue, setMaxYValue] = useState(3),
    [maxTotal, setMaxTotal] = useState(false),
    [maxTotalValue, setMaxTotalValue] = useState(6),
    [inputError, setInputError] = useState(""),
    [undo, setUndo] = useState<string[]>([]), [redo, setRedo] = useState<string[]>([]),
    [tableView, setTableView] = useState(false), [expanded, setExpanded] = useState(false),
    [mobileOptionsOpen, setMobileOptionsOpen] = useState(false), [mobileOutputOpen, setMobileOutputOpen] = useState(false),
    [tableWidth, setTableWidth] = useState(0),
    [zoom, setZoom] = useState(1),
    [abidNotation, setAbidNotation] = useState(false),
    [ignoreTransforms, setIgnoreTransforms] = useState(false), [debugOutput, setDebugOutput] = useState(false),
    [pageSize, setPageSize] = useState(1000), [showAll, setShowAll] = useState(false),
    [page, setPage] = useState(0),
    [pageInput, setPageInput] = useState("1");
  const [useLessRam, _setUseLessRam] = useState(false);
  const karn = isKarnMode(outputMode);
  const smartKarn = outputMode === "cskarn";
  const [offloadedTotal, setOffloadedTotal] = useState(0);
  const [pendingTailCount, setPendingTailCount] = useState(0);
  const [isRestoring, setIsRestoring] = useState(false);
  const setUseLessRam = (value: boolean) => { useLessRamRef.current = value; _setUseLessRam(value); };
  const totalCount = solutions.length + offloadedTotal + pendingTailCount;
  const deleteTablesOnQuitRef = useRef(!isNativePlatform());
  const [deleteTablesOnQuit, _setDeleteTablesOnQuit] = useState(!isNativePlatform());
  const deleteTablesOnQuitV2 = deleteTablesOnQuit;
  const setDeleteTablesOnQuit = (value: boolean) => {
    deleteTablesOnQuitRef.current = value;
    _setDeleteTablesOnQuit(value);
    if (value && !isNativePlatform()) void clearTableBlobs();
  };
  const [diskOpen, setDiskOpen] = useState(false);
  const [showAllConfirm, setShowAllConfirm] = useState(false);
  const [favoritesOpen, setFavoritesOpen] = useState(false),
    [favorites, setFavorites] = useState<Record<string, FavoriteBin>>({}),
    [pendingDeletes, setPendingDeletes] = useState<Record<string, number>>({});
  const [favoritesClosing, setFavoritesClosing] = useState(false);
  const [favoritesOpening, setFavoritesOpening] = useState(false);
  const favModalRef = useRef<HTMLDivElement>(null);
  const favClosingOriginRef = useRef({ x: 50, y: 50 });
  const favOpeningOriginRef = useRef({ x: 50, y: 50 });
  const [contextMenu, setContextMenu] = useState<{ x: number; y: number; alg: string } | null>(null);
  const [twoGenStatus, setTwoGenStatus] = useState<TwoGenStatus>({ compatibility: 0, cornersTwo: false, cornersPseudo: false });
  const [followTerminal, setFollowTerminal] = useState(true);
  const [completedWhilePaused, setCompletedWhilePaused] = useState(false);
  const [tableBusyMessage, setTableBusyMessage] = useState("");
  const [tableBusyTick, setTableBusyTick] = useState(0);
  const [outputToolsFaded, setOutputToolsFaded] = useState(false);
  const [debugTick, setDebugTick] = useState(0);
  // Search/filter panel for narrowing the terminal & table down to matching algs.
  const [filterOpen, setFilterOpen] = useState(false);
  const [filterQuery, setFilterQuery] = useState("");
  const [filterMatchCase, setFilterMatchCase] = useState(true);
  const [filterRegexMode, setFilterRegexMode] = useState(false);
  const [filterResults, setFilterResults] = useState<Solution[] | null>(null);
  const [filterAppliedQuery, setFilterAppliedQuery] = useState("");
  const [filterSearching, setFilterSearching] = useState(false);
  const [filterInvalid, setFilterInvalid] = useState(false);
  const filterSearchIdRef = useRef(0);
  const filterInputRef = useRef<HTMLInputElement | null>(null);
  useEffect(() => {
    try {
      const storedCase = localStorage.getItem("sq1opt.filterMatchCase");
      if (storedCase !== null) setFilterMatchCase(storedCase === "1");
      const storedRegex = localStorage.getItem("sq1opt.filterRegex");
      if (storedRegex !== null) setFilterRegexMode(storedRegex === "1");
    } catch { /* localStorage unavailable */ }
  }, []);
  useEffect(() => {
    try { localStorage.setItem("sq1opt.filterMatchCase", filterMatchCase ? "1" : "0"); } catch { /* localStorage unavailable */ }
  }, [filterMatchCase]);
  useEffect(() => {
    try { localStorage.setItem("sq1opt.filterRegex", filterRegexMode ? "1" : "0"); } catch { /* localStorage unavailable */ }
  }, [filterRegexMode]);
  useEffect(() => {
    if (filterOpen) requestAnimationFrame(() => filterInputRef.current?.focus());
  }, [filterOpen]);
  // Independent of filterOpen so hiding the overlay (✕) keeps the filter applied.
  const filterActive = filterAppliedQuery.trim() !== "" && filterResults !== null;
  const beginCloseFavorites = () => {
    if (favoritesClosing) return;
    const heartBtn = document.querySelector<HTMLElement>('.top-favorites-button');
    const modalEl = favModalRef.current;
    if (heartBtn && modalEl) {
      const hr = heartBtn.getBoundingClientRect();
      const mr = modalEl.getBoundingClientRect();
      favClosingOriginRef.current = {
        x: ((hr.left + hr.width / 2 - mr.left) / mr.width) * 100,
        y: ((hr.top + hr.height / 2 - mr.top) / mr.height) * 100,
      };
    }
    setFavoritesClosing(true);
  };
  const onFavCloseAnimEnd = (e: React.AnimationEvent) => {
    if (e.animationName !== "fav-modal-out") return;
    setFavoritesOpen(false);
    setFavoritesClosing(false);
    history.back();
  };
  const onFavOpenAnimEnd = (e: React.AnimationEvent) => {
    if (e.animationName !== "fav-modal-in") return;
    setFavoritesOpening(false);
  };
  useLayoutEffect(() => {
    if (!favoritesOpening || !favModalRef.current) return;
    const heartBtn = document.querySelector<HTMLElement>('.top-favorites-button');
    const modalEl = favModalRef.current;
    if (heartBtn && modalEl) {
      const hr = heartBtn.getBoundingClientRect();
      const mw = modalEl.offsetWidth, mh = modalEl.offsetHeight;
      const ml = (window.innerWidth - mw) / 2, mt = (window.innerHeight - mh) / 2;
      favOpeningOriginRef.current = {
        x: ((hr.left + hr.width / 2 - ml) / mw) * 100,
        y: ((hr.top + hr.height / 2 - mt) / mh) * 100,
      };
      modalEl.style.transformOrigin = `${favOpeningOriginRef.current.x}% ${favOpeningOriginRef.current.y}%`;
      modalEl.style.animationPlayState = 'running';
    }
  }, [favoritesOpening]);
  /* ============================================================================
   * TERMINAL / OUTPUT PANEL BEHAVIOR
   * ============================================================================
   *
   * The terminal is the output panel that shows solver log lines and solutions.
   * There are two views: terminal (raw text) and table (structured solutions).
   * Key refs/state:
   *   followTerminal / followTerminalRef  – whether auto-scroll is active
   *   completedWhilePaused               – solve finished while user had scrolled up
   *   isSwitchingViewRef                  – suppresses scroll during terminal↔table toggle
   *   terminalScrollPositionRef           – remembered scroll offset when switching views
   *   tableScrollPositionRef              – same, for table view
   *
   * ── SOLVE START (line ~1493) ──────────────────────────────────────────────
   *   Resets everything: followTerminal = true, tableView = false, scroll
   *   positions = 0, completedWhilePaused = false.
   *
   * ── DURING SOLVE (running = true) ─────────────────────────────────────────
   *   • New outputLines / statusLines / solutions arrive continuously.
   *   • The useEffect at line ~1230 fires on every content change. If
   *     followTerminal is true, it schedules a requestAnimationFrame to scroll
   *     the terminal to the bottom.
   *   • Scrolling UP (onWheel with deltaY < 0) sets followTerminal = false,
   *     which stops auto-scroll. A ⌄ button appears to re-enable it.
   *   • Scrolling up via scrollbar drag (no onWheel) is detected by
   *     handleTerminalScroll: it compares the new scrollTop with the previous
   *     one — only if scrollTop decreased (user actually scrolled up) AND the
   *     user is >50px above the bottom does it disable followTerminal. This
   *     avoids false triggers from content being added (scrollHeight grows
   *     but scrollTop stays the same). Re-enabling is only via the ⌄ button.
   *
   * ── SOLVE FINISHES (line ~1520) ──────────────────────────────────────────
   *   • If followTerminal was still true (user never scrolled away):
   *     → auto-switches to table view
   *     → shows busy messages while normalizing / building table
   *   • If followTerminal was false (user scrolled up during solve):
   *     → sets completedWhilePaused = true
   *     → shows ⊞ button to manually switch to table view later
   *
   * ── ⌄ BUTTON (line ~1830) ────────────────────────────────────────────────
   *   Calls scrollTerminalToBottom() which:
   *     1. Sets followTerminal = true
   *     2. Sets completedWhilePaused = false
   *     3. Scrolls the div to the bottom
   *
   * ── ⊞ BUTTON (line ~1829) ────────────────────────────────────────────────
   *   Appears when completedWhilePaused is true (solve finished while user
   *   was scrolled up). Clicking switches to table view.
   *
   * ── VIEW SWITCHING (terminal ↔ table) ─────────────────────────────────────
   *   switchToTableMode / switchToTerminalMode save the current scroll
   *   position, set isSwitchingViewRef = true, then toggle tableView.
   *   The useLayoutEffect at line ~1223 restores the saved position.
   *   isSwitchingViewRef is cleared by the useEffect at line ~1233 so that
   *   the auto-scroll effect does NOT fire during the switch.
   *
   * ── COMMON PITFALL ────────────────────────────────────────────────────────
   *   The rAF in the auto-scroll effect must NOT call scrollTerminalToBottom()
   *   directly, because that function unconditionally sets followTerminal=true,
   *   which would override a user who just scrolled up. Instead, the rAF
   *   callback must check followTerminalRef.current before scrolling.
   * ============================================================================
   */
  const terminalTextRef = useRef<HTMLDivElement>(null);
  const tableContainerRef = useRef<HTMLDivElement>(null);
  // True only while the user is actively touching/dragging the terminal
  // (mousedown or touch), so fast content growth alone can't be mistaken
  // for a user-initiated scroll-up (see handleTerminalScroll below).
  const terminalUserActiveRef = useRef(false);
  const terminalUserActiveTimeoutRef = useRef<number | undefined>(undefined);
  const markTerminalUserActive = () => {
    terminalUserActiveRef.current = true;
    if (terminalUserActiveTimeoutRef.current !== undefined) {
      window.clearTimeout(terminalUserActiveTimeoutRef.current);
      terminalUserActiveTimeoutRef.current = undefined;
    }
  };
  const scheduleTerminalUserInactive = () => {
    if (terminalUserActiveTimeoutRef.current !== undefined) window.clearTimeout(terminalUserActiveTimeoutRef.current);
    terminalUserActiveTimeoutRef.current = window.setTimeout(() => {
      terminalUserActiveRef.current = false;
      terminalUserActiveTimeoutRef.current = undefined;
    }, 400);
  };
  const terminalScrollPositionRef = useRef(0);
  const tableScrollPositionRef = useRef(0);
  const pageScrollEdgeRef = useRef<"top" | "bottom">("top");
  const pageInputFocused = useRef(false);
  const pageSwitcherRef = useRef<HTMLDivElement>(null);
  const pageInputRef = useRef<HTMLInputElement>(null);
  // Intentional feature by Abid: the page-switcher pill stays opaque as long as
  // autoscroll is running, and flashes opaque on overscroll page changes. After
  // autoscroll stops or the flash is triggered, it returns to its transparent
  // state once the cooldown below has elapsed.
  const PAGE_SWITCHER_COOLDOWN_MS = 2000;
  const [pageSwitcherOpaque, setPageSwitcherOpaque] = useState(false);
  const pageSwitcherHideTimerRef = useRef<number | undefined>(undefined);
  const touchNavRef = useRef<{ startY: number; lastY: number; moved: boolean; node: HTMLDivElement | null } | null>(null);
  const terminalPageRef = useRef(0);
  const tablePageRef = useRef(0);
  const restoreScrollRef = useRef<number | null>(null);
  const firstTableSwitchAfterSolveRef = useRef(true);
  const isSwitchingViewRef = useRef(false);
  const zoomRef = useRef(1);
  const cubeColumnRef = useRef<HTMLDivElement>(null);
  const tableMetricRef = useRef("Slice");
  const mainGridRef = useRef<HTMLDivElement>(null);
  const optionsPanelRef = useRef<HTMLDivElement>(null);
  const useLessRamRef = useRef(false);
  const pageRef = useRef(0);
  const pageSizeRef = useRef(1000);
  const showAllRef = useRef(false);
  const ramOffsetRef = useRef(0);
  const offloadedTotalRef = useRef(0);
  const offloadedChunksRef = useRef<Map<number, number>>(new Map());
  const pendingTailRef = useRef<Solution[] | null>(null);
  const offloadBusyRef = useRef(false);
  const offloadPendingRef = useRef(false);

  useEffect(() => {
    const el = document.documentElement;
    const update = () => {
      const bp = readBreakpoints();
      el.classList.toggle("tall-viewport", window.innerHeight / zoomRef.current >= bp.tall);
    };
    update();
    window.addEventListener("resize", update);
    return () => window.removeEventListener("resize", update);
  }, []);
  useEffect(() => {
    zoomRef.current = zoom;
    document.documentElement.classList.toggle("tall-viewport", window.innerHeight / zoom >= readBreakpoints().tall);
  }, [zoom]);
  useEffect(() => {
    const updateBreakpoints = () => {
      const effW = window.innerWidth / zoomRef.current;
      const bp = readBreakpoints();
      document.documentElement.classList.toggle("bp-720", effW <= bp.wide);
      document.documentElement.classList.toggle("bp-620", effW <= bp.semi);
      document.documentElement.classList.toggle("bp-460", effW <= bp.narrow);
    };
    updateBreakpoints();
    window.addEventListener("resize", updateBreakpoints);
    return () => window.removeEventListener("resize", updateBreakpoints);
  }, []);
  useEffect(() => {
    const update = () => {
      const effW = window.innerWidth / zoomRef.current;
      const bp = readBreakpoints();
      document.documentElement.classList.toggle("bp-720", effW <= bp.wide);
      document.documentElement.classList.toggle("bp-620", effW <= bp.semi);
      document.documentElement.classList.toggle("bp-460", effW <= bp.narrow);
    };
    update();
  }, [zoom]);
  useLayoutEffect(() => {
    const measure = () => {
      const col = cubeColumnRef.current;
      const grid = mainGridRef.current;
      if (col && grid) {
        grid.style.setProperty("--cube-h", col.scrollHeight + "px");
      }
    };
    measure();
    window.addEventListener("resize", measure);
    return () => window.removeEventListener("resize", measure);
  });
  useEffect(() => {
    const measure = () => {
      const col = cubeColumnRef.current;
      const grid = mainGridRef.current;
      if (col && grid) {
        grid.style.setProperty("--cube-h", col.scrollHeight + "px");
      }
    };
    measure();
  }, [zoom, running, undo.length, redo.length, tableView, expanded]);
  useEffect(() => {
    const anyOpen = modal !== null || favoritesOpen;
    if (anyOpen) {
      history.pushState({ modal: true }, "");
    }
    const onPop = () => {
      setModal(null);
      setShowAllConfirm(false);
      setFavoritesOpen(false);
      setFavoritesClosing(false);
    };
    window.addEventListener("popstate", onPop);
    return () => window.removeEventListener("popstate", onPop);
  }, [modal, favoritesOpen]);
  useEffect(() => {
    const node = tableContainerRef.current;
    if (!node || !tableView) return;
    const measure = () => setTableWidth(node.clientWidth);
    measure();
    const observer = new ResizeObserver(measure);
    observer.observe(node);
    return () => observer.disconnect();
  }, [tableView]);
  useEffect(() => {
    const panel = optionsPanelRef.current;
    if (!panel) return;
    const update = () => {
      const width = panel.clientWidth;
      const bp = readBreakpoints();
      panel.classList.toggle("panel-narrow", width <= bp.panel);
      panel.classList.toggle("panel-wide", width > bp.panel);
      panel.classList.toggle("panel-tiny", width <= bp.panelTiny);
    };
    update();
    const observer = new ResizeObserver(update);
    observer.observe(panel);
    return () => observer.disconnect();
  }, []);
  useEffect(() => {
    const handlePointerDown = (event: MouseEvent) => {
      if (!modeControlRef.current?.contains(event.target as Node)) {
        setModeMenu(false);
      }
      if (!(event.target as Element | null)?.closest(".option-dropdown")) {
        setOpenDropdown(null);
      }
      if (!(event.target as Element | null)?.closest(".top-menu-wrap")) {
        setMenu(false);
      }
      if (!contextMenu) return;
      if ((event.target as Element | null)?.closest(".solution-context")) return;
      setContextMenu(null);
    };
    window.addEventListener("pointerdown", handlePointerDown);
    return () => window.removeEventListener("pointerdown", handlePointerDown);
  }, [contextMenu]);
  useEffect(() => {
    const onMouseUp = () => scheduleTerminalUserInactive();
    window.addEventListener("mouseup", onMouseUp);
    return () => window.removeEventListener("mouseup", onMouseUp);
  }, []);
  const finalizeSliceHistory = () => {
    const pending = slicePending.current;
    if (!pending.length) return;
    const saved = pending.length % 2 === 0 && pending.length >= 2
      ? [pending[0], pending[pending.length - 1]] : [pending[0]];
    setUndo((old) => [...old, ...saved].slice(-64));
    setRedo([]);
    slicePending.current = [];
    sliceTimer.current = undefined;
  };
  const onCubeChange = (next: CubeState, action = "edit") => {
    const changed = positionString(next) !== positionString(stateRef.current);
    if (!ignoreHistory.current && changed) {
      if (action === "slice") {
        slicePending.current.push(positionString(stateRef.current));
        if (sliceTimer.current !== undefined) window.clearTimeout(sliceTimer.current);
        sliceTimer.current = window.setTimeout(finalizeSliceHistory, 600);
      } else if (action !== "escape") {
        setUndo((old) => [...old, positionString(stateRef.current)].slice(-64));
        setRedo([]);
      }
    }
    stateRef.current = next;
    setCubeState(next);
    setIgnoreMiddle(next.middle === 0);
  };
  useEffect(() => () => {
    if (sliceTimer.current !== undefined) window.clearTimeout(sliceTimer.current);
    if (renderFrame.current !== undefined) cancelAnimationFrame(renderFrame.current);
    if (pageSwitcherHideTimerRef.current !== undefined) window.clearTimeout(pageSwitcherHideTimerRef.current);
  }, []);
  const scrollTerminalToBottom = () => {
    const node = terminalTextRef.current;
    if (!node) return;
    followTerminalRef.current = true;
    setFollowTerminal(true);
    setCompletedWhilePaused(false);
    if (!showAll) {
      const lastPage = Math.max(0, totalPages - 1);
      pageScrollEdgeRef.current = "bottom";
      setPage(lastPage);
    }
    node.scrollTop = node.scrollHeight;
  };
  const openMobileOutput = () => {
    setMobileOutputOpen(true);
    requestAnimationFrame(scrollTerminalToBottom);
  };
  // Intentional feature by Abid: temporarily switch the page-switcher pill to
  // opaque so overscroll page changes are noticed, then let it fade back to
  // transparent after the cooldown. While autoscroll is running the pill is
  // already kept opaque, so no flash is needed there.
  const flashPageSwitcher = () => {
    if (followTerminalRef.current) return;
    if (pageSwitcherHideTimerRef.current !== undefined) {
      window.clearTimeout(pageSwitcherHideTimerRef.current);
      pageSwitcherHideTimerRef.current = undefined;
    }
    setPageSwitcherOpaque(true);
    pageSwitcherHideTimerRef.current = window.setTimeout(() => {
      pageSwitcherHideTimerRef.current = undefined;
      setPageSwitcherOpaque(false);
    }, PAGE_SWITCHER_COOLDOWN_MS);
  };
  const goToPage = (next: number, edge: "top" | "bottom") => {
    if (!filterActive && useLessRamRef.current && !isViewInRam(next, totalCountRefs())) setIsRestoring(true);
    setPage(next);
    pageScrollEdgeRef.current = edge;
    if (running && next === totalPages - 1) {
      followTerminalRef.current = true;
      setFollowTerminal(true);
      setCompletedWhilePaused(false);
    } else {
      followTerminalRef.current = false;
      setFollowTerminal(false);
    }
  };
  const commitPageInput = () => {
    const n = parseInt(pageInput, 10);
    if (Number.isFinite(n)) {
      const target = Math.min(totalPages, Math.max(1, n));
      goToPage(target - 1, "top");
    }
    setPageInput(String(clampedPage + 1));
  };
  const handleTerminalWheel = (event: React.WheelEvent<HTMLDivElement>) => {
    const node = terminalTextRef.current;
    if (!node) return;
    if (event.deltaY < 0) {
      if (running) {
        followTerminalRef.current = false;
        setFollowTerminal(false);
      }
      if (node.scrollHeight > node.clientHeight + 4 && node.scrollTop <= 1 && clampedPage > 0) { goToPage(clampedPage - 1, "bottom"); flashPageSwitcher(); }
    } else if (event.deltaY > 0) {
      const atBottom = node.scrollHeight - node.scrollTop - node.clientHeight < 4;
      if (node.scrollHeight > node.clientHeight + 4 && atBottom && clampedPage < totalPages - 1) { goToPage(clampedPage + 1, "top"); flashPageSwitcher(); }
    }
  };
  const handleTableWheel = (event: React.WheelEvent<HTMLDivElement>) => {
    const node = tableContainerRef.current;
    if (!node) return;
    if (event.deltaY < 0 && node.scrollHeight > node.clientHeight + 4 && node.scrollTop <= 1 && clampedPage > 0) { goToPage(clampedPage - 1, "bottom"); flashPageSwitcher(); }
    else if (event.deltaY > 0 && node.scrollHeight > node.clientHeight + 4 && node.scrollHeight - node.scrollTop - node.clientHeight < 4 && clampedPage < totalPages - 1) { goToPage(clampedPage + 1, "top"); flashPageSwitcher(); }
  };
  const handleTouchStart = (event: React.TouchEvent<HTMLDivElement>) => {
    const node = tableView ? tableContainerRef.current : terminalTextRef.current;
    if (!node) return;
    markTerminalUserActive();
    const touch = event.touches[0];
    touchNavRef.current = { startY: touch.clientY, lastY: touch.clientY, moved: false, node };
  };
  const handleTouchMove = (event: React.TouchEvent<HTMLDivElement>) => {
    const state = touchNavRef.current;
    if (!state) return;
    state.lastY = event.touches[0].clientY;
    state.moved = true;
  };
  const handleTouchEnd = () => {
    scheduleTerminalUserInactive();
    const state = touchNavRef.current;
    touchNavRef.current = null;
    const node = state?.node;
    if (!state || !state.moved || !node) return;
    const dy = state.lastY - state.startY;
    if (dy > 30) {
      if (node.scrollHeight > node.clientHeight + 4 && node.scrollTop <= 1 && clampedPage > 0) { goToPage(clampedPage - 1, "bottom"); flashPageSwitcher(); }
    } else if (dy < -30) {
      const atBottom = node.scrollHeight - node.scrollTop - node.clientHeight < 4;
      if (node.scrollHeight > node.clientHeight + 4 && atBottom && clampedPage < totalPages - 1) { goToPage(clampedPage + 1, "top"); flashPageSwitcher(); }
    }
  };
  const handleTerminalScroll = () => {
    const node = terminalTextRef.current;
    if (!node) return;
    const prev = terminalScrollPositionRef.current;
    const next = node.scrollTop;
    terminalScrollPositionRef.current = next;
    if (running && next < prev) {
      const nearBottom = node.scrollHeight - next - node.clientHeight < 50;
      if (!nearBottom && followTerminalRef.current && terminalUserActiveRef.current) {
        followTerminalRef.current = false;
        setFollowTerminal(false);
      }
    }
  };
  const handleTableScroll = () => {
    const node = tableContainerRef.current;
    if (!node) return;
    // Save current table scroll position
    tableScrollPositionRef.current = node.scrollTop;
  };
  const switchToTableMode = () => {
    if (terminalTextRef.current) {
      terminalScrollPositionRef.current = terminalTextRef.current.scrollTop;
    }
    if (firstTableSwitchAfterSolveRef.current) {
      terminalPageRef.current = 0;
      terminalScrollPositionRef.current = 0;
      tablePageRef.current = 0;
      tableScrollPositionRef.current = 0;
      firstTableSwitchAfterSolveRef.current = false;
    } else {
      terminalPageRef.current = clampedPage;
    }
    followTerminalRef.current = false;
    setFollowTerminal(false);
    isSwitchingViewRef.current = true;
    restoreScrollRef.current = tableScrollPositionRef.current;
    if (!filterActive && useLessRamRef.current && !isViewInRam(tablePageRef.current, totalCountRefs())) setIsRestoring(true);
    setPage(tablePageRef.current);
    setTableView(true);
  };
  const finishTableBusySoon = () => {
    requestAnimationFrame(() => requestAnimationFrame(() => setTableBusyMessage("")));
  };
  const switchToTerminalMode = () => {
    if (tableContainerRef.current) {
      tableScrollPositionRef.current = tableContainerRef.current.scrollTop;
    }
    tablePageRef.current = clampedPage;
    followTerminalRef.current = false;
    setFollowTerminal(false);
    isSwitchingViewRef.current = true;
    restoreScrollRef.current = terminalScrollPositionRef.current;
    if (!filterActive && useLessRamRef.current && !isViewInRam(terminalPageRef.current, totalCountRefs())) setIsRestoring(true);
    setPage(terminalPageRef.current);
    setTableView(false);
  };
  useEffect(() => {
    if (followTerminal && !isSwitchingViewRef.current && !isRestoring) {
      requestAnimationFrame(() => {
        if (followTerminalRef.current) {
          const node = terminalTextRef.current;
          if (node) node.scrollTop = node.scrollHeight;
        }
      });
    }
  }, [outputLines, statusLines, solutions, tableView, running, followTerminal, isRestoring]);
  useEffect(() => {
    isSwitchingViewRef.current = false;
  }, [tableView]);
  // Intentional feature by Abid: keep the page-switcher pill opaque as long as
  // autoscroll is running; once autoscroll stops (scroll-lock or solve end) it
  // returns to transparent after the cooldown.
  useEffect(() => {
    if (followTerminal) {
      if (pageSwitcherHideTimerRef.current !== undefined) {
        window.clearTimeout(pageSwitcherHideTimerRef.current);
        pageSwitcherHideTimerRef.current = undefined;
      }
      setPageSwitcherOpaque(true);
    } else if (pageSwitcherHideTimerRef.current === undefined) {
      pageSwitcherHideTimerRef.current = window.setTimeout(() => {
        pageSwitcherHideTimerRef.current = undefined;
        setPageSwitcherOpaque(false);
      }, PAGE_SWITCHER_COOLDOWN_MS);
    }
  }, [followTerminal]);
  useEffect(() => {
    pageRef.current = page;
  }, [page]);
  useEffect(() => {
    pageSizeRef.current = pageSize;
  }, [pageSize]);
  useEffect(() => {
    showAllRef.current = showAll;
  }, [showAll]);
  useEffect(() => {
    if (showAll || !followTerminalRef.current) return;
    setPage(Math.max(0, totalPages - 1));
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [totalCount, pageSize, showAll]);
  useEffect(() => {
    setPage(0);
  }, [pageSize, showAll]);
  useEffect(() => {
    if (!settingsReady.current) return;
    if (!filterActive && useLessRam && !isViewInRam(page, totalCountRefs())) setIsRestoring(true);
    void syncOffload();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [solutions, page, pageSize, useLessRam, showAll, running, filterActive]);
  useEffect(() => {
    const onMouseDown = (event: MouseEvent) => {
      const pill = pageSwitcherRef.current;
      const input = pageInputRef.current;
      if (pill && input && document.activeElement === input && !pill.contains(event.target as Node)) {
        input.blur();
      }
    };
    document.addEventListener("mousedown", onMouseDown);
    return () => document.removeEventListener("mousedown", onMouseDown);
  }, []);
  useLayoutEffect(() => {
    if (followTerminalRef.current || isRestoring) return;
    const node = tableView ? tableContainerRef.current : terminalTextRef.current;
    if (!node) return;
    if (restoreScrollRef.current !== null) {
      node.scrollTop = restoreScrollRef.current;
      restoreScrollRef.current = null;
    } else {
      node.scrollTop = pageScrollEdgeRef.current === "bottom" ? node.scrollHeight : 0;
    }
  }, [page, tableView, isRestoring]);
  useEffect(() => {
    if (!tableBusyMessage) return;
    const id = window.setInterval(() => setTableBusyTick((value) => value + 1), 900);
    return () => window.clearInterval(id);
  }, [tableBusyMessage]);
  useEffect(() => {
    // Gather live rate samples in the background (regardless of whether the debug
    // modal is open) so the graph is always fully populated, including after the
    // solve has finished. History is reset when the next solve starts.
    const id = setInterval(() => {
      const start = solveStartTimeRef.current;
      if (!start) return;
      const now = performance.now();
      const running = runningRef.current;
      const refTime = running ? now : (solveStopTimeRef.current || now);
      const elapsedSec = (refTime - start) / 1000;
      if (elapsedSec <= 0) return;

      const history = rateHistoryRef.current;
      const last = history[history.length - 1];

      if (running) {
        const timestamps = debugStatsRef.current.solutionTimestamps;
        const keepFrom = now - 15000;
        let trim = 0;
        while (trim < timestamps.length && timestamps[trim] <= keepFrom) trim++;
        if (trim) timestamps.splice(0, trim);

        const windowMs = Math.max(1000, Math.min(10000, elapsedSec * 1000));
        const cutoff = refTime - windowMs;
        let count = 0;
        for (let i = timestamps.length - 1; i >= 0; i--) {
          if (timestamps[i] > cutoff) count++;
          else break;
        }
        const solRate = count / (windowMs / 60000);
        const nodeRate = progressRateRef.current;
        if (!last || elapsedSec - last.t >= 0.4) history.push({ t: elapsedSec, sol: solRate, node: nodeRate });
      } else if (!last || elapsedSec - last.t >= 0.05) {
        // Solve finished: push one final point at the stop time, then freeze.
        const windowMs = Math.max(1000, Math.min(10000, elapsedSec * 1000));
        const cutoff = refTime - windowMs;
        const timestamps = debugStatsRef.current.solutionTimestamps;
        let count = 0;
        for (let i = timestamps.length - 1; i >= 0; i--) {
          if (timestamps[i] > cutoff) count++;
          else break;
        }
        const solRate = count / (windowMs / 60000);
        const nodeRate = elapsedSec > 0 ? progressNodesRef.current / elapsedSec : 0;
        history.push({ t: elapsedSec, sol: solRate, node: nodeRate });
      }
      if (history.length > 30000) {
        const compacted: { t: number; sol: number; node: number }[] = [];
        for (let i = 0; i < history.length; i += 2) compacted.push(history[i]);
        rateHistoryRef.current = compacted;
      }
    }, 250);
    return () => clearInterval(id);
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);
  useEffect(() => {
    // Re-render while the debug modal is open so the grid stats keep updating.
    if (modal !== "debug") return;
    const id = setInterval(() => setDebugTick(t => t + 1), 250);
    return () => clearInterval(id);
  }, [modal]);

  const armOutputToolFade = () => {
    if (outputIdleTimer.current !== undefined) window.clearTimeout(outputIdleTimer.current);
    outputIdleTimer.current = window.setTimeout(() => setOutputToolsFaded(true), 1500);
  };
  const markOutputToolsActive = () => {
    setOutputToolsFaded(false);
    if (solutionsRef.current.length) armOutputToolFade();
  };
  useEffect(() => {
    if (!solutions.length) {
      setOutputToolsFaded(false);
      return;
    }
    setOutputToolsFaded(false);
    armOutputToolFade();
    return () => {
      if (outputIdleTimer.current !== undefined) window.clearTimeout(outputIdleTimer.current);
    };
  }, [solutions.length]);
  const toggleIgnoreMiddle = (checked: boolean) => {
    const current = stateRef.current;
    if (checked && current.middle !== 0) preIgnoreMiddle.current = current.middle;
    const next = { ...current, middle: checked ? 0 : preIgnoreMiddle.current };
    cubeActions.current?.set(next);
    setIgnoreMiddle(checked);
  };
  const currentRunKey = () => {
    const flags = solverFlags({ metric, all, suboptimal, depths, generator, two, cubeshape: cubeShape, ignoreEquator: ignoreMiddle, angle, maxX, maxXValue, maxY, maxYValue, maxTotal, maxTotalValue });
    if (ignoreTransforms) flags.push("-x");
    if (smartKarn) flags.push("-k2");
    if (outputMode === "karn") flags.push("-k1");
    if (outputMode === "abid") flags.push("-k3");
    return [lastSolvePosition.current || positionString(cubeState), ...flags].join(" ");
  };
  const applyRunConfig = (key: string) => {
    const [position, ...flags] = key.split(/\s+/).filter(Boolean), parsed = parsePosition(position || "");
    if (parsed) restore(position);
    setMetric(flags.includes("-es") ? "Slice" : flags.includes("-ea") ? "Angle" : "Move");
    const allFlag = flags.find((flag) => /^-a\d*$/.test(flag));
    setAll(Boolean(allFlag)); setSuboptimal(allFlag && allFlag.length > 2 ? Number(allFlag.slice(2)) : 0);
    const depthFlag = flags.find((flag) => flag.startsWith("-d")); setDepths(depthFlag?.slice(2) || "");
    setGenerator(flags.includes("-g"));
    setTwo(flags.includes("-2") ? "2 Gen" : flags.includes("-p") ? "Pseudo 2 Gen" : "None");
    setCubeShapeMemory(flags.includes("-c"));
    if (flags.includes("-m") !== ignoreMiddle) { ignoreHistory.current = true; toggleIgnoreMiddle(flags.includes("-m")); ignoreHistory.current = false; }
    setAngle(flags.includes("-nb") ? "Both" : flags.includes("-nu") ? "Top" : flags.includes("-nd") ? "Bottom" : "None");
    const setLimit = (prefix: string, setEnabled: (v: boolean) => void, setValue: (v: number) => void) => {
      const flag = flags.find((value) => value.startsWith(prefix)); setEnabled(Boolean(flag));
      if (flag) setValue(Number(flag.slice(2)));
    };
    setLimit("-X", setMaxX, setMaxXValue); setLimit("-Y", setMaxY, setMaxYValue); setLimit("-Z", setMaxTotal, setMaxTotalValue);
    setIgnoreTransforms(flags.includes("-x"));
    if (flags.includes("-k1")) setOutputMode("karn");
    else if (flags.includes("-k2")) setOutputMode("cskarn");
    else if (flags.includes("-k3")) setOutputMode("abid");
    solutionsRef.current = [];
    outputLinesRef.current = [];
    seenRaw.current.clear();
    seenDisplay.current.clear();
    setSolutions([]);
    setOutputLines([]);
    setStatusLines([]);
    setRunCubeShape(false);
    setTableView(false);
    history.back();
  };
  const restore = (position: string) => {
    const next = parsePosition(position);
    if (!next) return;
    ignoreHistory.current = true;
    cubeActions.current?.set(next);
    ignoreHistory.current = false;
  };
  const doUndo = () => {
    if (running) return;
    setUndo((stack) => {
      if (!stack.length) return stack;
      const previous = stack[stack.length - 1];
      const snapshot = positionString(stateRef.current);
      restore(previous);
      setRedo((items) => [...items, snapshot].slice(-64));
      return stack.slice(0, -1);
    });
  };
  const doRedo = () => {
    if (running) return;
    setRedo((stack) => {
      if (!stack.length) return stack;
      const next = stack[stack.length - 1];
      const snapshot = positionString(stateRef.current);
      restore(next);
      setUndo((items) => [...items, snapshot].slice(-64));
      return stack.slice(0, -1);
    });
  };
  const chooseMode = (next: string) => {
    setMode(next);
    setInput("");
    setModeMenu(false);
  };
  const cycleMode = () =>
    chooseMode(
      mode === "SCRAMBLE" ? "ALG" : mode === "ALG" ? "POSITION" : "SCRAMBLE",
    );
  const apply = async (fromSolved = false) => {
    let next: CubeState | undefined;
    if (mode === "POSITION") next = parsePosition(input);
    else {
      let raw = input.trim() || "0,0";
      if (mode === "ALG") raw = invertScramble(raw);
      const leadingSlash = /^[\\/]/.test(raw), trailingSlash = raw.length > 1 && /[\\/]$/.test(raw);
      const commaInput = addCommas(raw);
      try {
        raw = tauri()?.core?.invoke ? await tauri()!.core!.invoke<string>("unkarnify", { input: commaInput }) : commaInput;
      } catch { raw = commaInput; }
      if (leadingSlash && !raw.startsWith("/")) raw = "/" + raw;
      if (trailingSlash && !raw.endsWith("/")) raw += "/";
      const base = fromSolved
        ? { position: [...solved], partial: Array(24).fill(0), middle: 1, middlePartial: 0 }
        : cubeState;
      next = applyNumericAlgorithm(base, raw);
    }
    if (!next) {
      setInputError(mode === "POSITION" ? t('errors.invalidPosition') : t('errors.invalidAlg'));
      return;
    }
    cubeActions.current?.set(next);
    setCubeState(next);
    setInputError("");
  };
  const buildDisplayPair = async (line: string, startPosition: string, sliceStart?: string) => {
    const lb = line.lastIndexOf("["), rb = line.lastIndexOf("]");
    if (lb < 0 || rb < 0) return { rawDisplay: line, karnDisplay: line };
    const rawAlg = line.slice(0, lb).trim();
    let karnDisplay = line;
    // Only round-trip through the karnify IPC when karn output is actually
    // shown — in "abid"/"normal" modes the streamed lines already carry every
    // display we need, and any extra invoke per line is pure overhead.
    if (karn) {
      try {
        const converted = await tauri()?.core?.invoke<string>("karnify", {
          input: rawAlg,
          position: smartKarn && !lastSolveCubeShape.current ? startPosition : null,
          generator,
        });
        if (converted) karnDisplay = `${converted}  ${line.slice(lb).trim()}`;
      } catch { /* retain numeric output */ }
    }
    return {
      rawDisplay: injectSliceIndicator(line, sliceStart),
      karnDisplay: injectSliceIndicator(karnDisplay, sliceStart),
    };
  };
  /*
   * LIVE STREAMING — INTENTIONAL FEATURE (by Abid)
   *
   * Solutions appear one-by-one in the terminal as the solver emits them, similar
   * to how AI UIs stream tokens. This is THE magic of the app. It MUST NOT be
   * traded away for raw speed — no batching, no hiding behind a spinner, no
   * waiting for "all results" before showing anything. Every solution must be
   * flushed to the terminal as soon as it is available.
   */
  const addOutputLine = (line: OutputLine) => {
    outputLinesRef.current = [...outputLinesRef.current, line].slice(-1000);
    scheduleSolutionFlush();
  };
  const setRunningState = (value: boolean) => {
    runningRef.current = value;
    setRunning(value);
  };
  const flushSolutionState = () => {
    renderFrame.current = undefined;
    setOutputLines(outputLinesRef.current);
    setSolutions(solutionsRef.current);
  };
  const scheduleSolutionFlush = () => {
    if (renderFrame.current !== undefined) return;
    renderFrame.current = requestAnimationFrame(flushSolutionState);
  };
  const replaceOutputLines = (lines: OutputLine[]) => {
    outputLinesRef.current = lines.slice(-1000);
    scheduleSolutionFlush();
  };
  const addSolution = (row: Solution) => {
    const ramEnd = ramOffsetRef.current + solutionsRef.current.length;
    const pendingLen = pendingTailRef.current?.length ?? 0;
    const total = solutionsRef.current.length + offloadedTotalRef.current + pendingLen;
    if (useLessRamRef.current && ramEnd < total) {
      const tailStart = total - pendingLen;
      if (pendingLen && ramEnd === tailStart) {
        solutionsRef.current = [...solutionsRef.current, ...pendingTailRef.current!];
        pendingTailRef.current = null;
        setPendingTailCount(0);
      } else {
        pendingTailRef.current = [...(pendingTailRef.current ?? []), row];
        setPendingTailCount(pendingTailRef.current.length);
        scheduleSolutionFlush();
        debugStatsRef.current.solutionTimestamps.push(performance.now());
        return;
      }
    }
    solutionsRef.current = [...solutionsRef.current, row];
    scheduleSolutionFlush();
    debugStatsRef.current.solutionTimestamps.push(performance.now());
  };
  const setSolutionRows = (rows: Solution[]) => {
    solutionsRef.current = rows;
    scheduleSolutionFlush();
  };
  const totalCountRefs = () => offloadedTotalRef.current + solutionsRef.current.length + (pendingTailRef.current?.length ?? 0);
  const offloadRange = async (start: number, rows: Solution[]) => {
    if (!rows.length) return;
    for (let i = 0; i < rows.length; i += OFFLOAD_CHUNK) {
      const chunk = rows.slice(i, i + OFFLOAD_CHUNK);
      const chunkStart = start + i;
      offloadedChunksRef.current.set(chunkStart, chunk.length);
      await writeOffloadedChunk(chunkStart, chunk);
    }
    offloadedTotalRef.current += rows.length;
  };
  const restorePage = async (viewStart: number, viewEnd: number, tailStart: number, total: number) => {
    const loaded: { index: number; row: Solution }[] = [];
    const diskEnd = Math.min(viewEnd, tailStart);
    if (viewStart < diskEnd) {
      const chunks = [...offloadedChunksRef.current.entries()].sort((a, b) => a[0] - b[0]);
      for (const [chunkStart, count] of chunks) {
        const chunkEnd = chunkStart + count;
        if (chunkEnd <= viewStart || chunkStart >= diskEnd) continue;
        const raw = await readOffloadedChunk(chunkStart);
        if (!raw || raw.length !== count) {
          offloadedChunksRef.current.delete(chunkStart);
          offloadedTotalRef.current -= count;
          continue;
        }
        const from = Math.max(0, viewStart - chunkStart);
        const to = Math.min(count, diskEnd - chunkStart);
        for (let i = from; i < to; i++) loaded.push({ index: chunkStart + i, row: raw[i] });
        offloadedChunksRef.current.delete(chunkStart);
        offloadedTotalRef.current -= count;
        await removeOffloadedChunk(chunkStart);
        const before = raw.slice(0, from);
        if (before.length) { offloadedChunksRef.current.set(chunkStart, before.length); offloadedTotalRef.current += before.length; await writeOffloadedChunk(chunkStart, before); }
        const after = raw.slice(to);
        if (after.length) { const aStart = chunkStart + to; offloadedChunksRef.current.set(aStart, after.length); offloadedTotalRef.current += after.length; await writeOffloadedChunk(aStart, after); }
      }
      loaded.sort((a, b) => a.index - b.index);
    }
    let result = loaded.map((x) => x.row);
    if (viewEnd > tailStart) {
      const from = Math.max(0, viewStart - tailStart);
      if (from > 0) await offloadRange(tailStart, (pendingTailRef.current ?? []).slice(0, from));
      const pending = pendingTailRef.current ?? [];
      const to = Math.min(pending.length, viewEnd - tailStart);
      result = [...result, ...pending.slice(from, to)];
      if (to >= pending.length) pendingTailRef.current = null;
      else pendingTailRef.current = pending.slice(to);
      setPendingTailCount(pendingTailRef.current?.length ?? 0);
    }
    return result;
  };
  const flushPendingTailToDisk = async () => {
    const pending = pendingTailRef.current;
    if (!pending?.length) { pendingTailRef.current = null; setPendingTailCount(0); return; }
    const total = totalCountRefs();
    await offloadRange(total - pending.length, pending);
    pendingTailRef.current = null;
    setPendingTailCount(0);
  };
  const restoreAll = async () => {
    const chunks = [...offloadedChunksRef.current.entries()].sort((a, b) => a[0] - b[0]);
    const ramOffset = ramOffsetRef.current;
    const full: Solution[] = [];
    let inserted = false;
    for (const [start, count] of chunks) {
      if (!inserted && start >= ramOffset) { full.push(...solutionsRef.current); inserted = true; }
      const raw = await readOffloadedChunk(start);
      if (raw && raw.length === count) full.push(...raw);
      await removeOffloadedChunk(start);
    }
    if (!inserted) full.push(...solutionsRef.current);
    const pending = pendingTailRef.current ?? [];
    pendingTailRef.current = null;
    setPendingTailCount(0);
    full.push(...pending);
    offloadedChunksRef.current.clear();
    offloadedTotalRef.current = 0;
    ramOffsetRef.current = 0;
    solutionsRef.current = full;
    setSolutions([...full]);
    setOffloadedTotal(0);
  };
  const isViewInRam = (view: number, total: number) => {
    const pc = computeTotalPages(total, pageSizeRef.current);
    const v = Math.min(Math.max(view, 0), pc - 1);
    const vs = v * pageSizeRef.current;
    const ve = v === pc - 1 ? total : Math.min(vs + pageSizeRef.current, total);
    const rs = ramOffsetRef.current;
    const re = rs + solutionsRef.current.length;
    return vs >= rs && ve <= re;
  };
  const clearSolutions = () => {
    solutionsRef.current = [];
    pendingTailRef.current = null;
    setPendingTailCount(0);
    offloadedChunksRef.current.clear();
    offloadedTotalRef.current = 0;
    ramOffsetRef.current = 0;
    setOffloadedTotal(0);
    setSolutions([]);
    void clearOffloadedSolutions();
  };
  const syncOffload = async () => {
    if (!useLessRamRef.current) {
      if (offloadedTotalRef.current || (pendingTailRef.current?.length ?? 0)) {
        if (offloadBusyRef.current) { offloadPendingRef.current = true; return; }
        offloadBusyRef.current = true;
        try { await restoreAll(); } finally {
          offloadBusyRef.current = false;
          if (offloadPendingRef.current) { offloadPendingRef.current = false; void syncOffload(); }
        }
      }
      return;
    }
    if (offloadBusyRef.current) { offloadPendingRef.current = true; return; }
    offloadBusyRef.current = true;
    try {
      let total = totalCountRefs();
      if (!total) { setIsRestoring(false); return; }
      if (!runningRef.current && (pendingTailRef.current?.length ?? 0)) {
        await flushPendingTailToDisk();
        total = totalCountRefs();
      }
      const pageCount = computeTotalPages(total, pageSizeRef.current);
      const view = Math.min(Math.max(pageRef.current, 0), pageCount - 1);
      const viewStart = view * pageSizeRef.current;
      const viewEnd = view === pageCount - 1 ? total : Math.min(viewStart + pageSizeRef.current, total);
      const ramOffset = ramOffsetRef.current;
      const ramEnd = ramOffset + solutionsRef.current.length;
      if (showAllRef.current) {
        if (solutionsRef.current.length === total) { setIsRestoring(false); return; }
        setIsRestoring(true);
        await offloadRange(ramOffset, solutionsRef.current);
        solutionsRef.current = [];
        ramOffsetRef.current = 0;
        await restoreAll();
        setIsRestoring(false);
        return;
      }
      if (viewStart >= ramOffset && viewEnd <= ramEnd) {
        if (runningRef.current) {
          if (viewStart > ramOffset) {
            await offloadRange(ramOffset, solutionsRef.current.slice(0, viewStart - ramOffset));
            const cur = solutionsRef.current;
            solutionsRef.current = cur.slice(viewStart - ramOffset);
            ramOffsetRef.current = viewStart;
            setSolutions([...solutionsRef.current]);
            setOffloadedTotal(offloadedTotalRef.current);
          }
          setIsRestoring(false);
          return;
        }
        if (viewStart > ramOffset) {
          await offloadRange(ramOffset, solutionsRef.current.slice(0, viewStart - ramOffset));
        }
        const cur = solutionsRef.current;
        const keep = viewEnd - viewStart;
        if (cur.length > viewEnd - ramOffset) {
          await offloadRange(viewEnd, cur.slice(viewEnd - ramOffset));
        }
        const cur2 = solutionsRef.current;
        solutionsRef.current = cur2.slice(viewStart - ramOffset, viewStart - ramOffset + keep);
        ramOffsetRef.current = viewStart;
        setSolutions([...solutionsRef.current]);
        setOffloadedTotal(offloadedTotalRef.current);
        setIsRestoring(false);
        return;
      }
      setIsRestoring(true);
      await offloadRange(ramOffset, solutionsRef.current);
      solutionsRef.current = [];
      ramOffsetRef.current = 0;
      const freshTotal = totalCountRefs();
      const freshPageCount = computeTotalPages(freshTotal, pageSizeRef.current);
      const freshView = Math.min(Math.max(pageRef.current, 0), freshPageCount - 1);
      const freshStart = freshView * pageSizeRef.current;
      const freshEnd = freshView === freshPageCount - 1 ? freshTotal : Math.min(freshStart + pageSizeRef.current, freshTotal);
      const pendingLen = pendingTailRef.current?.length ?? 0;
      const loaded = await restorePage(freshStart, freshEnd, freshTotal - pendingLen, freshTotal);
      solutionsRef.current = loaded;
      ramOffsetRef.current = freshStart;
      setSolutions([...loaded]);
      setOffloadedTotal(offloadedTotalRef.current);
      setIsRestoring(false);
    } finally {
      offloadBusyRef.current = false;
      if (offloadPendingRef.current) { offloadPendingRef.current = false; void syncOffload(); }
    }
  };
  // Renders a solution line in the current outputMode. Shared by the live
  // stream (dedup key), the solution rows, the terminal and the table so the
  // same algorithm always renders identically everywhere.
  const buildDisplayText = (rawDisplay: string, karnDisplay: string, abidDisplay: string | undefined, sliceStart: string | undefined) => {
    const base = normalizeLine(karn ? karnDisplay : rawDisplay, normalize);
    if (outputMode === "abid" && abidDisplay) return normalizeLine(abidDisplay, normalize);
    return notationStyle(base, outputMode, !!sliceStart);
  };
  const receiveSolverLine = async (line: string, startPosition: string, runId: number) => {
    if (stopped.current) return;
    if (runId !== solveRunId.current) return;
    if (line.startsWith("__PROGRESS__")) {
      if (!solveStartTimeRef.current) solveStartTimeRef.current = performance.now();
      const nm = line.match(/nodes=(\d+)/);
      if (nm) {
        const nodes = parseInt(nm[1], 10);
        const now = performance.now();
        if (lastProgressNodesRef.current > 0) {
          const dt = (now - lastProgressAtRef.current) / 1000;
          if (dt > 0) progressRateRef.current = (nodes - lastProgressNodesRef.current) / dt;
        }
        lastProgressNodesRef.current = nodes;
        lastProgressAtRef.current = now;
        progressNodesRef.current = nodes;
      }
      return;
    }
    const lb = line.lastIndexOf("["), rb = line.lastIndexOf("]");
    if (lb < 0 || rb < 0) {
      if (debugOutput || !seenRaw.current.size) addOutputLine({ raw: line, karn: line, isSolution: false });
      return;
    }
    const rawAlg = line.slice(0, lb).trim();
    if (seenRaw.current.has(rawAlg)) return;
    seenRaw.current.add(rawAlg);
    const metricsPart = line.slice(lb, rb + 1);
    const afterMetrics = line.slice(rb + 1).trim();
    let rating: RatingResult | undefined, sliceStart: string | undefined;
    let rawDisplay: string, karnDisplay: string, abidDisplay: string | undefined;
    if (afterMetrics) {
      // Extended line layout: <karn>  R{...rating...}  <abid>. Every field is
      // separated by two spaces, and the karn/abid texts themselves only ever
      // contain single spaces, so a plain split on two spaces recovers the
      // fields. The abid field is emitted only when the solve ran with -k3;
      // the rating block only appears for cubeshape solves. Either may be
      // absent, but the karn field is always present in extended output.
      const fields = afterMetrics.split("  ");
      const karnified = fields[0].trim();
      let abidified: string | undefined;
      if (fields[1] && fields[1].startsWith("R{")) {
        try {
          const raw = JSON.parse(fields[1].slice(1));
          rating = { finalScore: raw.f, sliceStart: raw.ss, phase1: raw.p1, phase2: raw.p2, phase3: raw.p3, phase4: raw.p4, ergoUp: raw.eu, ergoDown: raw.ed, sliceCount: raw.sc, movement: raw.mv, bonus: raw.bn, valid: true };
          if (rating.valid) sliceStart = ratingSliceStart(rating);
        } catch { /* unrated */ }
        abidified = fields.slice(2).join("  ") || undefined;
      } else {
        abidified = fields.slice(1).join("  ") || undefined;
      }
      rawDisplay = injectSliceIndicator(rawAlg + "  " + metricsPart, sliceStart);
      karnDisplay = injectSliceIndicator(karnified + "  " + metricsPart, sliceStart);
      if (abidified) abidDisplay = abidified + "  " + metricsPart;
    } else {
      // Legacy format (no extended data): use IPC fallback
      if (lastSolveCubeShape.current && tauri()?.core?.invoke) {
        try {
          rating = await tauri()!.core!.invoke<RatingResult>("rate_algorithm", { algorithm: rawAlg, initialTopA: /^[1-8XYZ]/i.test(startPosition) });
          if (runId !== solveRunId.current) return;
          if (rating.valid) sliceStart = ratingSliceStart(rating);
        } catch {}
      }
      const pair = await buildDisplayPair(line, startPosition, sliceStart);
      if (runId !== solveRunId.current) return;
      rawDisplay = pair.rawDisplay;
      karnDisplay = pair.karnDisplay;
    }
    const displayAlg = lineAlg(buildDisplayText(rawDisplay, karnDisplay, abidDisplay, sliceStart));
    if (seenDisplay.current.has(displayAlg)) return;
    seenDisplay.current.add(displayAlg);
    if (seenRaw.current.size === 1) {
      firstSolutionAt.current = performance.now();
      if (!solveStartTimeRef.current) solveStartTimeRef.current = firstSolutionAt.current;
      if (!debugOutput) replaceOutputLines(outputLinesRef.current.filter((entry) => entry.isSolution));
    }
    const counts = parseSolutionCounts(line);
    const cleanLine = rawAlg + "  " + metricsPart;
    const row: Solution = { raw: cleanLine, rawDisplay, karnDisplay, abidDisplay, algRaw: rawAlg, ...counts, ergoRaw: rating?.valid ? ratingScore(rating) : undefined, sliceStart };
    addSolution(row);
    addOutputLine({ raw: rawDisplay, karn: karnDisplay, isSolution: true, algRaw: rawAlg, sliceStart });
  };
  const solve = async () => {
    const native = tauri();
    if (!native?.core?.invoke) {
      const nativeMsg = t('status.nativeUnavailable');
      const fallback = { raw: nativeMsg, karn: nativeMsg, isSolution: false };
      outputLinesRef.current = [fallback];
      setOutputLines([fallback]);
      openMobileOutput();
      return;
    }
    if (runningRef.current) {
      stopped.current = true;
      const stopMsg = t('status.stopRequested');
      addOutputLine({ raw: stopMsg, karn: stopMsg, isSolution: false });
      setStatusLines((lines) => [...lines, t('status.stopReady')].slice(-8));
      setRunningState(false);
      void native.core.invoke("stop_solver").catch(() => undefined);
      return;
    }
    if ((two === "2 Gen" && (twoGenStatus.compatibility < 2 || (cubeShape && !twoGenStatus.cornersTwo))) ||
        (two === "Pseudo 2 Gen" && (twoGenStatus.compatibility < 1 || (cubeShape && !twoGenStatus.cornersPseudo)))) return;
    openMobileOutput();
    const flags = solverFlags({ metric, all, suboptimal, depths, generator, two, cubeshape: cubeShape, ignoreEquator: ignoreMiddle, angle, maxX, maxXValue, maxY, maxYValue, maxTotal, maxTotalValue });
    if (ignoreTransforms) flags.push("-x");
    if (smartKarn) flags.push("-k2");
    if (outputMode === "karn") flags.push("-k1");
    if (outputMode === "abid") flags.push("-k3");
    if (debugOutput) flags.push("-v5");
    stopped.current = false;
    solutionsRef.current = [];
    outputLinesRef.current = [];
    ramOffsetRef.current = 0;
    offloadedTotalRef.current = 0;
    offloadedChunksRef.current.clear();
    pendingTailRef.current = null;
    setOffloadedTotal(0);
    setPendingTailCount(0);
    setIsRestoring(false);
    filterSearchIdRef.current++;
    setFilterOpen(false);
    setFilterQuery("");
    setFilterResults(null);
    setFilterAppliedQuery("");
    setFilterSearching(false);
    setFilterInvalid(false);
    if (useLessRamRef.current) void clearOffloadedSolutions();
    seenRaw.current.clear();
    seenDisplay.current.clear();
    lineQueue.current = Promise.resolve();
    firstSolutionAt.current = 0;
    solveStartTimeRef.current = 0;
    solveStopTimeRef.current = 0;
    debugStatsRef.current = { solutionTimestamps: [] };
    rateHistoryRef.current = [];
    progressNodesRef.current = 0;
    progressRateRef.current = 0;
    lastProgressNodesRef.current = 0;
    lastProgressAtRef.current = 0;
    followTerminalRef.current = true;
    lastSolveCubeShape.current = cubeShape;
    firstTableSwitchAfterSolveRef.current = true;
    tableMetricRef.current = metric;
    terminalScrollPositionRef.current = 0;
    tableScrollPositionRef.current = 0;
    terminalPageRef.current = 0;
    tablePageRef.current = 0;
    restoreScrollRef.current = null;
    setRunCubeShape(cubeShape);
    const solvingMsg = t('status.solving');
    setOutputLines([{ raw: solvingMsg, karn: solvingMsg, isSolution: false }]);
    outputLinesRef.current = [{ raw: solvingMsg, karn: solvingMsg, isSolution: false }];
    setStatusLines([]);
    setSolutions([]);
    setPage(0);
    setFollowTerminal(true);
    setTableView(false);
    setTableBusyMessage("");
    setCompletedWhilePaused(false);
    setRunningState(true);
    const runId = ++solveRunId.current;
    const start = positionString(cubeState), startedAt = performance.now();
    lastSolvePosition.current = start;
    try {
      if (!native.Channel) throw new Error(t('status.channelUnavailable'));
      const onLine = new native.Channel<string>();
      onLine.onmessage = async (line) => {
        if (runId !== solveRunId.current) return;
        lineQueue.current = lineQueue.current.then(() => receiveSolverLine(line, start, runId));
      };
      const result = await native.core.invoke<{ code: number | null; stdout: string; stderr: string }>("solve", { position: start, flags, onLine });
      if (runId !== solveRunId.current) return;
      const shouldAutoTable = followTerminalRef.current;
      // Intentional feature by Abid: only auto-switch to table view when there are
      // 2+ solutions, since table view is only useful for comparing/organizing output.
      if (shouldAutoTable && totalCountRefs() >= 2) {
        switchToTableMode();
        setTableBusyMessage(t('table.busyResolving'));
      }
      await lineQueue.current;
      if (shouldAutoTable) setTableBusyMessage(t('table.busyRating'));
      for (const line of `${result.stderr || ""}`.split(/\r?\n/).filter(Boolean))
        await receiveSolverLine(line, start, runId);
      if (runId !== solveRunId.current) return;
      if (lastSolveCubeShape.current && !useLessRamRef.current) {
        if (shouldAutoTable && totalCountRefs()) setTableBusyMessage(t('table.busyNormalizing'));
        setSolutionRows(medianNormalize(solutionsRef.current));
      }
      if (shouldAutoTable) setTableBusyMessage(t('table.busyBuilding'));
      flushSolutionState();
      const count = totalCountRefs();
      const status = `${stopped.current ? t('status.stopped') : result.code === 0 ? t('status.done') : t('status.error')} — ${count} solution${count === 1 ? "" : "s"} found in ${((performance.now() - startedAt) / 1000).toFixed(2)}s.`;
      setStatusLines(!useLessRamRef.current && lastSolveCubeShape.current && count ? [t('status.ranked').replace('{{count}}', String(count))] : [status]);
      // Intentional feature by Abid: only auto-switch to table view when 2+ solutions
      // exist, since table view is only useful for comparing/organizing output.
      if (shouldAutoTable && count >= 2) {
        switchToTableMode();
        finishTableBusySoon();
      } else if (count) {
        setCompletedWhilePaused(true);
        setTableBusyMessage("");
      } else {
        setTableBusyMessage("");
      }
    } catch (error) {
      setTableBusyMessage("");
      setStatusLines((lines) => [...lines, t('status.errorPrefix') + String(error)].slice(-8));
    } finally {
      if (runId === solveRunId.current) { solveStopTimeRef.current = performance.now(); setRunningState(false); }
    }
  };
  useEffect(() => {
    const onKeyDown = (event: KeyboardEvent) => {
      if (event.ctrlKey && event.key === "Enter") {
        event.preventDefault();
        void solve();
      }
      if (event.ctrlKey && (event.key === "=" || event.key === "+")) {
        event.preventDefault();
        setZoom((value) => {
          const next = Math.min(2, Math.round((value + 0.1) * 10) / 10);
          zoomRef.current = next;
          document.documentElement.classList.toggle("tall-viewport", window.innerHeight / next >= 810);
          return next;
        });
      }
      if (event.ctrlKey && event.key === "-") {
        event.preventDefault();
        setZoom((value) => {
          const next = Math.max(0.5, Math.round((value - 0.1) * 10) / 10);
          zoomRef.current = next;
          document.documentElement.classList.toggle("tall-viewport", window.innerHeight / next >= 810);
          return next;
        });
      }
      if (event.ctrlKey && event.key === "0") {
        event.preventDefault();
        zoomRef.current = 1;
        setZoom(1);
        document.documentElement.classList.toggle("tall-viewport", window.innerHeight >= 810);
      }
      if (event.ctrlKey && event.key.toLowerCase() === "z") { event.preventDefault(); doUndo(); }
      if (event.ctrlKey && event.key.toLowerCase() === "y") { event.preventDefault(); doRedo(); }
    };
    window.addEventListener("keydown", onKeyDown);
    return () => window.removeEventListener("keydown", onKeyDown);
  });
  useEffect(() => {
    void loadSettings().then((value) => {
      if (!value) { queueMicrotask(() => { settingsReady.current = true; }); return; }
      if (typeof value.outputMode === "string") {
        const restored = value.outputMode === "normal" ? "default" : value.outputMode;
        if (OUTPUT_MODES.includes(restored as OutputMode)) setOutputMode(restored as OutputMode);
      } else if (typeof value.smartKarn === "boolean" && typeof value.karn === "boolean") setOutputMode(value.smartKarn ? "cskarn" : value.karn ? "karn" : "default");
      else if (typeof value.karn === "boolean") setOutputMode(value.karn ? "karn" : "default");
      if (typeof value.abidNotation === "boolean") setAbidNotation(value.abidNotation);
      if (typeof value.ignoreTransforms === "boolean") setIgnoreTransforms(value.ignoreTransforms);
      if (typeof value.debugOutput === "boolean") setDebugOutput(value.debugOutput);
      if (typeof value.normalize === "string") setNormalize(value.normalize);
      if (typeof value.mode === "string") setMode(value.mode);
      if (typeof value.metric === "string") setMetric(value.metric);
      if (typeof value.two === "string") setTwo(value.two);
      if (typeof value.angle === "string") setAngle(value.angle);
      if (typeof value.all === "boolean") setAll(value.all);
      if (typeof value.suboptimal === "number") setSuboptimal(value.suboptimal);
      if (typeof value.depths === "string") setDepths(value.depths);
      if (typeof value.generator === "boolean") setGenerator(value.generator);
      if (typeof value.cubeShape === "boolean") setCubeShapeMemory(value.cubeShape);
      if (typeof value.ignoreMiddle === "boolean") {
        setIgnoreMiddle(value.ignoreMiddle);
        if (value.ignoreMiddle) queueMicrotask(() => { ignoreHistory.current = true; toggleIgnoreMiddle(true); ignoreHistory.current = false; });
      }
      if (typeof value.maxX === "boolean") setMaxX(value.maxX);
      if (typeof value.maxXValue === "number") setMaxXValue(value.maxXValue);
      if (typeof value.maxY === "boolean") setMaxY(value.maxY);
      if (typeof value.maxYValue === "number") setMaxYValue(value.maxYValue);
      if (typeof value.maxTotal === "boolean") setMaxTotal(value.maxTotal);
      if (typeof value.maxTotalValue === "number") setMaxTotalValue(value.maxTotalValue);
      if (typeof value.zoom === "number") { setZoom(value.zoom); zoomRef.current = value.zoom; document.documentElement.classList.toggle("tall-viewport", window.innerHeight / value.zoom >= 810); }
      if (typeof value.pageSize === "number" && PAGE_SIZE_OPTIONS.includes(value.pageSize)) setPageSize(value.pageSize);
      if (typeof value.showAll === "boolean") setShowAll(value.showAll);
      if (typeof value.useLessRam === "boolean") setUseLessRam(value.useLessRam);
      if (typeof value.deleteTablesOnQuitV2 === "boolean") setDeleteTablesOnQuit(value.deleteTablesOnQuitV2);
      queueMicrotask(() => { settingsReady.current = true; });
    });
  }, []);
  useEffect(() => {
    if (!settingsReady.current) return;
    void saveSettings({
      outputMode, abidNotation, ignoreTransforms, debugOutput, normalize, mode,
      metric, two, angle, all, suboptimal, depths, generator, cubeShape: cubeShapeMemory, ignoreMiddle,
      maxX, maxXValue, maxY, maxYValue, maxTotal, maxTotalValue, zoom, pageSize, showAll, useLessRam,
      deleteTablesOnQuitV2,
    });
  }, [outputMode, abidNotation, ignoreTransforms, debugOutput, normalize, mode, metric, two, angle, all, suboptimal, depths, generator, cubeShapeMemory, ignoreMiddle, maxX, maxXValue, maxY, maxYValue, maxTotal, maxTotalValue, zoom, pageSize, showAll, useLessRam, deleteTablesOnQuit]);
  useEffect(() => {
    if (!solutions.length || running) return;
    let cancelled = false;
    void (async () => {
      for (const solution of solutions) {
        const pair = await buildDisplayPair(solution.raw, lastSolvePosition.current || positionString(cubeState), solution.sliceStart);
        if (cancelled) return;
        const rebuilt = solutionsRef.current.map((row) => row.raw === solution.raw ? { ...row, ...pair } : row);
        setSolutionRows(rebuilt);
        replaceOutputLines(outputLinesRef.current.map((entry) => entry.algRaw === solution.algRaw ? { ...entry, raw: pair.rawDisplay, karn: pair.karnDisplay } : entry));
      }
    })();
    return () => { cancelled = true; };
    // Rebuild only when karnification inputs change, not when solution rows arrive.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [outputMode, generator]);
  useEffect(() => {
    let cancelled = false;
    if (!tauri()?.core?.invoke) return;
    const specificAngleBot = angle === "Both" || angle === "Bottom";
    void tauri()!.core!.invoke<TwoGenStatus>("two_gen_status", { position: rawPosition(cubeState), specificAngleBot })
      .then((status) => { if (!cancelled) setTwoGenStatus(status); })
      .catch((err) => console.error("two_gen_status invoke failed:", err));
    return () => { cancelled = true; };
  }, [cubeState, angle]);
  useEffect(() => {
    void loadFavorites().then((value) => {
      if (value) setFavorites(value);
      queueMicrotask(() => { favoritesReady.current = true; });
    });
  }, []);
  useEffect(() => {
    if (!favoritesReady.current) return;
    void saveFavorites(favorites);
  }, [favorites]);
  useEffect(() => {
    let unlisten: (() => void) | undefined;
    const cleanupOnQuit = () => {
      void clearOffloadedSolutions();
      if (deleteTablesOnQuitRef.current) {
        if (isNativePlatform()) void clearAllTables();
        else void clearTableBlobs();
      }
    };
    const onBeforeUnload = () => { cleanupOnQuit(); };
    window.addEventListener("beforeunload", onBeforeUnload);
    if ((window as Window & { __TAURI__?: unknown }).__TAURI__) {
      void import("@tauri-apps/api/window").then(async ({ getCurrentWindow }) => {
        try {
          unlisten = await getCurrentWindow().onCloseRequested(async () => {
            await clearOffloadedSolutions();
            if (deleteTablesOnQuitRef.current) await clearAllTables();
          });
        } catch { /* capability missing; the beforeunload fallback still runs */ }
      });
    }
    return () => {
      window.removeEventListener("beforeunload", onBeforeUnload);
      unlisten?.();
    };
  }, []);
  const cubeshapeBlockedBy2Gen = (two === "2 Gen" && !twoGenStatus.cornersTwo) ||
    (two === "Pseudo 2 Gen" && !twoGenStatus.cornersPseudo);
  const cubeshapeForced = !isGoodSquares(cubeState) || cubeshapeBlockedBy2Gen;
  const cubeShape = cubeShapeMemory && !cubeshapeForced;
  const twoGenBlocked = (two === "2 Gen" && (twoGenStatus.compatibility < 2 || (cubeShape && !twoGenStatus.cornersTwo))) ||
    (two === "Pseudo 2 Gen" && (twoGenStatus.compatibility < 1 || (cubeShape && !twoGenStatus.cornersPseudo)));
  const cubeshapeDisableReason = cubeshapeBlockedBy2Gen
    ? t('errors.no2GenCorners')
    : !inCubeshape(cubeState) ? (() => {
      const rTop = getLayerR(cubeState.position, 0), rBot = getLayerR(cubeState.position, 12);
      if (rTop < 0 || rBot < 0 || rTop === 1 || rBot === 1) return t('errors.notCubeshape');
      if (cubeState.partial.every((v) => v === 0)) {
        const odd = getParityOdd(cubeState.position);
        if ((rTop === rBot) !== odd) return t('errors.badParity');
      }
      return t('errors.notCubeshape');
    })() : null;
  const specificDepthsActive = depths.trim().length > 0;
  const commandFlags = solverFlags({ metric, all, suboptimal, depths, generator, two, cubeshape: cubeShape, ignoreEquator: ignoreMiddle, angle, maxX, maxXValue, maxY, maxYValue, maxTotal, maxTotalValue });
  if (ignoreTransforms) commandFlags.push("-x");
  if (smartKarn) commandFlags.push("-k2");
  if (outputMode === "karn") commandFlags.push("-k1");
  if (outputMode === "abid") commandFlags.push("-k3");
  const commandPreview = `croissant ${commandFlags.join(" ")} ${positionString(cubeState)}`;
  const showErgo = runCubeShape;
  const solutionDisplayText = (solution: Solution) =>
    buildDisplayText(solution.rawDisplay, solution.karnDisplay, solution.abidDisplay, solution.sliceStart);
  const displaySolution = (solution: Solution): DisplaySolution => {
    const display = solutionDisplayText(solution);
    return { ...solution, display, alg: lineAlg(display) };
  };
  // Abbreviates large counts to 3 sig figs with a k/m/b suffix, trimming trailing zeros
  const formatCount = (n: number): string => {
    const units: [number, string][] = [[1e9, "b"], [1e6, "m"], [1e3, "k"]];
    for (const [value, suffix] of units) {
      if (n >= value) {
        const scaled = n / value;
        const digits = scaled >= 100 ? 0 : scaled >= 10 ? 1 : 2;
        return `${scaled.toFixed(digits).replace(/\.?0+$/, "")}${suffix}`;
      }
    }
    return `${n}`;
  };
  // Scans all solutions (including offloaded/low-RAM chunks) and matches
  // against the displayed alg text (never the abid-barred glyphs) so the filter
  // covers everything, not just the currently loaded page.
  const runFilterSearch = async (query: string, id: number) => {
    let matcher: (alg: string) => boolean;
    if (filterRegexMode) {
      let re: RegExp;
      try {
        re = new RegExp(query, filterMatchCase ? "" : "i");
      } catch {
        if (id !== filterSearchIdRef.current) return;
        setFilterInvalid(true);
        setFilterResults([]);
        setFilterAppliedQuery(query);
        setFilterSearching(false);
        setPage(0);
        return;
      }
      matcher = (alg) => re.test(alg);
    } else {
      const needle = filterMatchCase ? query : query.toLowerCase();
      matcher = (alg) => (filterMatchCase ? alg : alg.toLowerCase()).includes(needle);
    }
    const seen = new Set<string>();
    const matches: Solution[] = [];
    const consider = (solution: Solution) => {
      const alg = displaySolution(solution).alg;
      if (seen.has(alg)) return;
      seen.add(alg);
      if (matcher(alg)) matches.push(solution);
    };
    if (useLessRamRef.current) {
      const chunks = [...offloadedChunksRef.current.entries()].sort((a, b) => a[0] - b[0]);
      for (const [start] of chunks) {
        if (id !== filterSearchIdRef.current) return;
        const raw = await readOffloadedChunk(start);
        if (raw) for (const s of raw) consider(s);
      }
      for (const s of pendingTailRef.current ?? []) consider(s);
    }
    for (const solution of solutionsRef.current) consider(solution);
    if (id !== filterSearchIdRef.current) return;
    setFilterInvalid(false);
    setFilterResults(matches);
    setFilterAppliedQuery(query);
    setFilterSearching(false);
    setPage(0);
  };
  useEffect(() => {
    if (!filterOpen) return;
    if (!filterQuery.trim()) {
      filterSearchIdRef.current++;
      setFilterResults(null);
      setFilterAppliedQuery("");
      setFilterSearching(false);
      setFilterInvalid(false);
      return;
    }
    setFilterSearching(true);
    setFilterInvalid(false);
    const id = ++filterSearchIdRef.current;
    // Delay the actual scan until typing settles, since it can walk the entire
    // (potentially offloaded) result set.
    const timer = window.setTimeout(() => { void runFilterSearch(filterQuery, id); }, 400);
    return () => window.clearTimeout(timer);
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [filterOpen, filterQuery, filterMatchCase, filterRegexMode, outputMode, normalize]);
  // Pagination targets the filtered set (already fully materialized in memory
  // by runFilterSearch) rather than the raw total when a filter is active.
  const activeTotalCount = filterActive ? filterResults!.length : totalCount;
  const totalPages = showAll ? 1 : (() => {
    const raw = Math.max(1, Math.ceil(activeTotalCount / pageSize));
    // Intentional feature by Abid: fold a tiny trailing overflow (<10% of a page)
    // into the previous page instead of creating a nearly-empty last page.
    const remainder = activeTotalCount % pageSize;
    return remainder > 0 && remainder < pageSize * 0.1 ? Math.max(1, raw - 1) : raw;
  })();
  const clampedPage = Math.min(page, totalPages - 1);
  const pageStart = clampedPage * pageSize;
  const pageEnd = showAll ? activeTotalCount : clampedPage === totalPages - 1 ? activeTotalCount : Math.min(pageStart + pageSize, activeTotalCount);
  useEffect(() => {
    if (pageInputFocused.current) return;
    setPageInput(String(clampedPage + 1));
  }, [clampedPage]);
  const pageSolutions = (() => {
    if (filterActive) {
      // filterResults is already deduplicated by alg during the search scan.
      const rows: DisplaySolution[] = [];
      for (let i = pageStart; i < pageEnd; i++) {
        const solution = filterResults![i];
        if (solution) rows.push(displaySolution(solution));
      }
      return rows;
    }
    const seen = new Set<string>();
    const rows: DisplaySolution[] = [];
    const ramOffset = useLessRam ? ramOffsetRef.current : 0;
    for (let i = pageStart; i < pageEnd; i++) {
      const index = i - ramOffset;
      if (index < 0 || index >= solutions.length) continue;
      const row = displaySolution(solutions[index]);
      if (seen.has(row.alg)) continue;
      seen.add(row.alg);
      rows.push(row);
    }
    return rows;
  })();
  const collectAllDisplays = async () => {
    const seen = new Set<string>();
    const out: string[] = [];
    const add = (solution: Solution) => {
      const display = solutionDisplayText(solution);
      const alg = lineAlg(display);
      if (seen.has(alg)) return;
      seen.add(alg);
      out.push(display);
    };
    if (useLessRamRef.current) {
      const chunks = [...offloadedChunksRef.current.entries()].sort((a, b) => a[0] - b[0]);
      for (const [start] of chunks) {
        const raw = await readOffloadedChunk(start);
        if (raw) for (const s of raw) add(s);
      }
      for (const s of pendingTailRef.current ?? []) add(s);
    }
    for (const solution of solutionsRef.current) add(solution);
    return out;
  };
  const displayErgo = (solution: Solution) =>
    running || tableBusyMessage ? solutionErgo(solution) ?? solution.ergoRaw : solutionErgo(solution);
  const tableSolutions = [...pageSolutions].sort((a, b) => {
    if (showErgo && pageSolutions.some((row) => displayErgo(row) !== undefined)) {
      const aErgo = displayErgo(a), bErgo = displayErgo(b);
      const aNan = aErgo === undefined, bNan = bErgo === undefined;
      if (aNan && !bNan) return 1;
      if (!aNan && bNan) return -1;
      if (!aNan && !bNan && aErgo !== bErgo) return bErgo - aErgo;
    }
    if (a.slices !== b.slices) return a.slices - b.slices;
    return a.moves - b.moves;
  });
  const terminalSolutions = pageSolutions.map((solution, index) => {
    const ergo = displayErgo(solution);
    const suffix = showErgo && !running
      ? ergo === undefined ? "  (⚠)" : `  (${ergo.toFixed(2)})`
      : "";
    return { key: `sol-${solution.raw}-${index}`, text: solution.display + suffix, solution };
  });
  const outputLineText = (entry: OutputLine) => entry.isSolution ? notationStyle(karn ? entry.karn : entry.raw, outputMode, !!entry.sliceStart) : entry.raw;
  const notationCleanClass = outputMode === "clean" ? "notation-clean" : "";
  const terminalNonSolutions = outputLines
    .filter((entry) => !entry.isSolution)
    .map((entry, index) => ({ key: `line-${index}-${entry.raw}`, text: outputLineText(entry) }));
  const copyTerminalText = () => {
    const lines = outputLines.map(outputLineText);
    void navigator.clipboard.writeText(lines.join("\n"));
    setStatusLines((old) => [...old, t('terminal.copied')].slice(-8));
  };
  const renderSolutionText = (text: string) => {
    const showBarred = outputMode === "abid" || (karn && abidNotation);
    if (!showBarred) return text;
    const lb = text.lastIndexOf("[");
    if (lb <= 0) return <span className="abid-inline">{abidify(text)}</span>;
    return <><span className="abid-inline">{abidify(text.slice(0, lb).trim())}</span>{"  " + text.slice(lb).trim()}</>;
  };
  const tableBusyMessages = [
    tableBusyMessage,
    t('table.busyResolving'),
    t('table.busyRating'),
    t('table.busyNormalizing'),
    t('table.busyBuilding'),
  ].filter((message, index, all) => message && all.indexOf(message) === index);
  const tableBusyText = tableBusyMessages[tableBusyTick % tableBusyMessages.length] || tableBusyMessage;
  const updateOptionalLimit = (raw: string, min: number, max: number, setEnabled: (value: boolean) => void, setValue: (value: number) => void) => {
    if (raw.trim() === "") {
      setEnabled(false);
      return;
    }
    const parsed = Number(raw);
    if (!Number.isFinite(parsed)) return;
    // The true maximum (6 or 12) lives in the empty placeholder state, so any
    // typed value above the accessible range falls back into that placeholder.
    if (parsed > max) {
      setEnabled(false);
      return;
    }
    setEnabled(true);
    setValue(Math.max(min, Math.trunc(parsed)));
  };
  const stepOptionalLimit = (enabled: boolean, value: number, dir: 1 | -1, min: number, max: number, setEnabled: (value: boolean) => void, setValue: (value: number) => void) => {
    // The empty placeholder mode represents the value one above max (6 or 12),
    // and wraps the stepper range: up from max → placeholder → min, and
    // down from min → placeholder → max.
    if (!enabled) {
      setEnabled(true);
      setValue(dir === 1 ? min : max);
      return;
    }
    if (dir === 1) {
      if (value >= max) {
        setEnabled(false);
      } else {
        setValue(value + 1);
      }
    } else if (value <= min) {
      setEnabled(false);
    } else {
      setValue(value - 1);
    }
  };
  const renderOptionsPanel = () => (
    <div className="options-panel" ref={optionsPanelRef}>
      <div className="mobile-modal-head">
        <b>{t('options.heading')}</b>
        <button aria-label={t('btn.closeOptions')} onClick={() => setMobileOptionsOpen(false)}><Icon name="close" /></button>
      </div>
      <h2>{t('options.heading')}</h2>
      <div className="select-grid">
        <OptionDropdown id="metric" label={t('options.metric')} title={tooltips.metric} value={metric} options={["Slice", "Move", "Angle"]} disabled={running} open={openDropdown === "metric"} setOpen={setOpenDropdown} onChange={setMetric} />
        <OptionDropdown id="two" label={t('options.twoGen')} title={tooltips.twoGen} value={two} options={["None", "Pseudo 2 Gen", "2 Gen"]} disabled={running} open={openDropdown === "two"} setOpen={setOpenDropdown} onChange={setTwo} />
        <OptionDropdown id="angle" label={t('options.lockLayer')} title={tooltips.angle} value={angle} options={["None", "Both", "Top", "Bottom"]} disabled={running} open={openDropdown === "angle"} setOpen={setOpenDropdown} onChange={setAngle} />
        <OptionDropdown id="normalize" label={t('options.normalizeABF')} title={tooltips.normalize} value={normalize} options={["None", "Both", "PreABF", "PostABF"]} disabled={running} open={openDropdown === "normalize"} setOpen={setOpenDropdown} onChange={setNormalize} />
      </div>
      <div className="check-grid">
        <span className="generator-toggle">{t('options.output')} <span className="generator-toggle-value" title={tooltips.generator} onClick={() => !running && setGenerator((g) => !g)}>{generator ? t('options.outputValueScramble') : t('options.outputValueSolution')}</span></span>
        <label className="inline-all-optimal" title={tooltips.all}>
          <input type="checkbox" checked={all} disabled={running} onChange={(e) => setAll(e.target.checked)} />
          <span>{t('options.generateAll')}</span>
          <span className="all-optimal-label">{suboptimal && !specificDepthsActive ? `${t('options.optimal')}+${suboptimal}` : t('options.optimal')}</span>
          {!specificDepthsActive && <span className="stepper-group">
            <button type="button" title={tooltips.suboptimal} disabled={running || !all} onClick={() => setSuboptimal((value) => Math.max(0, value - 1))}>−</button>
            <button type="button" title={tooltips.suboptimal} disabled={running || !all} onClick={() => setSuboptimal((value) => value + 1)}>+</button>
          </span>}
        </label>
        <label title={cubeshapeDisableReason ?? tooltips.cubeshape}><input type="checkbox" checked={cubeShape} disabled={running || !isGoodSquares(cubeState) || cubeshapeBlockedBy2Gen} onChange={(e) => setCubeShapeMemory(e.target.checked)} /> {t('options.stayInCS')}</label>
      </div>
      <div className="limit-grid">
        <label title={tooltips.maxX}>{t('options.maxTop')}
          <div className="number-input-wrap">
            <input type="number" min="0" max="5" value={maxX ? maxXValue : ""} placeholder="6" disabled={running} onChange={(e) => updateOptionalLimit(e.target.value, 0, 5, setMaxX, setMaxXValue)} />
            <div className="number-stepper">
              <button type="button" className="top-stepper" title={tooltips.maxX} disabled={running} onClick={() => stepOptionalLimit(maxX, maxXValue, 1, 0, 5, setMaxX, setMaxXValue)}>▲</button>
              <button type="button" className="bottom-stepper" title={tooltips.maxX} disabled={running} onClick={() => stepOptionalLimit(maxX, maxXValue, -1, 0, 5, setMaxX, setMaxXValue)}>▼</button>
            </div>
          </div>
        </label>
        <label title={tooltips.maxY}>{t('options.maxBottom')}
          <div className="number-input-wrap">
            <input type="number" min="0" max="5" value={maxY ? maxYValue : ""} placeholder="6" disabled={running} onChange={(e) => updateOptionalLimit(e.target.value, 0, 5, setMaxY, setMaxYValue)} />
            <div className="number-stepper">
              <button type="button" className="top-stepper" title={tooltips.maxY} disabled={running} onClick={() => stepOptionalLimit(maxY, maxYValue, 1, 0, 5, setMaxY, setMaxYValue)}>▲</button>
              <button type="button" className="bottom-stepper" title={tooltips.maxY} disabled={running} onClick={() => stepOptionalLimit(maxY, maxYValue, -1, 0, 5, setMaxY, setMaxYValue)}>▼</button>
            </div>
          </div>
        </label>
        <label title={tooltips.maxTotal}>{t('options.maxTotal')}
          <div className="number-input-wrap">
            <input type="number" min="2" max="11" value={maxTotal ? maxTotalValue : ""} placeholder="12" disabled={running} onChange={(e) => updateOptionalLimit(e.target.value, 2, 11, setMaxTotal, setMaxTotalValue)} />
            <div className="number-stepper">
              <button type="button" className="top-stepper" title={tooltips.maxTotal} disabled={running} onClick={() => stepOptionalLimit(maxTotal, maxTotalValue, 1, 2, 11, setMaxTotal, setMaxTotalValue)}>▲</button>
              <button type="button" className="bottom-stepper" title={tooltips.maxTotal} disabled={running} onClick={() => stepOptionalLimit(maxTotal, maxTotalValue, -1, 2, 11, setMaxTotal, setMaxTotalValue)}>▼</button>
            </div>
          </div>
        </label>
        <label title={tooltips.depths}>{t('options.specificDepths')}<input type="text" value={depths} disabled={running} onChange={(e) => /^\s*\d*(?:\s*,\s*\d*)*\s*$/.test(e.target.value) && setDepths(e.target.value)} placeholder={t('options.placeholderDepth')} /></label>
      </div>
    </div>
  );
  const computeDebugStats = () => {
    const now = performance.now();
    const start = solveStartTimeRef.current;
    if (!start) return { elapsed: "—", solutionCount: 0, nodesSearched: 0 };
    const end = runningRef.current ? now : (solveStopTimeRef.current || now);
    const elapsed = ((end - start) / 1000).toFixed(1);
    const solutionCount = totalCountRefs();
    const nodesSearched = progressNodesRef.current;
    return { elapsed, solutionCount, nodesSearched };
  };
  const renderOutputShell = () => {
    const tableCols = tableView ? computeTableCols(tableWidth, tableMetricRef.current, showErgo, document.documentElement.classList.contains("bp-720")) : null;
    return (
    <div className={`terminal-shell ${outputToolsFaded ? "tools-faded" : ""}`} onMouseMove={markOutputToolsActive} onMouseLeave={() => setOutputToolsFaded(true)}>
      <div className="output-tools">
        <div className="output-tools-left">
          <span className="generator-toggle">{t('outputNotation')} <span className="generator-toggle-value" title={tooltips.karn} onClick={() => !running && setModal("notation")}>{t('karnSelect.' + outputMode)}</span></span>
        </div>
        <div className="output-tools-right">
          {debugOutput && <button title={t('btn.debugStats')} onClick={() => setModal("debug")}><Icon name="timer" /></button>}
          <button
            className={`filter-trigger ${filterOpen || filterActive ? "active" : ""}`}
            title={t('filter.find')}
            disabled={running || !totalCount}
            onClick={() => setFilterOpen((v) => !v)}
          ><Icon name="search" /></button>
          <button title={t('btn.copyAll')} disabled={!totalCount} onClick={copyTerminalText}><Icon name="copy" /></button>
          <button title={tableView ? t('btn.switchTerminalView') : t('btn.switchTableView')} onClick={() => tableView ? switchToTerminalMode() : switchToTableMode()}><Icon name={tableView ? "list" : "grid"} /></button>
          <button className="mobile-output-close" title={t('btn.close')} aria-label={t('btn.close')} onClick={() => setMobileOutputOpen(false)}><Icon name="close" /></button>
          <button className="expand-output" title={expanded ? t('btn.shrinkTerminal') : t('btn.expandTerminal')} onClick={() => setExpanded((v) => !v)}><Icon name={expanded ? "collapse" : "expand"} /></button>
        </div>
      </div>
      {filterOpen && <div className="filter-overlay" role="search">
        <input
          ref={filterInputRef}
          type="text"
          className="filter-input"
          placeholder={t('filter.find')}
          value={filterQuery}
          spellCheck={false}
          autoCorrect="off"
          autoCapitalize="off"
          onChange={(event) => setFilterQuery(event.target.value)}
          onKeyDown={(event) => { if (event.key === "Escape") { event.preventDefault(); setFilterOpen(false); } }}
        />
        <span className={`filter-count ${filterInvalid ? "filter-count-invalid" : ""}`}>
          {filterQuery.trim() === "" ? t('filter.noInput') :
            filterInvalid ? t('filter.invalidPattern') :
            filterSearching ? <><span className="filter-spinner" aria-hidden="true" /> {t('filter.results')}</> :
            `${formatCount(filterResults ? filterResults.length : 0)} ${t('filter.results')}`}
        </span>
        <button
          type="button"
          className={`filter-toggle ${filterMatchCase ? "active" : ""}`}
          title={t('filter.matchCase')}
          aria-pressed={filterMatchCase}
          onClick={() => setFilterMatchCase((v) => !v)}
        >Aa</button>
        <button
          type="button"
          className={`filter-toggle ${filterRegexMode ? "active" : ""}`}
          title={t('filter.regex')}
          aria-pressed={filterRegexMode}
          onClick={() => setFilterRegexMode((v) => !v)}
        >.*</button>
        <button type="button" className="filter-close" title={t('filter.close')} aria-label={t('filter.close')} onClick={() => setFilterOpen(false)}><Icon name="close" /></button>
      </div>}
      {!followTerminal && !tableView && completedWhilePaused && <button className="terminal-follow-button" title={t('btn.switchTableView')} onClick={() => { switchToTableMode(); setCompletedWhilePaused(false); }}><Icon name="grid" /></button>}
      {!followTerminal && !tableView && running && <button className="terminal-follow-button" title={t('btn.scrollBottom')} onClick={scrollTerminalToBottom}><Icon name="chevronDown" /></button>}
      {running && <button className="mobile-floating-stop" onClick={() => void solve()}>{t('btn.stopSolver')}</button>}
      {!showAll && totalPages > 1 && <div className={`page-switcher${pageSwitcherOpaque ? " page-switcher-opaque" : ""}`} ref={pageSwitcherRef}>
        <button className="page-switcher-btn page-switcher-prev" disabled={clampedPage === 0 || (useLessRam && isRestoring)} title={t('btn.prevPage')} onClick={(event) => { event.currentTarget.blur(); pageInputRef.current?.blur(); goToPage(clampedPage - 1, "bottom"); }}><Icon name="chevronLeft" /></button>
        <div className="page-switcher-center">
          <span className="page-switcher-inputwrap">
            <input
              ref={pageInputRef}
              className="page-switcher-input"
              type="text"
              inputMode="numeric"
              pattern="[0-9]*"
              name="sq1opt-page-index"
              id="sq1opt-page-index"
              autoComplete="one-time-code"
              autoCorrect="off"
              autoCapitalize="off"
              spellCheck={false}
              value={pageInput}
              aria-label={t('btn.goToPage')}
              onBlur={() => { pageInputFocused.current = false; commitPageInput(); }}
              onChange={(event) => setPageInput(event.target.value.replace(/\D/g, "").slice(0, 6))}
              onKeyDown={(event) => { if (event.key === "Enter") commitPageInput(); }}
            />
            <span className="page-switcher-inputshadow" aria-hidden="true">{pageInput || "0"}</span>
          </span>
          <span className="page-switcher-total">/ {totalPages}</span>
        </div>
        <button className="page-switcher-btn page-switcher-next" disabled={clampedPage >= totalPages - 1 || (useLessRam && isRestoring)} title={t('btn.nextPage')} onClick={(event) => { event.currentTarget.blur(); pageInputRef.current?.blur(); goToPage(clampedPage + 1, "top"); }}><Icon name="chevronRight" /></button>
      </div>}
      {/* Intentional feature by Abid: table columns reflect the metric at solve time, not the live metric dropdown. */}
      {tableView && tableCols ? <div ref={tableContainerRef} className={`terminal metric-${tableMetricRef.current.toLowerCase()} ${showErgo ? "with-ergo" : ""}`} onScroll={handleTableScroll} onWheel={handleTableWheel} onTouchStart={handleTouchStart} onTouchMove={handleTouchMove} onTouchEnd={handleTouchEnd} onTouchCancel={() => { touchNavRef.current = null; }}>
        <div className="terminal-head" style={{ gridTemplateColumns: tableCols.template }}>{tableCols.hash && <span>{t('table.hash')}</span>}<b>{t('table.solution')}</b>{tableCols.angle && tableMetricRef.current === "Angle" && <span>{t('table.angle')}</span>}{tableCols.move && tableMetricRef.current !== "Slice" && <span>{t('table.moves')}</span>}{tableCols.slices && <span>{t('table.slices')}</span>}{tableCols.ergo && showErgo && <span>{t('table.ergo')}</span>}</div>
        {tableSolutions.map((x, i) => {
          const ergo = displayErgo(x);
          return <div className="solution" style={{ gridTemplateColumns: tableCols.template }} key={x.raw} onMouseDown={(event) => {
            if (event.button !== 0 && event.button !== 2) return;
            event.preventDefault();
            if (event.button === 0) { if (contextMenu) setContextMenu(null); return; }
            setContextMenu({ x: event.clientX, y: event.clientY, alg: x.display });
          }} onContextMenu={(event) => event.preventDefault()}>{tableCols.hash && <span>{pageStart + i + 1}</span>}<code className={`${outputMode === "abid" || (karn && abidNotation) ? "abid" : ""} ${notationCleanClass}`}>{outputMode === "abid" || (karn && abidNotation) ? abidify(x.alg) : x.alg}</code>{tableCols.angle && tableMetricRef.current === "Angle" && <span>{x.angle}</span>}{tableCols.move && tableMetricRef.current !== "Slice" && <span>{x.moves}</span>}{tableCols.slices && <span>{x.slices}</span>}{tableCols.ergo && showErgo && <span>{ergo === undefined ? "…" : ergo.toFixed(1)}</span>}</div>;
        })}
        {filterActive && !filterResults!.length && <div className="empty">{t('filter.noMatches')}</div>}
        {tableBusyMessage && <div className="table-busy"><span className="table-busy-spinner" /><span>{tableBusyText}</span></div>}
        {useLessRam && isRestoring && <div className="table-busy"><span className="table-busy-spinner" /><span>{t('terminal.loadingPage')}</span></div>}
      </div> : <div ref={terminalTextRef} className="terminal terminal-text" onWheel={handleTerminalWheel} onScroll={handleTerminalScroll} onMouseDown={markTerminalUserActive} onTouchStart={handleTouchStart} onTouchMove={handleTouchMove} onTouchEnd={handleTouchEnd} onTouchCancel={() => { touchNavRef.current = null; scheduleTerminalUserInactive(); }}>
        {useLessRam && isRestoring && <span className="terminal-line terminal-line-status">{t('terminal.loadingPage')}</span>}
        {!outputLines.length && !totalCount && <span className="terminal-line terminal-line-empty">{generator ? t('terminal.emptyScramble') : t('terminal.emptySolution')}</span>}
        {filterActive && !filterResults!.length && <span className="terminal-line terminal-line-empty">{t('filter.noMatches')}</span>}
        {terminalNonSolutions.map((line) => <span key={line.key} className={`terminal-line terminal-line-status ${notationCleanClass}`}>{line.text || " "}</span>)}
        {terminalSolutions.map((line, index) => <span key={line.key} className={`terminal-line terminal-line-solution ${index % 2 ? "terminal-line-b" : "terminal-line-a"} ${notationCleanClass}`}
          onMouseDown={(event) => {
            if (event.button !== 0) return;
            event.preventDefault();
            if (contextMenu) setContextMenu(null);
          }}
          onContextMenu={(event) => { event.preventDefault(); setContextMenu({ x: event.clientX, y: event.clientY, alg: line.solution.display }); }}>{renderSolutionText(line.text)}</span>)}
        {statusLines.map((line, index) => <span key={`status-${index}-${line}`} className="terminal-line terminal-line-final">{line}</span>)}
      </div>}
    </div>
    );
  };
  return (
    <div className={`app ${expanded ? "output-expanded" : ""} ${mobileOptionsOpen ? "mobile-options-open" : ""} ${mobileOutputOpen ? "mobile-output-open" : ""}`} style={zoom === 1 ? undefined : { transform: `scale(${zoom})`, transformOrigin: "top left", width: `${100 / zoom}%`, height: `${100 / zoom}dvh` }}>
      <header>
        <img className="app-icon" src="/icon-web.png" alt="" />
        <div className="brand">
          <b>{t('app.brand')}</b><sub> &nbsp; &nbsp; {t('app.byline')}</sub>
        </div>
        <div className="top-menu-wrap">
          <button className="top-favorites-button" title={t('btn.favorites')} onClick={() => { setFavoritesOpen(true); setFavoritesOpening(true); }}><Icon name="heart" /></button>
          <button className="top-menu-button" aria-label={t('btn.openMenu')} aria-expanded={menu} title={tooltips.menu} onMouseDown={(event) => event.preventDefault()} onClick={() => setMenu((value) => !value)}>
            <Icon name="dots" />
          </button>
          {menu && (
            <div className="top-menu" onClick={(event) => event.stopPropagation()}>
              <button
                onClick={() => {
                  setModal("settings");
                  setMenu(false);
                }}
              >
                {t('btn.settings')}
              </button>
              <button
                onClick={() => {
                  setModal("how");
                  setMenu(false);
                }}
              >
                {t('btn.howToUse')}
              </button>
              <button
                onClick={() => {
                  setModal("about");
                  setMenu(false);
                }}
              >
                {t('btn.about')}
              </button>
            </div>
          )}
        </div>
      </header>
      {toast && <div className="toast" role="status">{toast}</div>}
      <div className="inputbar">
        <div className="mode-control" ref={modeControlRef}>
          <button className="mode" title={tooltips.inputMode} onMouseDown={(event) => event.preventDefault()} onClick={cycleMode}>
            {mode}
          </button>
          <button
            className="arrow"
            aria-label={t('tooltips.modeMenu')}
            title={tooltips.modeMenu}
            onMouseDown={(event) => event.preventDefault()}
            onClick={() => {
              setOpenDropdown(null);
              setModeMenu((v) => !v);
            }}
          >
            <Icon name="chevronDown" size={10} />
          </button>
          {modeMenu && (
            <div className="mode-menu">
              <button
                className={mode === "SCRAMBLE" ? "selected" : ""}
                onMouseDown={(event) => event.preventDefault()}
                onClick={() => chooseMode("SCRAMBLE")}
              >
                {t('input.scramble')}
              </button>
              <button
                className={mode === "ALG" ? "selected" : ""}
                onMouseDown={(event) => event.preventDefault()}
                onClick={() => chooseMode("ALG")}
              >
                {t('input.alg')}
              </button>
              <button
                className={mode === "POSITION" ? "selected" : ""}
                onMouseDown={(event) => event.preventDefault()}
                onClick={() => chooseMode("POSITION")}
              >
                {t('input.position')}
              </button>
            </div>
          )}
        </div>
        <div className="input-control">
          <input
            className={inputError ? "has-error" : ""}
            title={inputError}
            value={input}
            onChange={(e) => setInput(e.target.value)}
            onKeyDown={(e) => {
              if (e.key === "Enter") { e.preventDefault(); void apply(e.shiftKey); }
              if (e.key === "Escape") {
                e.preventDefault(); setUndo([]); setRedo([]); ignoreHistory.current = true;
                cubeActions.current?.reset(); ignoreHistory.current = false;
              }
            }}
            placeholder={
              mode === "POSITION"
                ? t('input.placeholderPosition')
                : t('input.placeholderMoves')
            }
          />
          <button className="apply" title={tooltips.apply} onClick={() => void apply()}>
            {t('btn.apply')}
          </button>
          {inputError && <span className="input-error">{inputError}</span>}
        </div>
      </div>
      <div className="main-grid" ref={mainGridRef}>
        <aside className="cube-column" ref={cubeColumnRef}>
          <Cube
            actionsRef={cubeActions}
            onChange={onCubeChange}
            onOptions={() => setMobileOptionsOpen(true)}
          />
          <div className="moves">
            <button title={t('cube.titleUp')} onClick={() => cubeActions.current?.up()}>U′</button>
            <button
              className="slice"
              title={t('cube.titleSlice')}
              onClick={() => cubeActions.current?.slice()}
            >
              {t('btn.slice')}
            </button>
            <button title={t('cube.titleU')} onClick={() => cubeActions.current?.u()}>U</button>
            <button title={t('cube.titleD')} onClick={() => cubeActions.current?.d()}>D</button>
            <button title={t('cube.titleDp')} onClick={() => cubeActions.current?.dp()}>D′</button>
          </div>
          <div className="undo">
            <button title={t('cube.titleUndo')} disabled={!undo.length || running} onClick={doUndo}>{t('btn.undo')}</button>
            <button title={t('cube.titleRedo')} disabled={!redo.length || running} onClick={doRedo}>{t('btn.redo')}</button>
          </div>
          <button className={`solve ${running ? "is-running" : ""}`} disabled={!running && twoGenBlocked} title={running ? t('cube.titleSolve') : twoGenBlocked ? t('cube.titleSolveBlocked') : commandPreview} onClick={() => void solve()}>{running ? t('btn.stop') : t('btn.solve')}</button>
          <button className="mobile-open-output" onClick={openMobileOutput}>{t('btn.openOutput')}</button>
        </aside>
        {renderOptionsPanel()}
        {renderOutputShell()}
      </div>
      {contextMenu && <div className="solution-context" style={{
        left: Math.max(0, Math.min(contextMenu.x, window.innerWidth - 180)),
        top: Math.max(0, Math.min(contextMenu.y, window.innerHeight - 80)),
      }} onClick={(event) => event.stopPropagation()} onContextMenu={(event) => event.preventDefault()}>
        <button onClick={() => { void navigator.clipboard.writeText(lineWithoutBracket(contextMenu.alg)); setContextMenu(null); }}>{t('btn.copyAlg')}</button>
        <button onClick={() => {
          const key = currentRunKey();
          setFavorites((old) => ({ ...old, [key]: {
            name: old[key]?.name || `Position ${Object.keys(old).length + 1}`,
            algorithms: Array.from(new Set([...(old[key]?.algorithms || []), contextMenu.alg])),
          } }));
          setContextMenu(null);
          setFavoritesOpen(true);
          setFavoritesOpening(true);
        }}>{t('btn.addFavorite')}</button>
      </div>}
      {createPortal(<>
      {modal && <Modal type={modal} close={() => history.back()} settings={{
        ignoreTransforms, setIgnoreTransforms,
        debugOutput, setDebugOutput, zoom, setZoom, disabled: running, hasMaxTurn: maxX || maxY || maxTotal, language: lang,
        setLanguage: (code) => {
          setLang(code);
          setLangState(code);
          setToast(t('toast.languageSet'));
          if (toastTimerRef.current !== undefined) window.clearTimeout(toastTimerRef.current);
          toastTimerRef.current = window.setTimeout(() => setToast(null), 2600);
        },
        pageSize, setPageSize, showAll, setShowAll, pageSizeOptions: PAGE_SIZE_OPTIONS,
        useLessRam, setUseLessRam,
        onRequestShowAll: () => setShowAllConfirm(true),
        onOpenDiskSpace: () => setDiskOpen(true),
      }} notation={{
        outputMode, setOutputMode, abidNotation, setAbidNotation, disabled: running,
      }} debugStats={modal === "debug" ? computeDebugStats() : null} liveDebug={modal === "debug" ? () => ({
        now: performance.now(),
        running: runningRef.current,
        startTime: solveStartTimeRef.current,
        stopTime: solveStopTimeRef.current,
        history: rateHistoryRef.current,
        totalSolutions: totalCountRefs(),
        totalNodes: progressNodesRef.current,
      }) : null} />}
      {modal === "settings" && diskOpen && <DiskSpaceModal onClose={() => setDiskOpen(false)} deleteOnQuit={deleteTablesOnQuit} setDeleteOnQuit={setDeleteTablesOnQuit} solutions={solutions} onClearSolutions={clearSolutions} />}
      {showAllConfirm && <div className="modal-shade modal-shade-top" onClick={() => setShowAllConfirm(false)}>
        <div className="modal modal-confirm" onClick={(event) => event.stopPropagation()}>
          <button className="modal-close" onClick={() => setShowAllConfirm(false)}><Icon name="close" /></button>
          <h2>{t('modal.showAll.title')}</h2>
          <p>{t('modal.showAll.warning')}</p>
          <div className="modal-confirm-actions">
            <button onClick={() => setShowAllConfirm(false)}>{t('modal.showAll.cancel')}</button>
            <button className="modal-confirm-primary" onClick={() => { setShowAll(true); setShowAllConfirm(false); }}>{t('modal.showAll.confirm')}</button>
          </div>
        </div>
      </div>}
      {(favoritesOpen || favoritesClosing) && <div className={"modal-shade favorites-shade" + (favoritesClosing ? " closing" : "")} onPointerDown={(e) => { favShadeStartRef.current = e.target; }} onPointerUp={(e) => { favShadeEndRef.current = e.target; }} onClick={() => {
        const startOutside = !favShadeStartRef.current || !(favShadeStartRef.current as Element).closest(".favorites-modal");
        const endOutside = !favShadeEndRef.current || !(favShadeEndRef.current as Element).closest(".favorites-modal");
        if (startOutside && endOutside) beginCloseFavorites();
        favShadeStartRef.current = null;
        favShadeEndRef.current = null;
      }}>
        <div ref={favModalRef} className={"modal favorites-modal" + (favoritesClosing ? " closing" : "") + (favoritesOpening ? " opening" : "")} style={favoritesClosing ? { transformOrigin: `${favClosingOriginRef.current.x}% ${favClosingOriginRef.current.y}%` } : undefined} onAnimationEnd={favoritesClosing ? onFavCloseAnimEnd : favoritesOpening ? onFavOpenAnimEnd : undefined} onClick={(event) => event.stopPropagation()}>
          <button className="modal-close" onClick={beginCloseFavorites}><Icon name="close" /></button>
          <h2>{t('favorites.heading')}</h2>
          {!!totalCount && <button className="favorite-save" onClick={() => void (async () => {
            const key = currentRunKey();
            const algs = await collectAllDisplays();
            setFavorites((old) => ({ ...old, [key]: {
              name: old[key]?.name || `Position ${Object.keys(old).length + 1}`,
              algorithms: Array.from(new Set([...(old[key]?.algorithms || []), ...algs])),
            } }));
          })()}>{t('btn.saveSolutions')}</button>}
          {!Object.keys(favorites).length && <p>{t('favorites.empty')}</p>}
          {Object.entries(favorites).map(([key, bin]) => {
            const binDeleteKey = `bin::${key}`;
            const isBinPending = binDeleteKey in pendingDeletes;
            return <section className={`favorite-bin${isBinPending ? " favorite-bin-deleted" : ""}`} key={key}>
            <div className="favorite-bin-head">
              {isBinPending ? <><span>{t('favorites.binDeleted')}</span> <button className="favorite-undo" onClick={() => { clearTimeout(pendingDeletes[binDeleteKey]); setPendingDeletes((old) => { const next = { ...old }; delete next[binDeleteKey]; return next; }); }}>{t('favorites.undo')}</button>?</> : <>
              <input value={bin.name} aria-label="Favorite name" onChange={(event) => setFavorites((old) => ({ ...old, [key]: { ...old[key], name: event.target.value } }))} />
              <div className="favorite-actions">
                <button title={t('btn.applySetup')} onClick={() => applyRunConfig(key)}>{t('btn.applySetup')}</button>
                <button title={t('btn.copyAllAlgs')} onClick={() => void navigator.clipboard.writeText(bin.algorithms.map(lineWithoutBracket).join("\n"))}><Icon name="copy" /></button>
                <button title={t('btn.deleteBin')} onClick={() => {
                  const timer = window.setTimeout(() => {
                    setPendingDeletes((old) => { const next = { ...old }; delete next[binDeleteKey]; return next; });
                    setFavorites((old) => { const next = { ...old }; delete next[key]; return next; });
                  }, 5000);
                  setPendingDeletes((old) => ({ ...old, [binDeleteKey]: timer }));
                }}><Icon name="trash" /></button>
              </div>
              </>}
            </div>
            {!isBinPending && <ul className="favorite-algs">
              {bin.algorithms.map((alg, i) => {
                const deleteKey = `${key}::${alg}`;
                const isPending = deleteKey in pendingDeletes;
                return <li key={`${alg}-${i}`} className={isPending ? "favorite-alg-deleted" : ""}>
                  {isPending ? <><span>{t('favorites.algDeleted')}</span> <button className="favorite-undo" onClick={() => { clearTimeout(pendingDeletes[deleteKey]); setPendingDeletes((old) => { const next = { ...old }; delete next[deleteKey]; return next; }); }}>{t('favorites.undo')}</button>?</> : <>
                    <code>{alg}</code>
                    <button className="favorite-alg-copy" title={t('btn.copyAlgSmall')} onClick={() => void navigator.clipboard.writeText(lineWithoutBracket(alg))}><Icon name="copy" size={12} /></button>
                    <button className="favorite-alg-remove" title={t('btn.remove')} onClick={() => {
                      const timer = window.setTimeout(() => {
                        setPendingDeletes((old) => { const next = { ...old }; delete next[deleteKey]; return next; });
                        setFavorites((old) => {
                          const bin = old[key];
                          if (!bin) return old;
                          const remaining = bin.algorithms.filter((_, j) => j !== i);
                          if (remaining.length === 0) { const next = { ...old }; delete next[key]; return next; }
                          return { ...old, [key]: { ...bin, algorithms: remaining } };
                        });
                      }, 5000);
                      setPendingDeletes((old) => ({ ...old, [deleteKey]: timer }));
                    }}><Icon name="close" size={12} /></button>
                  </>}
                </li>;
              })}
            </ul>}
          </section>})}
        </div>
      </div>}
      </>, document.body)}
    </div>
  );
}
