const SOLVED = "A1B2C3D45E6F7G8H";

const PLACEHOLDER_SETS: Record<string, string> = {
  U: "ABCDU",
  V: "EFGHV",
  W: "ABCDEFGHUVW",
  X: "1234X",
  Y: "5678Y",
  Z: "12345678XYZ",
};

function isPlaceholder(c: string): boolean {
  return "UVWXYZ".indexOf(c) >= 0;
}

function buildSubstitutionMap(target: string, solved: string): Map<string, string> {
  const map = new Map<string, string>();
  for (let i = 0; i < 16; i++) {
    const t = target[i], s = solved[i];
    if (!isPlaceholder(t) && t !== s) {
      map.set(t, s);
    }
  }
  return map;
}

function applySubstitution(given: string, map: Map<string, string>): string {
  let out = "";
  for (let i = 0; i < given.length; i++) {
    out += map.get(given[i]) || given[i];
  }
  return out;
}

function assignPlaceholders(remapped: string, target: string): string {
  let out = "";
  for (let i = 0; i < 16; i++) {
    if (isPlaceholder(target[i])) {
      const ph = target[i];
      const piece = remapped[i];
      const valid = PLACEHOLDER_SETS[ph];
      if (valid && valid.indexOf(piece) >= 0) {
        out += ph;
        continue;
      }
    }
    out += remapped[i];
  }
  return out;
}

function pieceBelongsToPlaceholder(piece: string, placeholder: string): boolean {
  const set = PLACEHOLDER_SETS[placeholder];
  return set ? set.indexOf(piece) >= 0 : false;
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

function processTarget(given: string, target: string): string[] {
  const map = buildSubstitutionMap(target, SOLVED);
  const substituted = applySubstitution(given, map);

  const placeholderPositions: number[] = [];
  for (let i = 0; i < 16; i++) {
    if (isPlaceholder(target[i])) placeholderPositions.push(i);
  }

  if (placeholderPositions.length === 0) {
    return [substituted];
  }

  const groups = new Map<string, { positions: number[]; pieces: string[] }>();
  for (const pos of placeholderPositions) {
    const ph = target[pos];
    const group = groups.get(ph) || { positions: [], pieces: [] };
    group.positions.push(pos);
    group.pieces.push(substituted[pos]);
    groups.set(ph, group);
  }

  const groupKeys = Array.from(groups.keys());
  let hasOverlap = false;
  for (let i = 0; i < groupKeys.length; i++) {
    for (let j = i + 1; j < groupKeys.length; j++) {
      const setA = PLACEHOLDER_SETS[groupKeys[i]];
      const setB = PLACEHOLDER_SETS[groupKeys[j]];
      if (setA && setB) {
        for (const c of setA) {
          if (setB.indexOf(c) >= 0) { hasOverlap = true; break; }
        }
      }
      if (hasOverlap) break;
    }
    if (hasOverlap) break;
  }

  if (!hasOverlap) {
    return [assignPlaceholders(substituted, target)];
  }

  const groupEntries = groupKeys.map((k) => groups.get(k)!);
  const allPositions: number[] = [];
  const allPieces: string[] = [];
  const allTypes: string[] = [];
  for (const entry of groupEntries) {
    for (let j = 0; j < entry.positions.length; j++) {
      allPositions.push(entry.positions[j]);
      allPieces.push(entry.pieces[j]);
      allTypes.push(target[entry.positions[j]]);
    }
  }

  const seen = new Set<string>();
  const candidates: string[] = [];

  for (const perm of generatePermutations(allPieces)) {
    let valid = true;
    for (let j = 0; j < allPositions.length; j++) {
      if (!pieceBelongsToPlaceholder(perm[j], allTypes[j])) {
        valid = false;
        break;
      }
    }
    if (!valid) continue;

    const chars = substituted.split("");
    for (let j = 0; j < allPositions.length; j++) {
      chars[allPositions[j]] = perm[j];
    }
    const candidate = assignPlaceholders(chars.join(""), target);
    if (!seen.has(candidate)) {
      seen.add(candidate);
      candidates.push(candidate);
    }
  }

  if (candidates.length === 0) {
    candidates.push(assignPlaceholders(substituted, target));
  }

  return candidates;
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
    const map = buildSubstitutionMap(expandedTarget, SOLVED);
    if (debug) console.debug("[remap] target:", expandedTarget, "map:", Object.fromEntries(map));
    const substituted = applySubstitution(given, map);
    if (debug) console.debug("[remap] substituted:", substituted);
    const candidates = processTarget(given, expandedTarget);
    if (debug) console.debug("[remap] candidates from target:", candidates);
    for (const c of candidates) {
      if (!seen.has(c)) {
        seen.add(c);
        allCandidates.push(c);
      }
    }
  }

  if (allCandidates.length === 0) {
    const map = buildSubstitutionMap(target, SOLVED);
    allCandidates.push(assignPlaceholders(applySubstitution(given, map), target));
  }

  if (debug) console.debug("[remap] all candidates:", allCandidates);

  return allCandidates;
}
