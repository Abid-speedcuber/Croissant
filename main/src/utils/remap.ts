const SOLVED = "A1B2C3D45E6F7G8H";

function isPlaceholder(c: string): boolean {
  return "UVWXYZ".indexOf(c) >= 0;
}

// A concrete corner piece is A-H, a concrete edge is 1-8.
function isCornerPiece(p: string): boolean {
  return p >= "A" && p <= "H";
}

function* generatePermutations(arr: string[]): Generator<string[]> {
  if (arr.length <= 1) {
    yield arr.slice();
    return;
  }
  const sorted = arr.slice().sort();
  yield* heapPermutations(sorted, sorted.length);
}

function* heapPermutations(arr: string[], size: number): Generator<string[]> {
  if (size === 1) {
    yield arr.slice();
    return;
  }
  for (let i = 0; i < size; i++) {
    yield* heapPermutations(arr, size - 1);
    if (size % 2 === 1) {
      const tmp = arr[0];
      arr[0] = arr[size - 1];
      arr[size - 1] = tmp;
    } else {
      const tmp = arr[i];
      arr[i] = arr[size - 1];
      arr[size - 1] = tmp;
    }
  }
}

function expandDuplicateTargets(target: string): string[] {
  const counts = new Map<string, number>();
  for (let i = 0; i < 16; i++) {
    const c = target[i];
    counts.set(c, (counts.get(c) || 0) + 1);
  }

  const duplicates: { piece: string; positions: number[] }[] = [];
  for (const [piece, count] of counts) {
    if (count > 1 && !isPlaceholder(piece)) {
      const positions: number[] = [];
      for (let i = 0; i < 16; i++) {
        if (target[i] === piece) positions.push(i);
      }
      duplicates.push({ piece, positions });
    }
  }

  if (duplicates.length === 0) return [target];

  let candidates = [target];

  for (const { piece, positions } of duplicates) {
    const solvedPieces = positions.map((p) => SOLVED[p]);
    const uniqueSolved = [...new Set(solvedPieces)];
    if (uniqueSolved.length === 1) continue;

    const newCandidates: string[] = [];
    for (const existing of candidates) {
      for (const perm of generatePermutations(solvedPieces)) {
        const chars = existing.split("");
        for (let j = 0; j < positions.length; j++) {
          chars[positions[j]] = perm[j];
        }
        newCandidates.push(chars.join(""));
      }
    }
    candidates = [...new Set(newCandidates)];
  }

  return candidates;
}

// Translates a `given` position into the target's coordinate frame.
//
// Rule (confirmed):
//   (a) build a role-map from the target's concrete pieces: a concrete piece P
//       sitting at slot i of the target "plays the role of" SOLVED[i].
//   (b) relabel each concrete piece of the `given` by PIECE IDENTITY using that
//       map, keeping the result in place.
//   (c) a `given` placeholder stays as itself.
//   (d) a `given` concrete piece that is NOT one of the target's concrete
//       pieces (a "foreign" piece) becomes a placeholder: W if it is a corner,
//       Z if it is an edge.
function translatePosition(given: string, target: string): string {
  // build role-map: target concrete piece at slot i -> SOLVED[i]
  const map = new Map<string, string>();
  for (let i = 0; i < 16; i++) {
    const t = target[i];
    if (!isPlaceholder(t)) map.set(t, SOLVED[i]);
  }

  let out = "";
  for (let i = 0; i < given.length; i++) {
    const g = given[i];
    if (isPlaceholder(g)) { out += g; continue; }
    if (map.has(g)) { out += map.get(g)!; continue; }
    // foreign concrete piece: not mentioned by the target
    out += isCornerPiece(g) ? "W" : "Z";
  }
  return out;
}

function processTarget(given: string, target: string): string[] {
  return [translatePosition(given, target)];
}

export function remapPosition(given: string, target: string): string {
  const expandedTargets = expandDuplicateTargets(target);
  return processTarget(given, expandedTargets[0])[0];
}

export function generateRemapCandidates(given: string, target: string, debug?: boolean): string[] {
  const expandedTargets = expandDuplicateTargets(target);
  if (debug) console.debug("[remap] expanded targets:", expandedTargets);

  const allCandidates: string[] = [];
  const seen = new Set<string>();

  for (const expandedTarget of expandedTargets) {
    const candidates = processTarget(given, expandedTarget);
    if (debug) console.debug("[remap] target:", expandedTarget, "candidates:", candidates);
    for (const c of candidates) {
      if (!seen.has(c)) {
        seen.add(c);
        allCandidates.push(c);
      }
    }
  }

  if (debug) console.debug("[remap] all candidates:", allCandidates);

  return allCandidates;
}

// ============================================================================
// Rotation-class grouping (slice metric).
//
// Two inputs that differ only by free U/D turns solve in the same number of
// slices (top and bottom turns cost nothing in slice metric).  So a batch of
// inputs can be grouped by a canonical "rotation class"; solving one member
// yields the slice count for the whole group.
//
// The canonical form depends on whether we are cube-shape restricted (-c):
//   - cubeshape: the input is laid out 8-char top layer + 8-char bottom layer,
//     each layer an exact rotation of the other's.  No extension needed --
//     rotate the first 8 and last 8 characters independently.
//   - non-cubeshape (all 7356 shapes): extend each 8-char layer to a 12-char
//     string (doubling every corner piece) so that a U/D turn becomes a whole
//     cyclic shift of one half, then rotate each 12-char half independently.
// In both cases the middle layer is part of the key (grouping is invalid
// across different middle layers), read from the optional 17th char.
// ============================================================================

// Letters A-H are corners (an extended corner doubles to two chars); digits
// 1-8 are edges (one char).  Placeholder letters U-W are corners, X-Z edges.
function isCornerByIdentity(p: string): boolean {
  return (p >= "A" && p <= "H") || p === "U" || p === "V" || p === "W";
}

function extendLayer(layer8: string): string {
  let out = "";
  for (const ch of layer8) out += isCornerByIdentity(ch) ? ch + ch : ch;
  return out;
}

// Lexicographically smallest cyclic rotation of `s` over all 0..len-1 shifts.
function minRotation(s: string): string {
  let best = s;
  const n = s.length;
  for (let k = 1; k < n; k++) {
    const rotated = s.slice(k) + s.slice(0, k);
    if (rotated < best) best = rotated;
  }
  return best;
}

export function computeRotationClassKey(pos: string, cubeshape: boolean): string {
  const s = pos.toUpperCase();

  let middle = 0;
  let body = s;
  if (s.length === 17) {
    const last = s[16];
    if (last === "-") middle = 1;
    else if (last === "/") middle = -1;
    body = s.slice(0, 16);
  }

  const top8 = body.slice(0, 8);
  const bot8 = body.slice(8, 16);

  let canonicalTop: string;
  let canonicalBot: string;
  if (cubeshape) {
    canonicalTop = minRotation(top8);
    canonicalBot = minRotation(bot8);
  } else {
    canonicalTop = minRotation(extendLayer(top8));
    canonicalBot = minRotation(extendLayer(bot8));
  }

  return `${middle}|${canonicalTop}|${canonicalBot}`;
}
