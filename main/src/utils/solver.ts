import { CubeState, RatingResult, Solution, TauriGlobal } from "./types";

export function twistable(p: number[]) {
  return p[0] !== p[11] && p[5] !== p[6] && p[12] !== p[23] && p[17] !== p[18];
}

export function getLayerR(pos: number[], base: number): number {
  for (let r = 0; r < 3; r++) {
    let ok = true;
    for (let i = 0; i < 12; i++) {
      if ((i % 3 === r) !== (pos[base + i] >= 8)) { ok = false; break; }
    }
    if (ok) return r;
  }
  return -1;
}

export function getParityOdd(pos: number[]): boolean {
  let p = false;
  for (let i = 0; i < 24; i++) {
    for (let j = i; j < 24; j++) {
      if (pos[j] < pos[i]) p = !p;
      if (pos[j] < 8) j++;
    }
    if (pos[i] < 8) i++;
  }
  return p;
}

export function isGoodSquares(state: CubeState): boolean {
  const rTop = getLayerR(state.position, 0);
  if (rTop < 0) return false;
  const rBot = getLayerR(state.position, 12);
  if (rBot < 0) return false;
  if (rTop === 1 || rBot === 1) return false;
  if (state.partial.some((v) => v !== 0)) return true;
  const odd = getParityOdd(state.position);
  return (rTop === rBot) === odd;
}

