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
