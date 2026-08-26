const SOLVED = "A1B2C3D45E6F7G8H";

const PLACEHOLDER_SETS: Record<string, string> = {
  U: "ABCD",
  V: "EFGH",
  W: "ABCDEFGH",
  X: "1234",
  Y: "5678",
  Z: "12345678",
};

const PLACEHOLDER_TYPES: Record<string, string> = {};
for (const [ph, set] of Object.entries(PLACEHOLDER_SETS)) {
  for (const c of set) {
    PLACEHOLDER_TYPES[c] = (PLACEHOLDER_TYPES[c] || "") + ph;
  }
}

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

export function remapPosition(given: string, target: string): string {
  const map = buildSubstitutionMap(target, SOLVED);
  const substituted = applySubstitution(given, map);
  return assignPlaceholders(substituted, target);
}

export function generateRemapCandidates(given: string, target: string): string[] {
  const map = buildSubstitutionMap(target, SOLVED);
  const substituted = applySubstitution(given, map);

  const placeholderPositions: number[] = [];
  for (let i = 0; i < 16; i++) {
    if (isPlaceholder(target[i])) placeholderPositions.push(i);
  }
  if (placeholderPositions.length === 0) return [substituted];

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
    const result = assignPlaceholders(substituted, target);
    return [result];
  }

  const groupEntries = groupKeys.map((k) => groups.get(k)!);

  const allPositions: number[] = [];
  const allPieces: string[] = [];
  const allTypes: string[] = [];
  for (const entry of groupEntries) {
    for (let i = 0; i < entry.positions.length; i++) {
      allPositions.push(entry.positions[i]);
      allPieces.push(entry.pieces[i]);
      allTypes.push(target[entry.positions[i]]);
    }
  }

  const seen = new Set<string>();
  const candidates: string[] = [];

  for (const perm of generatePermutations(allPieces)) {

    let valid = true;
    for (let i = 0; i < allPositions.length; i++) {
      if (!pieceBelongsToPlaceholder(perm[i], allTypes[i])) {
        valid = false;
        break;
      }
    }
    if (!valid) continue;

    const chars = substituted.split("");
    for (let i = 0; i < allPositions.length; i++) {
      chars[allPositions[i]] = perm[i];
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
