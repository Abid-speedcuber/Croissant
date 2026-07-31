import { useEffect, useLayoutEffect, useRef, useState } from "react";
import { createPortal } from "react-dom";
import { loadSettings, saveSettings, loadFavorites, saveFavorites } from "./storage";
import {
  CubeState, Modal as ModalType, FavoriteBin, Solution, OutputLine, RatingResult, TwoGenStatus,
  DisplaySolution, DropdownProps, CubeActions, TauriGlobal,
  twistable, getLayerR, getParityOdd, inCubeshape, doMove, tauri, validDepths, solverFlags,
  positionString, rawPosition, parsePosition, invertScramble, addCommas, applyNumericAlgorithm,
  abidify, injectSliceIndicator, lineAlg, lineWithoutBracket, parseSolutionCounts,
  ratingScore, ratingSliceStart, solutionErgo, medianNormalize, normalizeLine, tooltips,
} from "./utils";
import { Modal } from './components/Modal';
import { t, LangCode, getLang, setLang } from './i18n';

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
          <span className="option-dropdown-arrow">▾</span>
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
          width: 231,
          height: 385,
          transform: "none",
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
  const debugStatsRef = useRef<{ solutionTimestamps: number[]; rateSamples: number[] }>({ solutionTimestamps: [], rateSamples: [] });
  const progressNodesRef = useRef(0);
  const progressDepthRef = useRef(0);
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
  const [menu, setMenu] = useState(false),
    [lang, setLangState] = useState<LangCode>(getLang()),
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
    [all, setAll] = useState(false),
    [generator, setGenerator] = useState(false),
    [cubeShapeMemory, setCubeShapeMemory] = useState(false),
    [ignoreMiddle, setIgnoreMiddle] = useState(false),
    [karn, setKarn] = useState(true),
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
    [zoom, setZoom] = useState(1),
    [smartKarn, setSmartKarn] = useState(true), [abidNotation, setAbidNotation] = useState(false),
    [ignoreTransforms, setIgnoreTransforms] = useState(false), [debugOutput, setDebugOutput] = useState(false);
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
  const terminalScrollPositionRef = useRef(0);
  const tableScrollPositionRef = useRef(0);
  const firstTableSwitchAfterSolveRef = useRef(true);
  const isSwitchingViewRef = useRef(false);
  const zoomRef = useRef(1);
  const cubeColumnRef = useRef<HTMLDivElement>(null);
  const tableMetricRef = useRef("Slice");
  const mainGridRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    const el = document.documentElement;
    const update = () => el.classList.toggle("tall-viewport", window.innerHeight / zoomRef.current >= 810);
    update();
    window.addEventListener("resize", update);
    return () => window.removeEventListener("resize", update);
  }, []);
  useEffect(() => {
    zoomRef.current = zoom;
    document.documentElement.classList.toggle("tall-viewport", window.innerHeight / zoom >= 810);
  }, [zoom]);
  useEffect(() => {
    const updateBreakpoints = () => {
      const effW = window.innerWidth / zoomRef.current;
      document.documentElement.classList.toggle("bp-720", effW <= 860);
      document.documentElement.classList.toggle("bp-620", effW <= 620);
      document.documentElement.classList.toggle("bp-460", effW <= 460);
    };
    updateBreakpoints();
    window.addEventListener("resize", updateBreakpoints);
    return () => window.removeEventListener("resize", updateBreakpoints);
  }, []);
  useEffect(() => {
    const update = () => {
      const effW = window.innerWidth / zoomRef.current;
      document.documentElement.classList.toggle("bp-720", effW <= 860);
      document.documentElement.classList.toggle("bp-620", effW <= 620);
      document.documentElement.classList.toggle("bp-460", effW <= 460);
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
      setFavoritesOpen(false);
      setFavoritesClosing(false);
    };
    window.addEventListener("popstate", onPop);
    return () => window.removeEventListener("popstate", onPop);
  }, [modal, favoritesOpen]);
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
  }, []);
  const scrollTerminalToBottom = () => {
    const node = terminalTextRef.current;
    if (!node) return;
    followTerminalRef.current = true;
    setFollowTerminal(true);
    setCompletedWhilePaused(false);
    node.scrollTop = node.scrollHeight;
  };
  const openMobileOutput = () => {
    setMobileOutputOpen(true);
    requestAnimationFrame(scrollTerminalToBottom);
  };
  const handleTerminalScroll = () => {
    const node = terminalTextRef.current;
    if (!node) return;
    const prev = terminalScrollPositionRef.current;
    const next = node.scrollTop;
    terminalScrollPositionRef.current = next;
    if (running && next < prev) {
      const nearBottom = node.scrollHeight - next - node.clientHeight < 50;
      if (!nearBottom && followTerminalRef.current) {
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
      terminalScrollPositionRef.current = 0;
      tableScrollPositionRef.current = 0;
      firstTableSwitchAfterSolveRef.current = false;
    }
    isSwitchingViewRef.current = true;
    setTableView(true);
  };
  const finishTableBusySoon = () => {
    requestAnimationFrame(() => requestAnimationFrame(() => setTableBusyMessage("")));
  };
  const switchToTerminalMode = () => {
    if (tableContainerRef.current) {
      tableScrollPositionRef.current = tableContainerRef.current.scrollTop;
    }
    isSwitchingViewRef.current = true;
    setTableView(false);
  };
  useLayoutEffect(() => {
    if (tableView && tableContainerRef.current) {
      tableContainerRef.current.scrollTop = tableScrollPositionRef.current;
    } else if (!tableView && terminalTextRef.current) {
      terminalTextRef.current.scrollTop = terminalScrollPositionRef.current;
    }
  }, [tableView]);
  useEffect(() => {
    if (followTerminal && !isSwitchingViewRef.current) {
      requestAnimationFrame(() => {
        if (followTerminalRef.current) {
          const node = terminalTextRef.current;
          if (node) node.scrollTop = node.scrollHeight;
        }
      });
    }
  }, [outputLines, statusLines, solutions, tableView, running, followTerminal]);
  useEffect(() => {
    isSwitchingViewRef.current = false;
  }, [tableView]);
  useEffect(() => {
    if (!tableBusyMessage) return;
    const id = window.setInterval(() => setTableBusyTick((value) => value + 1), 900);
    return () => window.clearInterval(id);
  }, [tableBusyMessage]);
  useEffect(() => {
    if (modal !== "debug") return;
    const id = setInterval(() => {
      if (!runningRef.current || !solveStartTimeRef.current) return;
      const stats = debugStatsRef.current;
      const now = performance.now();
      const elapsedTotal = (now - solveStartTimeRef.current) / 1000;
      const windowDur = Math.min(60, Math.max(0, elapsedTotal));
      const cutoff = now - windowDur * 1000;
      stats.solutionTimestamps = stats.solutionTimestamps.filter(t => t > cutoff);
      const rate = windowDur > 0 ? stats.solutionTimestamps.length / windowDur * 60 : 0;
      stats.rateSamples.push(rate);
      setDebugTick(t => t + 1);
    }, 5000);
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
    try {
      const converted = await tauri()?.core?.invoke<string>("karnify", {
        input: rawAlg,
        position: smartKarn && !lastSolveCubeShape.current ? startPosition : null,
        generator,
      });
      if (converted) karnDisplay = `${converted}  ${line.slice(lb).trim()}`;
    } catch { /* retain numeric output */ }
    return {
      rawDisplay: injectSliceIndicator(line, sliceStart),
      karnDisplay: injectSliceIndicator(karnDisplay, sliceStart),
    };
  };
  /*
   * LIVE STREAMING — INTENTIONAL FEATURE (Abid)
   *
   * Solutions appear one-by-one in the terminal as the solver emits them, similar
   * to how AI UIs stream tokens. This is THE magic of the app. It MUST NOT be
   * traded away for raw speed — no batching, no hiding behind a spinner, no
   * waiting for "all results" before showing anything. Every solution must be
   * flushed to the terminal as soon as it is available.
   *
   * The only acceptable optimization is to defer EXPENSIVE post-processing
   * (karnify, rating) while still showing the raw algorithm immediately.
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
    solutionsRef.current = [...solutionsRef.current, row];
    scheduleSolutionFlush();
    debugStatsRef.current.solutionTimestamps.push(performance.now());
  };
  const setSolutionRows = (rows: Solution[]) => {
    solutionsRef.current = rows;
    scheduleSolutionFlush();
  };
  const receiveSolverLine = async (line: string, startPosition: string, runId: number) => {
    if (stopped.current) return;
    if (runId !== solveRunId.current) return;
    if (line.startsWith("__PROGRESS__")) {
      if (!solveStartTimeRef.current) solveStartTimeRef.current = performance.now();
      const nm = line.match(/nodes=(\d+)/), dm = line.match(/depth=(\d+)/);
      if (nm) progressNodesRef.current = parseInt(nm[1], 10);
      if (dm) progressDepthRef.current = parseInt(dm[1], 10);
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
    let rawDisplay: string, karnDisplay: string;
    if (afterMetrics) {
      const rateStart = afterMetrics.indexOf(" R{");
      const karnEnd = rateStart >= 0 ? rateStart : afterMetrics.length;
      const karnified = afterMetrics.slice(0, karnEnd).trim();
      if (rateStart >= 0) {
        try {
          const raw = JSON.parse(afterMetrics.slice(rateStart + 2));
          rating = { finalScore: raw.f, sliceStart: raw.ss, phase1: raw.p1, phase2: raw.p2, phase3: raw.p3, phase4: raw.p4, ergoUp: raw.eu, ergoDown: raw.ed, sliceCount: raw.sc, movement: raw.mv, bonus: raw.bn, valid: true };
          if (rating.valid) sliceStart = ratingSliceStart(rating);
        } catch { /* unrated */ }
      }
      rawDisplay = injectSliceIndicator(rawAlg + "  " + metricsPart, sliceStart);
      karnDisplay = injectSliceIndicator(karnified + "  " + metricsPart, sliceStart);
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
    const displayAlg = lineAlg(normalizeLine(karn ? karnDisplay : rawDisplay, normalize));
    if (seenDisplay.current.has(displayAlg)) return;
    seenDisplay.current.add(displayAlg);
    if (seenRaw.current.size === 1) {
      firstSolutionAt.current = performance.now();
      if (!solveStartTimeRef.current) solveStartTimeRef.current = firstSolutionAt.current;
      if (!debugOutput) replaceOutputLines(outputLinesRef.current.filter((entry) => entry.isSolution));
    }
    const counts = parseSolutionCounts(line);
    const cleanLine = rawAlg + "  " + metricsPart;
    const row: Solution = { raw: cleanLine, rawDisplay, karnDisplay, algRaw: rawAlg, ...counts, ergoRaw: rating?.valid ? ratingScore(rating) : undefined, sliceStart };
    addSolution(row);
    addOutputLine({ raw: rawDisplay, karn: karnDisplay, isSolution: true, algRaw: rawAlg });
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
    if (debugOutput) flags.push("-v7");
    stopped.current = false;
    solutionsRef.current = [];
    outputLinesRef.current = [];
    seenRaw.current.clear();
    seenDisplay.current.clear();
    lineQueue.current = Promise.resolve();
    firstSolutionAt.current = 0;
    solveStartTimeRef.current = 0;
    solveStopTimeRef.current = 0;
    debugStatsRef.current = { solutionTimestamps: [], rateSamples: [] };
    progressNodesRef.current = 0;
    progressDepthRef.current = 0;
    followTerminalRef.current = true;
    lastSolveCubeShape.current = cubeShape;
    firstTableSwitchAfterSolveRef.current = true;
    tableMetricRef.current = metric;
    terminalScrollPositionRef.current = 0;
    tableScrollPositionRef.current = 0;
    setRunCubeShape(cubeShape);
    const solvingMsg = t('status.solving');
    setOutputLines([{ raw: solvingMsg, karn: solvingMsg, isSolution: false }]);
    outputLinesRef.current = [{ raw: solvingMsg, karn: solvingMsg, isSolution: false }];
    setStatusLines([]);
    setSolutions([]);
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
      if (shouldAutoTable && solutionsRef.current.length >= 2) {
        switchToTableMode();
        setTableBusyMessage(t('table.busyResolving'));
      }
      await lineQueue.current;
      if (shouldAutoTable) setTableBusyMessage(t('table.busyRating'));
      for (const line of `${result.stderr || ""}`.split(/\r?\n/).filter(Boolean))
        await receiveSolverLine(line, start, runId);
      if (runId !== solveRunId.current) return;
      if (lastSolveCubeShape.current) {
        if (shouldAutoTable && solutionsRef.current.length) setTableBusyMessage(t('table.busyNormalizing'));
        setSolutionRows(medianNormalize(solutionsRef.current));
      }
      if (shouldAutoTable) setTableBusyMessage(t('table.busyBuilding'));
      flushSolutionState();
      const count = solutionsRef.current.length;
      const status = `${stopped.current ? t('status.stopped') : result.code === 0 ? t('status.done') : t('status.error')} — ${count} solution${count === 1 ? "" : "s"} found in ${((performance.now() - startedAt) / 1000).toFixed(2)}s.`;
      setStatusLines(lastSolveCubeShape.current && count ? [t('status.ranked').replace('{{count}}', String(count))] : [status]);
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
      if (!value) return;
      if (typeof value.smartKarn === "boolean") setSmartKarn(value.smartKarn);
      if (typeof value.abidNotation === "boolean") setAbidNotation(value.abidNotation);
      if (typeof value.ignoreTransforms === "boolean") setIgnoreTransforms(value.ignoreTransforms);
      if (typeof value.debugOutput === "boolean") setDebugOutput(value.debugOutput);
      if (typeof value.karn === "boolean") setKarn(value.karn);
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
      queueMicrotask(() => { settingsReady.current = true; });
    });
  }, []);
  useEffect(() => {
    if (!settingsReady.current) return;
    void saveSettings({
      smartKarn, abidNotation, ignoreTransforms, debugOutput, karn, normalize, mode,
      metric, two, angle, all, suboptimal, depths, generator, cubeShape: cubeShapeMemory, ignoreMiddle,
      maxX, maxXValue, maxY, maxYValue, maxTotal, maxTotalValue, zoom,
    });
  }, [smartKarn, abidNotation, ignoreTransforms, debugOutput, karn, normalize, mode, metric, two, angle, all, suboptimal, depths, generator, cubeShapeMemory, ignoreMiddle, maxX, maxXValue, maxY, maxYValue, maxTotal, maxTotalValue, zoom]);
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
  }, [smartKarn, generator]);
  useEffect(() => {
    let cancelled = false;
    if (!tauri()?.core?.invoke) return;
    void tauri()!.core!.invoke<TwoGenStatus>("two_gen_status", { position: rawPosition(cubeState) })
      .then((status) => { if (!cancelled) setTwoGenStatus(status); })
      .catch((err) => console.error("two_gen_status invoke failed:", err));
    return () => { cancelled = true; };
  }, [cubeState]);
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
  const cubeshapeBlockedBy2Gen = (two === "2 Gen" && !twoGenStatus.cornersTwo) ||
    (two === "Pseudo 2 Gen" && !twoGenStatus.cornersPseudo);
  const cubeshapeForced = !inCubeshape(cubeState) || cubeshapeBlockedBy2Gen;
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
  const commandPreview = `croissant ${commandFlags.join(" ")} ${positionString(cubeState)}`;
  const showErgo = runCubeShape;
  const displaySolution = (solution: Solution): DisplaySolution => {
    const display = normalizeLine(karn ? solution.karnDisplay : solution.rawDisplay, normalize);
    return { ...solution, display, alg: lineAlg(display) };
  };
  const visibleSolutions = (() => {
    const seen = new Set<string>();
    const rows: DisplaySolution[] = [];
    for (const solution of solutions) {
      const row = displaySolution(solution);
      if (seen.has(row.alg)) continue;
      seen.add(row.alg);
      rows.push(row);
    }
    return rows;
  })();
  const displayErgo = (solution: Solution) =>
    running || tableBusyMessage ? solutionErgo(solution) ?? solution.ergoRaw : solutionErgo(solution);
  const tableSolutions = [...visibleSolutions].sort((a, b) => {
    if (showErgo && solutions.some((row) => displayErgo(row) !== undefined)) {
      const aErgo = displayErgo(a), bErgo = displayErgo(b);
      const aNan = aErgo === undefined, bNan = bErgo === undefined;
      if (aNan && !bNan) return 1;
      if (!aNan && bNan) return -1;
      if (!aNan && !bNan && aErgo !== bErgo) return bErgo - aErgo;
    }
    if (a.slices !== b.slices) return a.slices - b.slices;
    return a.moves - b.moves;
  });
  const terminalSolutions = visibleSolutions.map((solution, index) => {
    const ergo = displayErgo(solution);
    const suffix = showErgo && !running
      ? ergo === undefined ? "  (⚠)" : `  (${ergo.toFixed(2)})`
      : "";
    return { key: `sol-${solution.raw}-${index}`, text: solution.display + suffix, solution };
  });
  const terminalNonSolutions = outputLines
    .filter((entry) => !entry.isSolution)
    .map((entry, index) => ({ key: `line-${index}-${entry.raw}`, text: karn ? entry.karn : entry.raw }));
  const copyTerminalText = () => {
    const lines = outputLines.map((entry) => karn ? entry.karn : entry.raw);
    void navigator.clipboard.writeText(lines.join("\n"));
    setStatusLines((old) => [...old, t('terminal.copied')].slice(-8));
  };
  const renderSolutionText = (text: string) => {
    if (!abidNotation) return text;
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
    setEnabled(true);
    setValue(Math.min(max, Math.max(min, Math.trunc(parsed))));
  };
  const renderOptionsPanel = () => (
    <div className="options-panel">
      <div className="mobile-modal-head">
        <b>{t('options.heading')}</b>
        <button aria-label={t('btn.closeOptions')} onClick={() => setMobileOptionsOpen(false)}>✕</button>
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
        <label title={cubeshapeDisableReason ?? tooltips.cubeshape}><input type="checkbox" checked={cubeShape} disabled={running || !inCubeshape(cubeState) || cubeshapeBlockedBy2Gen} onChange={(e) => setCubeShapeMemory(e.target.checked)} /> {t('options.stayInCS')}</label>
      </div>
      <div className="limit-grid">
        <label title={tooltips.maxX}>{t('options.maxTop')}
          <div className="number-input-wrap">
            <input type="number" min="0" max="6" value={maxX ? maxXValue : ""} placeholder="6" disabled={running} onChange={(e) => updateOptionalLimit(e.target.value, 0, 6, setMaxX, setMaxXValue)} />
            <div className="number-stepper">
              <button type="button" className="top-stepper" title={tooltips.maxX} disabled={running} onClick={() => { setMaxX(true); setMaxXValue((value) => Math.min(6, (maxX ? value : 0) + 1)); }}>▲</button>
              <button type="button" className="bottom-stepper" title={tooltips.maxX} disabled={running} onClick={() => { setMaxX(true); setMaxXValue((value) => Math.max(0, (maxX ? value : 1) - 1)); }}>▼</button>
            </div>
          </div>
        </label>
        <label title={tooltips.maxY}>{t('options.maxBottom')}
          <div className="number-input-wrap">
            <input type="number" min="0" max="6" value={maxY ? maxYValue : ""} placeholder="6" disabled={running} onChange={(e) => updateOptionalLimit(e.target.value, 0, 6, setMaxY, setMaxYValue)} />
            <div className="number-stepper">
              <button type="button" className="top-stepper" title={tooltips.maxY} disabled={running} onClick={() => { setMaxY(true); setMaxYValue((value) => Math.min(6, (maxY ? value : 0) + 1)); }}>▲</button>
              <button type="button" className="bottom-stepper" title={tooltips.maxY} disabled={running} onClick={() => { setMaxY(true); setMaxYValue((value) => Math.max(0, (maxY ? value : 1) - 1)); }}>▼</button>
            </div>
          </div>
        </label>
        <label title={tooltips.maxTotal}>{t('options.maxTotal')}
          <div className="number-input-wrap">
            <input type="number" min="1" max="12" value={maxTotal ? maxTotalValue : ""} placeholder="12" disabled={running} onChange={(e) => updateOptionalLimit(e.target.value, 1, 12, setMaxTotal, setMaxTotalValue)} />
            <div className="number-stepper">
              <button type="button" className="top-stepper" title={tooltips.maxTotal} disabled={running} onClick={() => { setMaxTotal(true); setMaxTotalValue((value) => Math.min(12, (maxTotal ? value : 0) + 1)); }}>▲</button>
              <button type="button" className="bottom-stepper" title={tooltips.maxTotal} disabled={running} onClick={() => { setMaxTotal(true); setMaxTotalValue((value) => Math.max(1, (maxTotal ? value : 2) - 1)); }}>▼</button>
            </div>
          </div>
        </label>
        <label title={tooltips.depths}>{t('options.specificDepths')}<input type="text" value={depths} disabled={running} onChange={(e) => /^\s*\d*(?:\s*,\s*\d*)*\s*$/.test(e.target.value) && setDepths(e.target.value)} placeholder={t('options.placeholderDepth')} /></label>
      </div>
    </div>
  );
  const computeDebugStats = () => {
    const stats = debugStatsRef.current;
    const now = performance.now();
    const start = solveStartTimeRef.current;
    if (!start) return { elapsed: "—", solutionCount: 0, rollingRate: 0, avgRate: 0, stddevRate: 0, nodesSearched: 0, searchDepth: 0, nodeRate: 0 };
    const end = runningRef.current ? now : (solveStopTimeRef.current || now);
    const elapsed = ((end - start) / 1000).toFixed(1);
    const solutionCount = solutionsRef.current.length;
    const elapsedTotal = (end - start) / 1000;
    const windowDur = Math.min(60, Math.max(0, elapsedTotal));
    const cutoff = end - windowDur * 1000;
    const recent = stats.solutionTimestamps.filter(t => t > cutoff);
    const rollingRate = windowDur > 0 ? recent.length / windowDur * 60 : 0;
    const samples = stats.rateSamples;
    const avg = samples.length ? samples.reduce((a, b) => a + b, 0) / samples.length : 0;
    const stddev = samples.length > 1 ? Math.sqrt(samples.reduce((sum, v) => sum + (v - avg) ** 2, 0) / samples.length) : 0;
    const nodesSearched = progressNodesRef.current;
    const searchDepth = progressDepthRef.current;
    const nodeRate = elapsedTotal > 0 ? nodesSearched / elapsedTotal : 0;
    return { elapsed, solutionCount, rollingRate, avgRate: avg, stddevRate: stddev, nodesSearched, searchDepth, nodeRate };
  };
  const renderOutputShell = () => (
    <div className={`terminal-shell ${outputToolsFaded ? "tools-faded" : ""}`} onMouseMove={markOutputToolsActive} onMouseLeave={() => setOutputToolsFaded(true)}>
      <div className="output-tools">
        <div className="output-tools-left">
          <span className="generator-toggle">{t('outputNotation')} <span className="generator-toggle-value" title={tooltips.karn} onClick={() => !running && setKarn((k) => !k)}>{karn ? t('karnSelect.karn') : t('karnSelect.normal')}</span></span>
        </div>
        <div className="output-tools-right">
          {debugOutput && <button title={t('btn.debugStats')} onClick={() => setModal("debug")}>⏱</button>}
          <button title={t('btn.copyAll')} disabled={!solutions.length} onClick={copyTerminalText}>⧉</button>
          <button title={tableView ? t('btn.switchTerminalView') : t('btn.switchTableView')} onClick={() => tableView ? switchToTerminalMode() : switchToTableMode()}>{tableView ? "▤" : "⊞"}</button>
          <button className="mobile-output-close" title={t('btn.close')} aria-label={t('btn.close')} onClick={() => setMobileOutputOpen(false)}>×</button>
          <button className="expand-output" title={expanded ? t('btn.shrinkTerminal') : t('btn.expandTerminal')} onClick={() => setExpanded((v) => !v)}>{expanded ? "–" : "⤢"}</button>
        </div>
      </div>
      {!followTerminal && !tableView && completedWhilePaused && <button className="terminal-follow-button" title={t('btn.switchTableView')} onClick={() => { switchToTableMode(); setCompletedWhilePaused(false); }}>⊞</button>}
      {!followTerminal && !tableView && running && <button className="terminal-follow-button" title={t('btn.scrollBottom')} onClick={scrollTerminalToBottom}>⌄</button>}
      {running && <button className="mobile-floating-stop" onClick={() => void solve()}>{t('btn.stopSolver')}</button>}
      {/* Intentional feature by Abid: table columns reflect the metric at solve time, not the live metric dropdown. */}
      {tableView ? <div ref={tableContainerRef} className={`terminal metric-${tableMetricRef.current.toLowerCase()} ${showErgo ? "with-ergo" : ""}`} onScroll={handleTableScroll}>
        <div className="terminal-head"><span>{t('table.hash')}</span><b>{t('table.solution')}</b>{tableMetricRef.current === "Angle" && <span>{t('table.angle')}</span>}{tableMetricRef.current !== "Slice" && <span>{t('table.moves')}</span>}<span>{t('table.slices')}</span>{showErgo && <span>{t('table.ergo')}</span>}</div>
        {tableSolutions.map((x, i) => {
          const ergo = displayErgo(x);
          return <div className="solution" key={x.raw} onMouseDown={(event) => {
            if (event.button !== 0 && event.button !== 2) return;
            event.preventDefault();
            if (event.button === 0) { if (contextMenu) setContextMenu(null); return; }
            setContextMenu({ x: event.clientX, y: event.clientY, alg: x.display });
          }} onContextMenu={(event) => event.preventDefault()}><span>{i + 1}</span><code className={abidNotation ? "abid" : ""}>{abidNotation ? abidify(x.alg) : x.alg}</code>{tableMetricRef.current === "Angle" && <span>{x.angle}</span>}{tableMetricRef.current !== "Slice" && <span>{x.moves}</span>}<span>{x.slices}</span>{showErgo && <span>{ergo === undefined ? "…" : ergo.toFixed(1)}</span>}</div>;
        })}
        {tableBusyMessage && <div className="table-busy"><span className="table-busy-spinner" /><span>{tableBusyText}</span></div>}
      </div> : <div ref={terminalTextRef} className="terminal terminal-text" onWheel={(event) => { if (event.deltaY < 0 && running) { followTerminalRef.current = false; setFollowTerminal(false); } }} onScroll={handleTerminalScroll}>
        {!outputLines.length && !solutions.length && <span className="terminal-line terminal-line-empty">{generator ? t('terminal.emptyScramble') : t('terminal.emptySolution')}</span>}
        {terminalNonSolutions.map((line) => <span key={line.key} className="terminal-line terminal-line-status">{line.text || " "}</span>)}
        {terminalSolutions.map((line, index) => <span key={line.key} className={`terminal-line terminal-line-solution ${index % 2 ? "terminal-line-b" : "terminal-line-a"}`}
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
  return (
    <div className={`app ${expanded ? "output-expanded" : ""} ${mobileOptionsOpen ? "mobile-options-open" : ""} ${mobileOutputOpen ? "mobile-output-open" : ""}`} style={zoom === 1 ? undefined : { transform: `scale(${zoom})`, transformOrigin: "top left", width: `${100 / zoom}%`, height: `${100 / zoom}dvh` }}>
      <header>
        <img className="app-icon" src="/icon-web.png" alt="" />
        <div className="brand">
          <b>{t('app.brand')}</b><sub> &nbsp; &nbsp; {t('app.byline')}</sub>
        </div>
        <div className="top-menu-wrap">
          <button className="top-favorites-button" title={t('btn.favorites')} onClick={() => { setFavoritesOpen(true); setFavoritesOpening(true); }}>♥</button>
          <button className="top-menu-button" aria-label={t('btn.openMenu')} aria-expanded={menu} title={tooltips.menu} onMouseDown={(event) => event.preventDefault()} onClick={() => setMenu((value) => !value)}>
            ⋮
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
            ▾
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
      }} onClick={(event) => event.stopPropagation()}>
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
        smartKarn, setSmartKarn, abidNotation, setAbidNotation, ignoreTransforms, setIgnoreTransforms,
        debugOutput, setDebugOutput, zoom, setZoom, disabled: running, hasMaxTurn: maxX || maxY || maxTotal, language: lang,
        setLanguage: (code) => { setLang(code); setLangState(code); },
      }} debugStats={modal === "debug" ? computeDebugStats() : null} />}
      {(favoritesOpen || favoritesClosing) && <div className={"modal-shade" + (favoritesClosing ? " closing" : "")} style={favoritesClosing ? { background: "transparent", pointerEvents: "none" } : {}} onPointerDown={(e) => { favShadeStartRef.current = e.target; }} onPointerUp={(e) => { favShadeEndRef.current = e.target; }} onClick={() => {
        const startOutside = !favShadeStartRef.current || !(favShadeStartRef.current as Element).closest(".favorites-modal");
        const endOutside = !favShadeEndRef.current || !(favShadeEndRef.current as Element).closest(".favorites-modal");
        if (startOutside && endOutside) beginCloseFavorites();
        favShadeStartRef.current = null;
        favShadeEndRef.current = null;
      }}>
        <div ref={favModalRef} className={"modal favorites-modal" + (favoritesClosing ? " closing" : "") + (favoritesOpening ? " opening" : "")} style={favoritesClosing ? { transformOrigin: `${favClosingOriginRef.current.x}% ${favClosingOriginRef.current.y}%` } : undefined} onAnimationEnd={favoritesClosing ? onFavCloseAnimEnd : favoritesOpening ? onFavOpenAnimEnd : undefined} onClick={(event) => event.stopPropagation()}>
          <button className="modal-close" onClick={beginCloseFavorites}>✕</button>
          <h2>{t('favorites.heading')}</h2>
          {!!solutions.length && <button className="favorite-save" onClick={() => {
            const key = currentRunKey();
            setFavorites((old) => ({ ...old, [key]: {
              name: old[key]?.name || `Position ${Object.keys(old).length + 1}`,
              algorithms: Array.from(new Set([...(old[key]?.algorithms || []), ...visibleSolutions.map((solution) => solution.display)])),
            } }));
          }}>{t('btn.saveSolutions')}</button>}
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
                <button title={t('btn.copyAllAlgs')} onClick={() => void navigator.clipboard.writeText(bin.algorithms.map(lineWithoutBracket).join("\n"))}>⧉</button>
                <button title={t('btn.deleteBin')} onClick={() => {
                  const timer = window.setTimeout(() => {
                    setPendingDeletes((old) => { const next = { ...old }; delete next[binDeleteKey]; return next; });
                    setFavorites((old) => { const next = { ...old }; delete next[key]; return next; });
                  }, 5000);
                  setPendingDeletes((old) => ({ ...old, [binDeleteKey]: timer }));
                }}>🗑</button>
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
                    <button className="favorite-alg-copy" title={t('btn.copyAlgSmall')} onClick={() => void navigator.clipboard.writeText(lineWithoutBracket(alg))}>⧉</button>
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
                    }}>✕</button>
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