export function inCubeshape(state: CubeState): boolean {
  const rTop = getLayerR(state.position, 0);
  if (rTop < 0) return false;
  const rBot = getLayerR(state.position, 12);
  if (rBot < 0) return false;
  if (rTop === 1 || rBot === 1) return false;
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

export function doMove(s: CubeState, key: string): CubeState {
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

export function tauri(): TauriGlobal | undefined {
  const nativeWindow = window as Window & { __SQ1_NATIVE__?: TauriGlobal; __TAURI__?: TauriGlobal };
  return nativeWindow.__SQ1_NATIVE__ ?? nativeWindow.__TAURI__;
}

export function validDepths(value: string) { return /^\d+(?:,\d+)*$/.test(value.replace(/\s/g, "")); }

export function solverFlags(options: {
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

export function positionString(state: CubeState) {
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

export function rawPosition(state: CubeState) {
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

export function parsePosition(text: string): CubeState | undefined {
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
  const middle = input.length === 16 ? 0 : input[16] === "-" ? 1 : input[16] === "/" ? -1 : 0;
  return { position: encoded, partial, middle, middlePartial: 0 };
}

export function invertScramble(text: string) {
  return text.trim().split("/").reverse().map((part) => {
    const raw = part.trim().replace(/[()]/g, "");
    const values = raw.split(",").map((x) => Number(x.trim()));
    if (values.length === 2 && values.every(Number.isFinite)) return `${-values[0]},${-values[1]}`;
    if (values.length === 1 && raw && Number.isFinite(values[0])) return String(-values[0]);
    return part;
  }).join("/");
}

export function addCommas(text: string) {
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

export function applyNumericAlgorithm(state: CubeState, text: string): CubeState | undefined {
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

export type OutputMode = "normal" | "karn" | "cskarn" | "abid";

export const OUTPUT_MODES: OutputMode[] = ["normal", "karn", "cskarn", "abid"];

export function nextOutputMode(mode: OutputMode): OutputMode {
  return OUTPUT_MODES[(OUTPUT_MODES.indexOf(mode) + 1) % OUTPUT_MODES.length];
}

/*
 * NOTATION-CONVERSION POLICY — READ BEFORE ADDING A FRONTEND CONVERTER
 *
 * ALL notation conversion (WCA -> karn, WCA -> abid) happens SOLVER-SIDE
 * (see -k1/-k2/-k3 in src-tauri/native/sq1opt.cpp, `printsol`). The solver
 * emits each solution line already carrying the raw WCA alg, the karn text,
 * and (with -k3) the abid text, so the frontend can render any mode from the
 * streamed line with ZERO conversion.
 *
 * This is deliberate: for live streaming, an extra IPC round-trip (or a JS
 * pass) per solution line costs MORE than the solver needs to find solutions.
 * Do NOT "helpfully" add a frontend converter that re-derives karn/abid from
 * the raw alg while solutions are streaming.
 *
 * The functions below are the exception, not the rule. They exist ONLY for
 *  1. `abidify`/`abidSpacing` fallbacks when displaying rows that were solved
 *     WITHOUT -k3 (older data, or a solve run in a different output mode), and
 *  2. `abidify`, which maps digits to the Kompact font's private-use-area
 *     barred-digit glyphs. That is FONT RENDERING (tied to the app's font),
 *     not notation conversion, so it stays in the frontend.
 */

/**
 * Abid's notation: normal (WCA) notation with Abid's negative number
 * representation (barred digits via abidify). The only difference from plain
 * WCA-with-barred-numbers is the slashes: every slash becomes a space except
 *  1. a leading slash  ("/3,0/4,0")
 *  2. a trailing slash ("3,0/4,0/")
 *  3. the slice-start marker inserted by the alg rater (upslice "/",
 *     downslice "\", or neutral "|"). The rater always writes it in place of
 *     the first separator, so hasIndicator tells us whether that first
 *     separator is the rater's marker rather than a regular slash.
 */
export function abidSpacing(text: string, hasIndicator?: boolean) {
  const lb = text.lastIndexOf("[");
  const alg = lb > 0 ? text.slice(0, lb) : text;
  const rest = lb > 0 ? text.slice(lb) : "";
  const markerAt = hasIndicator ? alg.search(/[\/\\|]/) : -1;
  const firstIdx = alg.length - alg.trimStart().length;
  const lastIdx = alg.length - 1 - (alg.length - alg.trimEnd().length);
  let out = "";
  for (let i = 0; i < alg.length; i++) {
    const ch = alg[i];
    if (ch !== "/") { out += ch; continue; }
    const leading = i === firstIdx;
    const trailing = i === lastIdx;
    const isMarker = i === markerAt;
    if (leading || trailing || isMarker) out += ch;
    else out += " ";
  }
  return out + rest;
}

export function abidify(text: string) {
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

export function injectSliceIndicator(line: string, indicator?: string) {
  if (!indicator) return line;
  const separator = line.search(/[\/\\|]/);
  if (separator >= 0) return line.slice(0, separator) + indicator + line.slice(separator + 1);
  const space = line.indexOf(" ");
  return space >= 0 ? line.slice(0, space) + indicator + line.slice(space + 1) : line;
}

export function lineAlg(line: string) {
  const lb = line.lastIndexOf("[");
  return (lb > 0 ? line.slice(0, lb) : line).trim();
}

export function lineWithoutBracket(line: string) {
  return lineAlg(line);
}

export function parseSolutionCounts(line: string) {
  const lb = line.lastIndexOf("["), rb = line.lastIndexOf("]");
  const counts = lb >= 0 && rb > lb ? line.slice(lb + 1, rb).split("|").map((x) => Number(x.trim()) || 0) : [];
  return { slices: counts[0] || 0, moves: counts[1] || 0, angle: counts[2] || 0 };
}

export function ratingScore(rating?: RatingResult) {
  const value = rating?.finalScore ?? rating?.final_score;
  return typeof value === "number" && Number.isFinite(value) ? value : undefined;
}

export function ratingSliceStart(rating?: RatingResult) {
  const value = rating?.sliceStart ?? rating?.slice_start;
  if (typeof value === "number") return value ? String.fromCharCode(value) : undefined;
  return value || undefined;
}

export function solutionErgo(solution: Solution) {
  return solution.ergo;
}

export function medianNormalize(rows: Solution[]) {
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

export function normalizeLine(line: string, normalize: string) {
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
}

export const tooltips = {
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
  karn: "Click to cycle the output notation: Normal (WCA), Karn, CS-aware Karn, or Abid (no-slashes).",
  maxX: "Limit the maximum top-layer turn in either direction (0-6).",
  maxY: "Limit the maximum bottom-layer turn in either direction (0-6).",
  maxTotal: "Limit the maximum combined |top|+|bottom| turn per move pair (1-12).",
  depths: "Comma-separated list of depths to search, e.g. 8,9.",
};
