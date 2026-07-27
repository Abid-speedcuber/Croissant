import { useEffect, useLayoutEffect, useRef, useState } from "react";
import { loadSettings, saveSettings, loadFavorites, saveFavorites } from "./storage";

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
type CubeState = {
  position: number[];
  partial: number[];
  middle: number;
  middlePartial: number;
};
type Modal = "settings" | "how" | "about" | null;
type FavoriteBin = { name: string; algorithms: string[] };
type Solution = {
  raw: string;
  rawDisplay: string;
  karnDisplay: string;
  algRaw: string;
  slices: number;
  moves: number;
  angle: number;
  ergoRaw?: number;
  ergo?: number;
  sliceStart?: string;
};
type OutputLine = { raw: string; karn: string; isSolution: boolean; algRaw?: string };
type RatingResult = {
  finalScore?: number;
  final_score?: number;
  phase1?: number;
  phase2?: number;
  phase3?: number;
  phase4?: number;
  ergoUp?: number;
  ergo_up?: number;
  ergoDown?: number;
  ergo_down?: number;
  sliceCount?: number;
  slice_count?: number;
  movement?: number;
  bonus?: number;
  valid?: boolean;
  sliceStart?: number | string;
  slice_start?: number | string;
};
type TwoGenStatus = { compatibility: number; cornersTwo: boolean; cornersPseudo: boolean };
type DisplaySolution = Solution & { display: string; alg: string };
type DropdownProps = {
  id: string;
  label: string;
  title: string;
  value: string;
  options: string[];
  disabled?: boolean;
  open: boolean;
  setOpen: (id: string | null) => void;
  onChange: (value: string) => void;
};

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
                onMouseDown={(event) => event.preventDefault()}
                onClick={() => {
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

function twistable(p: number[]) {
  return p[0] !== p[11] && p[5] !== p[6] && p[12] !== p[23] && p[17] !== p[18];
}
function inCubeshape(state: CubeState) {
  for (let base = 0; base < 24; base += 12) {
    let layerMatches = false;
    for (let remainder = 0; remainder < 3; remainder++) {
      let match = true;
      for (let i = 0; i < 12; i++) {
        const expectedEdge = i % 3 === remainder, edge = state.position[base + i] >= 8;
        if (expectedEdge !== edge) { match = false; break; }
      }
      if (match) { layerMatches = true; break; }
    }
    if (!layerMatches) return false;
  }
  return true;
}
function turn(
  p: number[],
  q: number[],
  start: number,
  end: number,
  dir: number,
) {
  for (let n = 0; n < 12; n++) {
    const c = dir > 0 ? p[end - 1] : p[start],
      d = dir > 0 ? q[end - 1] : q[start];
    if (dir > 0) {
      for (let i = end - 1; i > start; i--) {
        p[i] = p[i - 1];
        q[i] = q[i - 1];
      }
    } else {
      for (let i = start; i < end - 1; i++) {
        p[i] = p[i + 1];
        q[i] = q[i + 1];
      }
    }
    p[dir > 0 ? start : end - 1] = c;
    q[dir > 0 ? start : end - 1] = d;
    if (twistable(p)) break;
  }
}
function doMove(s: CubeState, key: string): CubeState {
  const p = [...s.position],
    q = [...s.partial];
  if (key === "u") turn(p, q, 0, 12, 1);
  if (key === "up") turn(p, q, 0, 12, -1);
  if (key === "d") turn(p, q, 12, 24, 1);
  if (key === "dp") turn(p, q, 12, 24, -1);
  if (key === "slice" && twistable(p)) {
    for (let i = 6; i < 12; i++) {
      [p[i], p[i + 6]] = [p[i + 6], p[i]];
      [q[i], q[i + 6]] = [q[i + 6], q[i]];
    }
    return { ...s, position: p, partial: q, middle: -s.middle };
  }
  return { ...s, position: p, partial: q };
}

type CubeActions = {
  u: () => void;
  up: () => void;
  d: () => void;
  dp: () => void;
  slice: () => void;
  reset: () => void;
  set: (state: CubeState) => void;
};

type TauriGlobal = {
  core?: { invoke: <T>(command: string, args?: Record<string, unknown>) => Promise<T> };
  event?: { listen: (name: string, handler: (event: { payload: unknown }) => void) => Promise<() => void> };
  Channel?: new <T>() => { onmessage: (message: T) => void };
};
function tauri(): TauriGlobal | undefined {
  const nativeWindow = window as Window & { __SQ1_NATIVE__?: TauriGlobal; __TAURI__?: TauriGlobal };
  return nativeWindow.__SQ1_NATIVE__ ?? nativeWindow.__TAURI__;
}
function validDepths(value: string) { return /^\d+(?:,\d+)*$/.test(value.replace(/\s/g, "")); }
function solverFlags(options: {
  metric: string; all: boolean; suboptimal: number; depths: string; generator: boolean; two: string;
  cubeshape: boolean; ignoreEquator: boolean; angle: string; maxX: boolean; maxXValue: number;
  maxY: boolean; maxYValue: number; maxTotal: boolean; maxTotalValue: number;
}) {
  const flags: string[] = [];
  if (options.metric === "Slice") flags.push("-es");
  if (options.metric === "Angle") flags.push("-ea");
  if (options.all) flags.push(options.suboptimal && !validDepths(options.depths) ? "-a" + options.suboptimal : "-a");
  if (validDepths(options.depths)) flags.push("-d" + options.depths.replace(/\s/g, ""));
  if (options.generator) flags.push("-g");
  if (options.two === "2 Gen") flags.push("-2");
  if (options.two === "Pseudo 2 Gen") flags.push("-p");
  if (options.cubeshape) flags.push("-c");
  if (options.ignoreEquator) flags.push("-m");
  if (options.angle === "Both") flags.push("-nb");
  if (options.angle === "Top") flags.push("-nu");
  if (options.angle === "Bottom") flags.push("-nd");
  if (options.maxX) flags.push("-X" + options.maxXValue);
  if (options.maxY) flags.push("-Y" + options.maxYValue);
  if (options.maxTotal) flags.push("-Z" + options.maxTotalValue);
  return flags;
}
function positionString(state: CubeState) {
  let result = "", nextPartialCorner = -3, nextPartialEdge = 18;
  for (let i = 0; i < 24; i++) {
    const value = state.position[i], partial = state.partial[i];
    let encoded = value;
    if (partial && value < 8) {
      encoded = nextPartialCorner + (partial === 2 ? 2 : value < 4 ? 0 : 1);
      nextPartialCorner -= 3;
    } else if (partial) {
      encoded = nextPartialEdge + (partial === 2 ? 2 : value < 12 ? 0 : 1);
      nextPartialEdge += 3;
    }
    result += encoded >= 0 && encoded <= 15
      ? "ABCDEFGH12345678"[encoded]
      : encoded < 0 ? encoded % 3 === 0 ? "U" : encoded % 3 === -2 ? "V" : "W"
        : encoded % 3 === 0 ? "X" : encoded % 3 === 1 ? "Y" : "Z";
    if (value < 8) i++;
  }
  return result + (state.middle === 1 ? "-" : state.middle === -1 ? "/" : "");
}
function rawPosition(state: CubeState) {
  const result = Array(24).fill(0);
  let nextPartialCorner = -3, nextPartialEdge = 18;
  for (let i = 0; i < 24; i++) {
    const value = state.position[i], partial = state.partial[i], corner = value < 8;
    let encoded = value;
    if (partial && corner) {
      encoded = nextPartialCorner + (partial === 2 ? 2 : value < 4 ? 0 : 1);
      nextPartialCorner -= 3;
    } else if (partial) {
      encoded = nextPartialEdge + (partial === 2 ? 2 : value < 12 ? 0 : 1);
      nextPartialEdge += 3;
    }
    result[i] = encoded;
    if (corner && i + 1 < 24) result[++i] = encoded;
  }
  return result;
}
function parsePosition(text: string): CubeState | undefined {
  const input = text.trim().toUpperCase();
  if (input.length < 15 || input.length > 17) return;
  const encoded: number[] = [], partial: number[] = [], counts = Array(16).fill(0);
  let nextPartialCorner = -3, nextPartialEdge = 18;
  let topPartialCorners = 0, bottomPartialCorners = 0, topPartialEdges = 0, bottomPartialEdges = 0;
  for (let i = 0; i < 16 && encoded.length < 24; i++) {
    const token = input[i];
    let value = "ABCDEFGH".indexOf(token), definition = 0;
    if (value < 0 && token >= "1" && token <= "8") value = Number(token) + 7;
    else if (value < 0 && token === "U") { topPartialCorners++; value = nextPartialCorner; nextPartialCorner -= 3; definition = 1; }
    else if (value < 0 && token === "V") { bottomPartialCorners++; value = nextPartialCorner + 1; nextPartialCorner -= 3; definition = 1; }
    else if (value < 0 && token === "W") { value = nextPartialCorner + 2; nextPartialCorner -= 3; definition = 2; }
    else if (value < 0 && token === "X") { topPartialEdges++; value = nextPartialEdge; nextPartialEdge += 3; definition = 1; }
    else if (value < 0 && token === "Y") { bottomPartialEdges++; value = nextPartialEdge + 1; nextPartialEdge += 3; definition = 1; }
    else if (value < 0 && token === "Z") { value = nextPartialEdge + 2; nextPartialEdge += 3; definition = 2; }
    else if (value < 0) return;
    if (value >= 0 && value <= 15 && ++counts[value] > 1) return;
    const corner = value < 8;
    encoded.push(value); partial.push(definition);
    if (corner) { if (encoded.length >= 24) return; encoded.push(value); partial.push(definition); }
  }
  if (encoded.length !== 24) return;
  const sum = (start: number) => counts.slice(start, start + 4).filter(Boolean).length;
  if (sum(0) + topPartialCorners > 4 || sum(4) + bottomPartialCorners > 4 ||
      sum(8) + topPartialEdges > 4 || sum(12) + bottomPartialEdges > 4) return;
  const pools = {
    topC: [0, 1, 2, 3].filter((x) => !counts[x]), botC: [4, 5, 6, 7].filter((x) => !counts[x]),
    topE: [8, 9, 10, 11].filter((x) => !counts[x]), botE: [12, 13, 14, 15].filter((x) => !counts[x]),
  };
  const take = (pool: number[]) => pool.pop()!;
  for (let i = 0; i < 24; i++) {
    const corner = encoded[i] < 8;
    if (partial[i] === 1) {
      const top = encoded[i] % 3 === 0;
      encoded[i] = take(corner ? top ? pools.topC : pools.botC : top ? pools.topE : pools.botE);
      if (corner) encoded[i + 1] = encoded[i];
    }
    if (corner) i++;
  }
  const freeC = [...pools.botC, ...pools.topC], freeE = [...pools.botE, ...pools.topE];
  for (let i = 0; i < 24; i++) {
    const corner = encoded[i] < 8;
    if (partial[i] === 2) { encoded[i] = take(corner ? freeC : freeE); if (corner) encoded[i + 1] = encoded[i]; }
    if (corner) i++;
  }
  const middle = input.length === 16 ? 0 : input[16] === "-" ? 1 : input[16] === "+" ? -1 : 0;
  return { position: encoded, partial, middle, middlePartial: 0 };
}
function invertScramble(text: string) {
  return text.trim().split("/").reverse().map((part) => {
    const raw = part.trim().replace(/[()]/g, "");
    const values = raw.split(",").map((x) => Number(x.trim()));
    if (values.length === 2 && values.every(Number.isFinite)) return `${-values[0]},${-values[1]}`;
    if (values.length === 1 && raw && Number.isFinite(values[0])) return String(-values[0]);
    return part;
  }).join("/");
}
function addCommas(text: string) {
  return text.replace(/[\\/]/g, " ").trim().split(/\s+/).filter(Boolean).map((token) => {
    const bare = token.replace(/[()]/g, "");
    if (!bare || bare.includes(",") || !/^-?\d+$/.test(bare)) return token;
    if (bare.length === 1 || (bare.length === 2 && bare.startsWith("-"))) return `${bare},0`;
    if (bare.length === 2) return `${bare[0]},${bare[1]}`;
    if (bare.length === 3) return bare.startsWith("-") ? `${bare.slice(0, 2)},${bare[2]}` : `${bare[0]},${bare.slice(1)}`;
    if (bare.length === 4) return `${bare.slice(0, 2)},${bare.slice(2)}`;
    return token;
  }).join(" ");
}
function applyNumericAlgorithm(state: CubeState, text: string): CubeState | undefined {
  const steps: ({ slice: true } | { slice: false; top: number; bottom: number })[] = [];
  for (const [index, piece] of text.replace(/\\/g, "/").split("/").entries()) {
    if (index) steps.push({ slice: true });
    const value = piece.trim();
    if (!value) continue;
    const pair = value.replace(/[()]/g, "").match(/^(-?\d+)(?:\s*,\s*(-?\d+))?$/);
    if (!pair) return;
    steps.push({ slice: false, top: Number(pair[1]), bottom: Number(pair[2] ?? 0) });
  }
  if (!steps.length) return;
  let current = state;
  for (const step of steps) {
    if (step.slice) {
      if (!twistable(current.position)) return;
      current = doMove(current, "slice");
      continue;
    }
    const position = [...current.position], partial = [...current.partial];
    const rotate = (amount: number, start: number, end: number) => {
      const normalized = ((amount % 12) + 12) % 12;
      for (let n = 0; n < normalized; n++) {
        const p = position[end - 1], q = partial[end - 1];
        for (let i = end - 1; i > start; i--) { position[i] = position[i - 1]; partial[i] = partial[i - 1]; }
        position[start] = p; partial[start] = q;
      }
    };
    rotate(step.top, 0, 12); rotate(step.bottom, 12, 24);
    current = { ...current, position, partial };
  }
  return twistable(current.position) ? current : undefined;
}
function abidify(text: string) {
  const normal = (digit: number) => String.fromCodePoint(0xe000 + digit);
  const single = (digit: number) => String.fromCodePoint(0xe006 + digit);
  const right = (digit: number) => String.fromCodePoint(0xe00b + digit);
  const left = (digit: number) => String.fromCodePoint(0xe010 + digit);
  if (text.includes(",")) return text.replace(/(-?\d+),(-?\d+)/g, (_, first, second) => {
    const a = Number(first), b = Number(second);
    const map = (value: number, fn: (n: number) => string) => String(Math.abs(value)).split("").map((x) => fn(Number(x))).join("");
    if (a < 0 && b < 0) return map(a, right) + map(b, left);
    return map(a, a < 0 ? single : normal) + map(b, b < 0 ? single : normal);
  });
  let result = "";
  for (let i = 0; i < text.length;) {
    if (text[i] === "-" && /\d/.test(text[i + 1] || "")) {
      if (text[i + 2] === "-" && /\d/.test(text[i + 3] || "")) {
        result += right(Number(text[i + 1])) + left(Number(text[i + 3])); i += 4;
      } else { result += single(Number(text[i + 1])); i += 2; }
    } else if (/\d/.test(text[i])) { result += normal(Number(text[i++])); }
    else result += text[i++];
  }
  return result;
}
function injectSliceIndicator(line: string, indicator?: string) {
  if (!indicator) return line;
  const separator = line.search(/[\/\\|]/);
  if (separator >= 0) return line.slice(0, separator) + indicator + line.slice(separator + 1);
  const space = line.indexOf(" ");
  return space >= 0 ? line.slice(0, space) + indicator + line.slice(space + 1) : line;
}
function lineAlg(line: string) {
  const lb = line.lastIndexOf("[");
  return (lb > 0 ? line.slice(0, lb) : line).trim();
}
function lineWithoutBracket(line: string) {
  return lineAlg(line);
}
function parseSolutionCounts(line: string) {
  const lb = line.lastIndexOf("["), rb = line.lastIndexOf("]");
  const counts = lb >= 0 && rb > lb ? line.slice(lb + 1, rb).split("|").map((x) => Number(x.trim()) || 0) : [];
  return { slices: counts[0] || 0, moves: counts[1] || 0, angle: counts[2] || 0 };
}
function ratingScore(rating?: RatingResult) {
  const value = rating?.finalScore ?? rating?.final_score;
  return typeof value === "number" && Number.isFinite(value) ? value : undefined;
}
function ratingSliceStart(rating?: RatingResult) {
  const value = rating?.sliceStart ?? rating?.slice_start;
  if (typeof value === "number") return value ? String.fromCharCode(value) : undefined;
  return value || undefined;
}
function solutionErgo(solution: Solution) {
  return solution.ergo;
}
function medianNormalize(rows: Solution[]) {
  const valid = rows.map((row) => row.ergoRaw).filter((score): score is number => Number.isFinite(score)).sort((a, b) => a - b);
  const middle = Math.floor(valid.length / 2);
  const median = !valid.length ? 0 : valid.length % 2 ? valid[middle] : (valid[middle - 1] + valid[middle]) / 2;
  return rows.map((row) => ({ ...row, ergo: row.ergoRaw === undefined ? undefined : row.ergoRaw - median }))
    .sort((a, b) => {
      const aNan = a.ergo === undefined, bNan = b.ergo === undefined;
      if (aNan && bNan) return 0;
      if (aNan) return 1;
      if (bNan) return -1;
      return (b.ergo ?? 0) - (a.ergo ?? 0);
    });
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
function Modal({
  type,
  close,
  settings,
}: {
  type: Exclude<Modal, null>;
  close: () => void;
  settings?: {
    smartKarn: boolean; setSmartKarn: (value: boolean) => void;
    abidNotation: boolean; setAbidNotation: (value: boolean) => void;
    ignoreTransforms: boolean; setIgnoreTransforms: (value: boolean) => void;
    debugOutput: boolean; setDebugOutput: (value: boolean) => void;
    zoom: number; setZoom: (value: number) => void;
    disabled: boolean;
    hasMaxTurn: boolean;
  };
}) {
  const content =
    type === "settings" ? (
      <div className="modal-article">
        <h2>Settings</h2>
        <div className="settings-list">
          <label className="modal-check">
            <input type="checkbox" checked={settings?.smartKarn ?? true} disabled={settings?.disabled} onChange={(e) => settings?.setSmartKarn(e.target.checked)} />
            <span>Use smarter karn</span>
          </label>
          <label className="modal-check">
            <input type="checkbox" checked={settings?.abidNotation ?? false} disabled={settings?.disabled} onChange={(e) => settings?.setAbidNotation(e.target.checked)} />
            <span>Abid's notation</span>
          </label>
          <label className="modal-check">
            <input type="checkbox" checked={(settings?.ignoreTransforms || settings?.hasMaxTurn) ?? false} disabled={settings?.disabled || settings?.hasMaxTurn} onChange={(e) => settings?.setIgnoreTransforms(e.target.checked)} />
            <span>Ignore move equivalences</span>
          </label>
          <label className="modal-check">
            <input type="checkbox" checked={settings?.debugOutput ?? false} disabled={settings?.disabled} onChange={(e) => settings?.setDebugOutput(e.target.checked)} />
            <span>Debug output</span>
          </label>
        </div>
        <div className="settings-slider">
          <span className="settings-slider-label">UI scale</span>
          <input type="range" min="0.5" max="2" step="0.1" value={settings?.zoom ?? 1} disabled={settings?.disabled} onChange={(e) => settings?.setZoom(Number(e.target.value))} />
          <span className="settings-slider-value">{Math.round((settings?.zoom ?? 1) * 100)}%</span>
          {(settings?.zoom ?? 1) !== 1 && <button className="settings-slider-reset" disabled={settings?.disabled} onClick={() => settings?.setZoom(1)}>Reset</button>}
        </div>
      </div>
    ) : type === "about" ? (
      <div className="modal-article">
        <h2>About Croissant</h2>
        <p>
          This program stemmed from the optimal Square-1 solver by <a href="https://www.jaapsch.net/puzzles/" target="_blank" rel="noreferrer">Jaap Scherphuis</a>.
        </p>
        <p>
          v2 was created by Michael Gottlieb (<a href="https://github.com/qqwref" target="_blank" rel="noreferrer">GitHub</a>, <a href="https://www.worldcubeassociation.org/persons/2006GOTT01" target="_blank" rel="noreferrer">WCA</a>), who rewrote the solver with significant improvements and optimisations.
        </p>
        <p>
          Read the old documentations <a href="https://github.com/abid/croissant/blob/main/docs/sq1opt_old.txt" target="_blank" rel="noreferrer">here</a>. Note that it is largely not applicable within v3.
        </p>
        <p>
          This is the official <strong>v3</strong>. New in v3:
        </p>
        <ul>
          <li>Actual graphical UI</li>
          <li>Ability to generate a solution from a specific angle</li>
          <li>Improved karnotation support</li>
          <li>Algorithm ergonomics rater</li>
        </ul>
        <p>
          v3 is created by <a href="https://www.worldcubeassociation.org/persons/2024ASHR02" target="_blank" rel="noreferrer">Abid Ibn Ashraf</a> and <a href="https://www.worldcubeassociation.org/persons/2023MAOS01" target="_blank" rel="noreferrer">Matt Mao</a>.
        </p>
      </div>
    ) : (
      <div className="modal-article how-to-use">
        <h2>How to Use</h2>
        <div className="modal-section-title">Keyboard shortcuts:</div>
        <ul>
          <li><strong>Z</strong> = Undo &nbsp; <strong>Y</strong> = Redo</li>
          <li><strong>Esc</strong> = Reset the cube to solved</li>
          <li><strong>Ctrl + Enter</strong> = Start or stop the solver</li>
          <li><strong>Ctrl + Z</strong> = Undo state change &nbsp; <strong>Ctrl + Y</strong> = Redo state change</li>
          <li><strong>Ctrl + =</strong> = Zoom in &nbsp; <strong>Ctrl + -</strong> = Zoom out</li>
          <li><strong>Ctrl + 0</strong> = Reset zoom level</li>
        </ul>
        <p>
          Click on two pieces to <strong>swap</strong> them. Or use the below shortcuts (identical to cstimer):
        </p>
        <ul>
          <li><strong>J</strong> = U, only by one piece &nbsp; <strong>F</strong> = U′, only by one piece</li>
          <li><strong>S</strong> = D, only by one piece &nbsp; <strong>L</strong> = D′, only by one piece</li>
          <li><strong>I</strong> or <strong>K</strong> = Slice</li>
          <li><strong>H</strong> = U, by two pieces &nbsp; <strong>G</strong> = U′, by two pieces</li>
          <li><strong>W</strong> = D, by two pieces &nbsp; <strong>O</strong> = D′, by two pieces</li>
        </ul>
        <div className="modal-section-title">Scramble / Alg Input</div>
        <p>
          Type some moves and hit <strong>Apply</strong>. Karn will be parsed correctly.
        </p>
        <p>
          Use the mode button (to the left of the input) to switch between three modes:
        </p>
        <ul>
          <li><strong>Scram</strong>: applies moves forward as a scramble</li>
          <li><strong>Alg</strong>: inverts before applying, useful for testing algs</li>
          <li><strong>Pos</strong>: interprets the input as a string of the raw state (e.g. A1B2C4D38E6F7G5H)</li>
        </ul>
        <p>
          For the first two modes, use <strong>Enter</strong> to apply the moves from the current state, or <strong>Shift + Enter</strong> to first reset the cube to solved state before applying.
        </p>
        <div className="modal-section-title">Favorites</div>
        <p>
          Algs can be saved to bins for later reference. Right-click a generated alg to:
        </p>
        <ul>
          <li><strong>⧉ Copy alg</strong> — copies the alg itself</li>
          <li><strong>♥ Add to Favorites Bin</strong> — saves the alg to a bin</li>
        </ul>
        <p>
          Bins are identified by the <strong>configurations</strong> of the solve, so algs from the same setup always land in the same bin regardless of when they were added.
        </p>
        <p>
          Click the <strong>♥</strong> button (visible in the terminal area) to open the Favorites Bin, where you can:
        </p>
        <ul>
          <li>Click a <strong>bin title</strong> to re-apply that configuration and clear the terminal.</li>
          <li>Use <strong>✏</strong> to rename a bin</li>
          <li><strong>⧉</strong> to copy all its algs</li>
          <li><strong>🗑</strong> to delete the bin entirely</li>
          <li>Click <strong>✕</strong> next to any alg to remove it</li>
        </ul>
        <p>
          Favorited algs are stored on your device, unless you delete them.
        </p>
        <div className="modal-section-title">Options</div>
        <p>
          Hover over any option to read its description. Quick reference:
        </p>
        <ul>
          <li><strong>Metric</strong>: how move length is counted — <strong>Slice</strong> (only slices), <strong>Move</strong> (layer turns too), or <strong>Angle</strong>.</li>
          <li><strong>All optimal</strong>: find every shortest solution, not just the first one found.</li>
          <li><strong>+suboptimal</strong>: also return solutions up to N moves longer than optimal.</li>
          <li><strong>Specific depths</strong>: search only the listed move counts (comma-separated, e.g. "8,9").</li>
          <li><strong>Generator alg</strong>: output algs will set up the case from solved instead of solving it.</li>
          <li><strong>2 Gen / Pseudo 2 Gen</strong>: restrict moves to top-layer turns and slices (or a pseudo variant).</li>
          <li><strong>Stay in cubeshape</strong>: restrict to algs that keep the puzzle in cubeshape throughout.</li>
          <li><strong>Karn output</strong>: display solutions in karn instead of WCA notation.</li>
          <li><strong>Lock layer angle on pre-ABF</strong>: constrain the pre-ABF move to ±1 or 0 on either/both layers.</li>
          <li><strong>Normalize ABF</strong>: simplify ABF moves in the output (e.g. 3-1 → 0-1, 43 → 10).</li>
          <li><strong>Max top / bottom / total turns</strong>: cap how large layer turns can be.</li>
        </ul>
        <div className="modal-section-title">Settings</div>
        <ul>
          <li><strong>Use smarter karn</strong>: don't karnify things like T when the alg goes out of CS.</li>
          <li><strong>Abid's notation</strong>: use barred numbers for negatives. This is just a display setting.</li>
          <li><strong>Ignore move equivalences</strong>: generate all possible algs, even with y2 algs.</li>
          <li><strong>Debug output</strong>: outputs internal solver states.</li>
        </ul>
        <div className="modal-section-title">Output</div>
        <p>
          Solutions appear in the terminal as they are found. Once algs are present, several buttons appear in the corner of the terminal area:
        </p>
        <ul>
          <li><strong>⧉</strong> — copy all algs in the terminal to the clipboard.</li>
          <li><strong>♥</strong> — see the Favorites section.</li>
          <li><strong>⊞</strong> — switch between terminal view and table view.</li>
          <li><strong>⤢</strong> — expand the terminal to full screen.</li>
        </ul>
        <p>
          If <strong>Stay in cubeshape</strong> was active, algs will be roughly sorted by their <strong>ergonomics</strong>. The numbers are relative and for reference only.
        </p>
      </div>
    );
  return (
    <div className="modal-shade" onClick={close}>
      <div className="modal" onClick={(e) => e.stopPropagation()}>
        <button className="modal-close" onClick={close}>
          ✕
        </button>
        {content}
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
  const [menu, setMenu] = useState(false),
    [modal, setModal] = useState<Modal>(null),
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
    [cubeShape, setCubeShape] = useState(false),
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
    [favorites, setFavorites] = useState<Record<string, FavoriteBin>>({});
  const [contextMenu, setContextMenu] = useState<{ x: number; y: number; alg: string } | null>(null);
  const [twoGenStatus, setTwoGenStatus] = useState<TwoGenStatus>({ compatibility: 2, cornersTwo: true, cornersPseudo: true });
  const [followTerminal, setFollowTerminal] = useState(true);
  const [completedWhilePaused, setCompletedWhilePaused] = useState(false);
  const [tableBusyMessage, setTableBusyMessage] = useState("");
  const [tableBusyTick, setTableBusyTick] = useState(0);
  const [outputToolsFaded, setOutputToolsFaded] = useState(false);
  const terminalTextRef = useRef<HTMLDivElement>(null);
  const tableContainerRef = useRef<HTMLDivElement>(null);
  const terminalScrollPositionRef = useRef(0);
  const tableScrollPositionRef = useRef(0);
  const firstTableSwitchAfterSolveRef = useRef(true);
  const isSwitchingViewRef = useRef(false);
  const zoomRef = useRef(1);

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
    if (!inCubeshape(next)) setCubeShape(false);
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
    terminalScrollPositionRef.current = node.scrollTop;
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
    if (followTerminal && !isSwitchingViewRef.current) requestAnimationFrame(scrollTerminalToBottom);
  }, [outputLines, statusLines, solutions, tableView, running, followTerminal]);
  useEffect(() => {
    isSwitchingViewRef.current = false;
  }, [tableView]);
  useEffect(() => {
    if (!tableBusyMessage) return;
    const id = window.setInterval(() => setTableBusyTick((value) => value + 1), 900);
    return () => window.clearInterval(id);
  }, [tableBusyMessage]);
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
    ignoreHistory.current = true; cubeActions.current?.set(next); ignoreHistory.current = false;
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
    setCubeShape(flags.includes("-c"));
    if (flags.includes("-m") !== ignoreMiddle) toggleIgnoreMiddle(flags.includes("-m"));
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
    setFavoritesOpen(false);
  };
  const restore = (position: string) => {
    const next = parsePosition(position);
    if (!next) return;
    ignoreHistory.current = true;
    cubeActions.current?.set(next);
    ignoreHistory.current = false;
  };
  const doUndo = () => {
    if (!undo.length || running) return;
    const previous = undo[undo.length - 1];
    setRedo((items) => [...items, positionString(stateRef.current)].slice(-64));
    restore(previous);
    setUndo(undo.slice(0, -1));
  };
  const doRedo = () => {
    if (!redo.length || running) return;
    const next = redo[redo.length - 1];
    setUndo((items) => [...items, positionString(stateRef.current)].slice(-64));
    restore(next);
    setRedo(redo.slice(0, -1));
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
      setInputError(mode === "POSITION" ? "Invalid position string." : "Invalid algorithm or blocked slice.");
      return;
    }
    cubeActions.current?.set(next);
    setCubeState(next);
    setInputError("");
  };
  const normalizeLine = (line: string) => {
    if (normalize === "None") return line;
    const lb = line.lastIndexOf("["), alg = (lb > 0 ? line.slice(0, lb) : line).trim(), bracket = lb > 0 ? "  " + line.slice(lb).trim() : "";
    const norm = (block: string) => block.replace(/(-?\d)(,?)(-?\d)/, (_, a, comma, b) => {
      const n = (v: string) => { const x = ((Number(v) % 3) + 3) % 3; return x === 2 ? -1 : x; };
      return `${n(a)}${comma}${n(b)}`;
    });
    const separators = [...alg.matchAll(/[\/\\|\s]/g)];
    if (!separators.length) { const one = norm(alg); return one === "0,0" || one === "00" ? bracket.trimStart() : one + bracket; }
    const firstAt = separators[0].index!, lastAt = separators[separators.length - 1].index!;
    let first = alg.slice(0, firstAt), middle = alg.slice(firstAt, lastAt + 1), last = alg.slice(lastAt + 1);
    if (normalize === "Both" || normalize === "PreABF") first = norm(first);
    if (normalize === "Both" || normalize === "PostABF") last = norm(last);
    if (first === "0,0" || first === "00") { first = ""; middle = middle.replace(/^\s/, ""); }
    if (last === "0,0" || last === "00") { last = ""; middle = middle.replace(/\s$/, ""); }
    return first + middle + last + bracket;
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
  };
  const setSolutionRows = (rows: Solution[]) => {
    solutionsRef.current = rows;
    scheduleSolutionFlush();
  };
  const receiveSolverLine = async (line: string, startPosition: string, runId: number) => {
    // Early exit: don't process lines if stop was requested
    if (stopped.current) return;
    if (runId !== solveRunId.current) return;
    const lb = line.lastIndexOf("["), rb = line.lastIndexOf("]");
    if (lb < 0 || rb < 0) {
      if (debugOutput || !seenRaw.current.size) addOutputLine({ raw: line, karn: line, isSolution: false });
      return;
    }
    const rawAlg = line.slice(0, lb).trim();
    if (seenRaw.current.has(rawAlg)) return;
    seenRaw.current.add(rawAlg);
    let rating: RatingResult | undefined, sliceStart: string | undefined;
    if (lastSolveCubeShape.current && tauri()?.core?.invoke) {
      try {
        rating = await tauri()!.core!.invoke<RatingResult>("rate_algorithm", { algorithm: rawAlg, initialTopA: /^[1-8XYZ]/i.test(startPosition) });
        if (runId !== solveRunId.current) return;
        if (rating.valid) sliceStart = ratingSliceStart(rating);
      } catch { /* an unrated row remains available */ }
    }
    // Always apply karnify, even for late solutions (FIXED: removed stopped.current check)
    const { rawDisplay, karnDisplay } = await buildDisplayPair(line, startPosition, sliceStart);
    if (runId !== solveRunId.current) return;
    const displayAlg = lineAlg(normalizeLine(karn ? karnDisplay : rawDisplay));
    if (seenDisplay.current.has(displayAlg)) return;
    seenDisplay.current.add(displayAlg);
    if (seenRaw.current.size === 1) {
      firstSolutionAt.current = performance.now();
      if (!debugOutput) replaceOutputLines(outputLinesRef.current.filter((entry) => entry.isSolution));
    }
    const counts = parseSolutionCounts(line);
    const row: Solution = { raw: line, rawDisplay, karnDisplay, algRaw: rawAlg, ...counts, ergoRaw: rating?.valid ? ratingScore(rating) : undefined, sliceStart };
    addSolution(row);
    addOutputLine({ raw: rawDisplay, karn: karnDisplay, isSolution: true, algRaw: rawAlg });
  };
  const solve = async () => {
    const native = tauri();
    if (!native?.core?.invoke) {
      const fallback = { raw: "Native solving is available from the Tauri desktop app, not the browser preview.", karn: "Native solving is available from the Tauri desktop app, not the browser preview.", isSolution: false };
      outputLinesRef.current = [fallback];
      setOutputLines([fallback]);
      openMobileOutput();
      return;
    }
    if (runningRef.current) {
      stopped.current = true;
      addOutputLine({ raw: "Stop requested. The integrated solver will stop when the current solve returns.", karn: "Stop requested. The integrated solver will stop when the current solve returns.", isSolution: false });
      setStatusLines((lines) => [...lines, "Stop requested. Ready for the next solve."].slice(-8));
      setRunningState(false);
      void native.core.invoke("stop_solver").catch(() => undefined);
      return;
    }
    if ((two === "2 Gen" && (twoGenStatus.compatibility < 2 || (cubeShape && !twoGenStatus.cornersTwo))) ||
        (two === "Pseudo 2 Gen" && (twoGenStatus.compatibility < 1 || (cubeShape && !twoGenStatus.cornersPseudo)))) return;
    openMobileOutput();
    const flags = solverFlags({ metric, all, suboptimal, depths, generator, two, cubeshape: cubeShape, ignoreEquator: ignoreMiddle, angle, maxX, maxXValue, maxY, maxYValue, maxTotal, maxTotalValue });
    if (ignoreTransforms) flags.push("-x");
    stopped.current = false;
    solutionsRef.current = [];
    outputLinesRef.current = [];
    seenRaw.current.clear();
    seenDisplay.current.clear();
    lineQueue.current = Promise.resolve();
    firstSolutionAt.current = 0;
    followTerminalRef.current = true;
    lastSolveCubeShape.current = cubeShape;
    firstTableSwitchAfterSolveRef.current = true;
    terminalScrollPositionRef.current = 0;
    tableScrollPositionRef.current = 0;
    setRunCubeShape(cubeShape);
    setOutputLines([{ raw: "Solving…", karn: "Solving…", isSolution: false }]);
    outputLinesRef.current = [{ raw: "Solving…", karn: "Solving…", isSolution: false }];
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
      if (!native.Channel) throw new Error("The native solver channel is unavailable.");
      const onLine = new native.Channel<string>();
      onLine.onmessage = async (line) => {
        if (runId !== solveRunId.current) return;
        lineQueue.current = lineQueue.current.then(() => receiveSolverLine(line, start, runId));
      };
      const result = await native.core.invoke<{ code: number | null; stdout: string; stderr: string }>("solve", { position: start, flags, onLine });
      if (runId !== solveRunId.current) return;
      const shouldAutoTable = followTerminalRef.current;
      if (shouldAutoTable) {
        switchToTableMode();
        setTableBusyMessage("Resolving latest solutions");
      }
      await lineQueue.current;
      if (shouldAutoTable) setTableBusyMessage("Rating algs");
      for (const line of `${result.stderr || ""}`.split(/\r?\n/).filter(Boolean))
        await receiveSolverLine(line, start, runId);
      if (runId !== solveRunId.current) return;
      if (lastSolveCubeShape.current) {
        if (shouldAutoTable && solutionsRef.current.length) setTableBusyMessage("Normalizing rating");
        setSolutionRows(medianNormalize(solutionsRef.current));
      }
      if (shouldAutoTable) setTableBusyMessage("Building the table");
      flushSolutionState();
      const count = solutionsRef.current.length;
      const status = `${stopped.current ? "Stopped" : result.code === 0 ? "Done" : "Error"} — ${count} solution${count === 1 ? "" : "s"} found in ${((performance.now() - startedAt) / 1000).toFixed(2)}s.`;
      setStatusLines(lastSolveCubeShape.current && count ? [`Ranked ${count} algs by ergonomics.`] : [status]);
      if (shouldAutoTable) {
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
      setStatusLines((lines) => [...lines, "ERROR: " + String(error)].slice(-8));
    } finally {
      if (runId === solveRunId.current) setRunningState(false);
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
      if (typeof value.cubeShape === "boolean") setCubeShape(value.cubeShape);
      if (typeof value.ignoreMiddle === "boolean") {
        setIgnoreMiddle(value.ignoreMiddle);
        if (value.ignoreMiddle) queueMicrotask(() => toggleIgnoreMiddle(true));
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
      metric, two, angle, all, suboptimal, depths, generator, cubeShape, ignoreMiddle,
      maxX, maxXValue, maxY, maxYValue, maxTotal, maxTotalValue, zoom,
    });
  }, [smartKarn, abidNotation, ignoreTransforms, debugOutput, karn, normalize, mode, metric, two, angle, all, suboptimal, depths, generator, cubeShape, ignoreMiddle, maxX, maxXValue, maxY, maxYValue, maxTotal, maxTotalValue, zoom]);
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
      .catch(() => undefined);
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
  const twoGenBlocked = (two === "2 Gen" && (twoGenStatus.compatibility < 2 || (cubeShape && !twoGenStatus.cornersTwo))) ||
    (two === "Pseudo 2 Gen" && (twoGenStatus.compatibility < 1 || (cubeShape && !twoGenStatus.cornersPseudo)));
  const specificDepthsActive = depths.trim().length > 0;
  const commandFlags = solverFlags({ metric, all, suboptimal, depths, generator, two, cubeshape: cubeShape, ignoreEquator: ignoreMiddle, angle, maxX, maxXValue, maxY, maxYValue, maxTotal, maxTotalValue });
  if (ignoreTransforms) commandFlags.push("-x");
  const commandPreview = `croissant ${commandFlags.join(" ")} ${positionString(cubeState)}`;
  const showErgo = runCubeShape;
  const displaySolution = (solution: Solution): DisplaySolution => {
    const display = normalizeLine(karn ? solution.karnDisplay : solution.rawDisplay);
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
    setStatusLines((old) => [...old, "Terminal copied to clipboard!"].slice(-8));
  };
  const renderSolutionText = (text: string) => {
    if (!abidNotation) return text;
    const lb = text.lastIndexOf("[");
    if (lb <= 0) return <span className="abid-inline">{abidify(text)}</span>;
    return <><span className="abid-inline">{abidify(text.slice(0, lb).trim())}</span>{"  " + text.slice(lb).trim()}</>;
  };
  const tableBusyMessages = [
    tableBusyMessage,
    "Resolving latest solutions",
    "Rating algs",
    "Normalizing rating",
    "Building the table",
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
  const tooltips = {
    menu: "Menu",
    inputMode: "Switch input mode: Scramble, Alg, or Position.",
    modeMenu: "Choose input mode.",
    apply: "Apply the input. Shift+Enter first resets the cube to solved.",
    reset: "Reset  [Esc]",
    metric: "Choose how move length is counted: Slice, Move, or Angle metric.",
    twoGen: "Restrict solving to 2 Gen or Pseudo 2 Gen move sets.",
    angle: "Lock the pre-ABF angle move to +/-1 or 0.",
    normalize: "Control which ABF moves are normalized in the output.",
    all: "Find all optimal solutions, or optimal plus extra depths.",
    suboptimal: "Extra moves beyond optimal to also find.",
    generator: "Output algs that set up the case from solved instead of solving it.",
    cubeshape: "Only generate algs that keep the puzzle in cubeshape throughout.",
    ignoreEquator: "Ignore equator states. Equivalent to clicking the middle bar until it is gray.",
    karn: "Display solutions in karnotation instead of WCA notation.",
    maxX: "Limit the maximum top-layer turn in either direction (0-6).",
    maxY: "Limit the maximum bottom-layer turn in either direction (0-6).",
    maxTotal: "Limit the maximum combined |top|+|bottom| turn per move pair (1-12).",
    depths: "Comma-separated list of depths to search, e.g. 8,9.",
  };
  const renderOptionsPanel = () => (
    <div className="options-panel">
      <div className="mobile-modal-head">
        <b>Options</b>
        <button aria-label="Close options" onClick={() => setMobileOptionsOpen(false)}>✕</button>
      </div>
      <h2>Options</h2>
      <div className="select-grid">
        <OptionDropdown id="metric" label="Metric" title={tooltips.metric} value={metric} options={["Slice", "Move", "Angle"]} disabled={running} open={openDropdown === "metric"} setOpen={setOpenDropdown} onChange={setMetric} />
        <OptionDropdown id="two" label="2 Gen" title={tooltips.twoGen} value={two} options={["None", "Pseudo 2 Gen", "2 Gen"]} disabled={running} open={openDropdown === "two"} setOpen={setOpenDropdown} onChange={setTwo} />
        <OptionDropdown id="angle" label="Lock layer angle on preabf" title={tooltips.angle} value={angle} options={["None", "Both", "Top", "Bottom"]} disabled={running} open={openDropdown === "angle"} setOpen={setOpenDropdown} onChange={setAngle} />
        <OptionDropdown id="normalize" label="Normalize ABF" title={tooltips.normalize} value={normalize} options={["None", "Both", "PreABF", "PostABF"]} disabled={running} open={openDropdown === "normalize"} setOpen={setOpenDropdown} onChange={setNormalize} />
      </div>
      <div className="check-grid">
        <label className="inline-all-optimal" title={tooltips.all}>
          <input type="checkbox" checked={all} disabled={running} onChange={(e) => setAll(e.target.checked)} />
          <span>Generate All Solutions:</span>
          <span className="all-optimal-label">{suboptimal && !specificDepthsActive ? `Optimal+${suboptimal}` : "Optimal"}</span>
          {!specificDepthsActive && <span className="stepper-group">
            <button type="button" title={tooltips.suboptimal} disabled={running || !all} onClick={() => setSuboptimal((value) => Math.max(0, value - 1))}>−</button>
            <button type="button" title={tooltips.suboptimal} disabled={running || !all} onClick={() => setSuboptimal((value) => value + 1)}>+</button>
          </span>}
        </label>
        <label title={tooltips.generator}><input type="checkbox" checked={generator} disabled={running} onChange={(e) => setGenerator(e.target.checked)} /> Generator alg</label>
        <label title={tooltips.cubeshape}><input type="checkbox" checked={cubeShape} disabled={running || !inCubeshape(cubeState)} onChange={(e) => setCubeShape(e.target.checked)} /> Stay in cubeshape</label>
        <label title={tooltips.ignoreEquator}><input type="checkbox" checked={ignoreMiddle} disabled={running} onChange={(e) => toggleIgnoreMiddle(e.target.checked)} /> Ignore equator</label>
        <label title={tooltips.karn}><input type="checkbox" checked={karn} disabled={running} onChange={(e) => setKarn(e.target.checked)} /> Karn output</label>
      </div>
      <div className="limit-grid">
        <label title={tooltips.maxX}>Max top turn:
          <div className="number-input-wrap">
            <input type="number" min="0" max="6" value={maxX ? maxXValue : ""} placeholder="6" disabled={running} onChange={(e) => updateOptionalLimit(e.target.value, 0, 6, setMaxX, setMaxXValue)} />
            <div className="number-stepper">
              <button type="button" title={tooltips.maxX} disabled={running} onClick={() => { setMaxX(true); setMaxXValue((value) => Math.min(6, (maxX ? value : 0) + 1)); }}>▲</button>
              <button type="button" title={tooltips.maxX} disabled={running} onClick={() => { setMaxX(true); setMaxXValue((value) => Math.max(0, (maxX ? value : 1) - 1)); }}>▼</button>
            </div>
          </div>
        </label>
        <label title={tooltips.maxY}>Max bottom turn:
          <div className="number-input-wrap">
            <input type="number" min="0" max="6" value={maxY ? maxYValue : ""} placeholder="6" disabled={running} onChange={(e) => updateOptionalLimit(e.target.value, 0, 6, setMaxY, setMaxYValue)} />
            <div className="number-stepper">
              <button type="button" title={tooltips.maxY} disabled={running} onClick={() => { setMaxY(true); setMaxYValue((value) => Math.min(6, (maxY ? value : 0) + 1)); }}>▲</button>
              <button type="button" title={tooltips.maxY} disabled={running} onClick={() => { setMaxY(true); setMaxYValue((value) => Math.max(0, (maxY ? value : 1) - 1)); }}>▼</button>
            </div>
          </div>
        </label>
        <label title={tooltips.maxTotal}>Max total turn:
          <div className="number-input-wrap">
            <input type="number" min="1" max="12" value={maxTotal ? maxTotalValue : ""} placeholder="12" disabled={running} onChange={(e) => updateOptionalLimit(e.target.value, 1, 12, setMaxTotal, setMaxTotalValue)} />
            <div className="number-stepper">
              <button type="button" title={tooltips.maxTotal} disabled={running} onClick={() => { setMaxTotal(true); setMaxTotalValue((value) => Math.min(12, (maxTotal ? value : 0) + 1)); }}>▲</button>
              <button type="button" title={tooltips.maxTotal} disabled={running} onClick={() => { setMaxTotal(true); setMaxTotalValue((value) => Math.max(1, (maxTotal ? value : 2) - 1)); }}>▼</button>
            </div>
          </div>
        </label>
        <label title={tooltips.depths}>Specific depths:<input type="text" value={depths} disabled={running} onChange={(e) => /^\s*\d*(?:\s*,\s*\d*)*\s*$/.test(e.target.value) && setDepths(e.target.value)} placeholder="e.g. 8,9" /></label>
      </div>
    </div>
  );
  const renderOutputShell = () => (
    <div className={`terminal-shell ${outputToolsFaded ? "tools-faded" : ""}`} onMouseMove={markOutputToolsActive} onMouseLeave={() => setOutputToolsFaded(true)}>
      <div className="output-tools">
        <button title="Copy all algs in terminal" disabled={!solutions.length} onClick={copyTerminalText}>⧉</button>
        <button title="Open the favorites bin" onClick={() => setFavoritesOpen(true)}>♥</button>
        <button title={tableView ? "Switch to terminal view" : "Switch to table view"} onClick={() => tableView ? switchToTerminalMode() : switchToTableMode()}>{tableView ? "▤" : "⊞"}</button>
        <button className="mobile-output-close" title="Close output" aria-label="Close output" onClick={() => setMobileOutputOpen(false)}>×</button>
        <button className="expand-output" title={expanded ? "Shrink terminal" : "Expand terminal"} onClick={() => setExpanded((v) => !v)}>{expanded ? "–" : "⤢"}</button>
      </div>
      {!followTerminal && !tableView && completedWhilePaused && <button className="terminal-follow-button" title="Switch to table view" onClick={() => { switchToTableMode(); setCompletedWhilePaused(false); }}>⊞</button>}
      {!followTerminal && !tableView && running && <button className="terminal-follow-button" title="Scroll to bottom and resume auto-scroll" onClick={scrollTerminalToBottom}>⌄</button>}
      {running && <button className="mobile-floating-stop" onClick={() => void solve()}>Stop</button>}
      {tableView ? <div ref={tableContainerRef} className={`terminal metric-${metric.toLowerCase()} ${showErgo ? "with-ergo" : ""}`} onScroll={handleTableScroll}>
        <div className="terminal-head"><span>#</span><b>Solution</b>{metric === "Angle" && <span>Angle</span>}{metric !== "Slice" && <span>Moves</span>}<span>Slices</span>{showErgo && <span>Ergo</span>}</div>
        {tableSolutions.map((x, i) => {
          const ergo = displayErgo(x);
          return <div className="solution" key={x.raw} onMouseDown={(event) => { if (event.button !== 0 && event.button !== 2) return; event.preventDefault(); setContextMenu({ x: event.clientX, y: event.clientY, alg: x.display }); }} onContextMenu={(event) => event.preventDefault()}><span>{i + 1}</span><code className={abidNotation ? "abid" : ""}>{abidNotation ? abidify(x.alg) : x.alg}</code>{metric === "Angle" && <span>{x.angle}</span>}{metric !== "Slice" && <span>{x.moves}</span>}<span>{x.slices}</span>{showErgo && <span>{ergo === undefined ? "…" : ergo.toFixed(1)}</span>}</div>;
        })}
        {tableBusyMessage && <div className="table-busy"><span className="table-busy-spinner" /><span>{tableBusyText}</span></div>}
      </div> : <div ref={terminalTextRef} className="terminal terminal-text" onWheel={(event) => { if (event.deltaY < 0 && running) { followTerminalRef.current = false; setFollowTerminal(false); } }} onScroll={handleTerminalScroll}>
        {!outputLines.length && !solutions.length && <span className="terminal-line terminal-line-empty">solution will be displayed here...</span>}
        {terminalNonSolutions.map((line) => <span key={line.key} className="terminal-line terminal-line-status">{line.text || " "}</span>)}
        {terminalSolutions.map((line, index) => <span key={line.key} className={`terminal-line terminal-line-solution ${index % 2 ? "terminal-line-b" : "terminal-line-a"}`} onContextMenu={(event) => { event.preventDefault(); setContextMenu({ x: event.clientX, y: event.clientY, alg: line.solution.display }); }}>{renderSolutionText(line.text)}</span>)}
        {statusLines.map((line, index) => <span key={`status-${index}-${line}`} className="terminal-line terminal-line-final">{line}</span>)}
      </div>}
    </div>
  );
  return (
    <div className={`app ${expanded ? "output-expanded" : ""} ${mobileOptionsOpen ? "mobile-options-open" : ""} ${mobileOutputOpen ? "mobile-output-open" : ""}`} style={zoom === 1 ? undefined : { transform: `scale(${zoom})`, transformOrigin: "top left", width: `${100 / zoom}%`, height: `${100 / zoom}dvh` }}>
      <header>
        <img className="app-icon" src="/icon-web.png" alt="" />
        <div className="brand">
          <b>CROISSANT</b><sub> &nbsp; &nbsp; by Abid and Matt</sub>
        </div>
        <div className="top-menu-wrap">
          <button className="top-menu-button" aria-label="Open menu" aria-expanded={menu} title={tooltips.menu} onMouseDown={(event) => event.preventDefault()} onClick={() => setMenu((value) => !value)}>
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
                Settings
              </button>
              <button
                onClick={() => {
                  setModal("how");
                  setMenu(false);
                }}
              >
                How to Use
              </button>
              <button
                onClick={() => {
                  setModal("about");
                  setMenu(false);
                }}
              >
                About
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
            aria-label="Choose input mode"
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
                Scramble
              </button>
              <button
                className={mode === "ALG" ? "selected" : ""}
                onMouseDown={(event) => event.preventDefault()}
                onClick={() => chooseMode("ALG")}
              >
                Alg
              </button>
              <button
                className={mode === "POSITION" ? "selected" : ""}
                onMouseDown={(event) => event.preventDefault()}
                onClick={() => chooseMode("POSITION")}
              >
                Position
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
                ? "ABCDEFGH12345678-"
                : "1,0 / 3,3 / 0,-3 / ...  (supports karn)"
            }
          />
          <button className="apply" title={tooltips.apply} onClick={() => void apply()}>
            Apply
          </button>
          {inputError && <span className="input-error">{inputError}</span>}
        </div>
      </div>
      <div className="main-grid">
        <aside className="cube-column">
          <Cube
            actionsRef={cubeActions}
            onChange={onCubeChange}
            onOptions={() => setMobileOptionsOpen(true)}
          />
          <div className="moves">
            <button title="Turn top layer counterclockwise." onClick={() => cubeActions.current?.up()}>U′</button>
            <button
              className="slice"
              title="Slice  [I/K]"
              onClick={() => cubeActions.current?.slice()}
            >
              Slice [I/K]
            </button>
            <button title="Turn top layer clockwise." onClick={() => cubeActions.current?.u()}>U</button>
            <button title="Turn bottom layer clockwise." onClick={() => cubeActions.current?.d()}>D</button>
            <button title="Turn bottom layer counterclockwise." onClick={() => cubeActions.current?.dp()}>D′</button>
          </div>
          <div className="undo">
            <button title="Undo  [Ctrl+Z]" disabled={!undo.length || running} onClick={doUndo}>Undo (Ctrl+Z)</button>
            <button title="Redo  [Ctrl+Y]" disabled={!redo.length || running} onClick={doRedo}>Redo (Ctrl+Y)</button>
          </div>
          <button className={`solve ${running ? "is-running" : ""}`} disabled={!running && twoGenBlocked} title={running ? "Stop the current solve and make the UI ready for another solve." : twoGenBlocked ? "This position is not compatible with the selected 2-gen constraints." : commandPreview} onClick={() => void solve()}>{running ? "■ Stop [Ctrl+Enter]" : "▶ Solve [Ctrl+Enter]"}</button>
          <button className="mobile-open-output" onClick={openMobileOutput}>Open Output Terminal / Table</button>
        </aside>
        {renderOptionsPanel()}
        {renderOutputShell()}
      </div>
      {contextMenu && <div className="solution-context" style={{ left: contextMenu.x, top: contextMenu.y }} onClick={(event) => event.stopPropagation()}>
        <button onClick={() => { void navigator.clipboard.writeText(lineWithoutBracket(contextMenu.alg)); setContextMenu(null); }}>⧉  Copy alg</button>
        <button onClick={() => {
          const key = currentRunKey();
          setFavorites((old) => ({ ...old, [key]: {
            name: old[key]?.name || `Position ${Object.keys(old).length + 1}`,
            algorithms: Array.from(new Set([...(old[key]?.algorithms || []), contextMenu.alg])),
          } }));
          setContextMenu(null);
          setFavoritesOpen(true);
        }}>♥  Add to Favorites Bin</button>
      </div>}
      {modal && <Modal type={modal} close={() => setModal(null)} settings={{
        smartKarn, setSmartKarn, abidNotation, setAbidNotation, ignoreTransforms, setIgnoreTransforms,
        debugOutput, setDebugOutput, zoom, setZoom, disabled: running, hasMaxTurn: maxX || maxY || maxTotal,
      }} />}
      {favoritesOpen && <div className="modal-shade" onClick={() => setFavoritesOpen(false)}>
        <div className="modal favorites-modal" onClick={(event) => event.stopPropagation()}>
          <button className="modal-close" onClick={() => setFavoritesOpen(false)}>✕</button>
          <h2>Favorites</h2>
          {!!solutions.length && <button className="favorite-save" onClick={() => {
            const key = currentRunKey();
            setFavorites((old) => ({ ...old, [key]: {
              name: old[key]?.name || `Position ${Object.keys(old).length + 1}`,
              algorithms: Array.from(new Set([...(old[key]?.algorithms || []), ...visibleSolutions.map((solution) => solution.display)])),
            } }));
          }}>Save current solutions</button>}
          {!Object.keys(favorites).length && <p>No saved solution bins yet.</p>}
          {Object.entries(favorites).map(([key, bin]) => <section className="favorite-bin" key={key}>
            <input value={bin.name} aria-label="Favorite name" onChange={(event) => setFavorites((old) => ({ ...old, [key]: { ...old[key], name: event.target.value } }))} />
            <div className="favorite-actions"><button onClick={() => applyRunConfig(key)}>Apply setup</button><button onClick={() => void navigator.clipboard.writeText(bin.algorithms.join("\n"))}>Copy</button><button onClick={() => setFavorites((old) => { const next = { ...old }; delete next[key]; return next; })}>Delete</button></div>
            <pre>{bin.algorithms.join("\n")}</pre>
          </section>)}
        </div>
      </div>}
    </div>
  );
}
