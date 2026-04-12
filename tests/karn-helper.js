// sq1_tester.js — standalone, no deps
// Usage: node sq1_tester.js "<scramble>"

const SHAPE_COLORS = {
  'CCECCECCECCEECCECCECCECC': 'blue',
  'ECCECCECCECCCCECCECCECCE': 'red',
  'CCECCECCECCECCECCECCECCE': null,
  'ECCECCECCECCECCECCECCECC': null,
};

function solvedState() {
  return 'CCECCECCECCEECCECCECCECC'.split('');
}

function rotateSegment(arr, start, len, k) {
  const n = ((k % len) + len) % len;
  if (n === 0) return;
  const seg = arr.slice(start, start + len);
  const out = new Array(len);
  for (let i = 0; i < len; i++) out[(i + n) % len] = seg[i];
  for (let i = 0; i < len; i++) arr[start + i] = out[i];
}

function doSlice(arr) {
  for (let i = 0; i < 6; i++) [arr[i], arr[12 + i]] = [arr[12 + i], arr[i]];
}

function* tokenize(s) {
  let i = 0;
  const L = s.length;
  const ws = /\s/;
  const int = /^([+-]?\d+)/;
  const skip = () => { while (i < L && ws.test(s[i])) i++; };
  while (true) {
    skip();
    if (i >= L) return;
    const ch = s[i];
    if (ch === '(') {
      i++; skip();
      let m = s.slice(i).match(int);
      if (!m) { i++; continue; }
      const t = +m[1]; i += m[1].length; skip();
      if (s[i] === ',') i++; skip();
      m = s.slice(i).match(int);
      if (!m) { i++; continue; }
      const b = +m[1]; i += m[1].length; skip();
      if (s[i] === ')') i++; skip();
      const slash = s[i] === '/';
      if (slash) i++;
      yield { k: 'tb', t, b, slash };
      continue;
    }
    if (ch === '/') { i++; yield { k: '/' }; continue; }
    i++;
  }
}

function applyScramble(scr) {
  const a = solvedState();
  for (const tok of tokenize(scr)) {
    if (tok.k === 'tb') {
      rotateSegment(a, 0, 12, tok.t);
      rotateSegment(a, 12, 12, tok.b);
      if (tok.slash) doSlice(a);
    } else {
      doSlice(a);
    }
  }
  return a;
}

function getShapeKey(state) {
  return state.join('');
}

function findSlashPositions(scramble) {
  const positions = [];
  let i = 0;
  const L = scramble.length;
  const ws = /\s/;
  while (i < L) {
    if (ws.test(scramble[i])) { i++; continue; }
    if (scramble[i] === '(') {
      while (i < L && scramble[i] !== ')') i++;
      if (i < L) i++;
      while (i < L && ws.test(scramble[i])) i++;
      if (i < L && scramble[i] === '/') { positions.push(i); i++; }
      continue;
    }
    if (scramble[i] === '/') { positions.push(i); i++; continue; }
    i++;
  }
  return positions;
}

function run(scramble) {
  const slashPositions = findSlashPositions(scramble);
  if (slashPositions.length === 0) return scramble;

  // Build state after each slash
  const statesAfter = slashPositions.map(pos =>
    applyScramble(scramble.slice(0, pos + 1))
  );

  // State before slash si = state after slash si-1, or solved if si=0
  const stateBefore = si => applyScramble(scramble.slice(0, slashPositions[si]));

  // Track square/non-square transitions, insert markers
  let wasSquare = getShapeKey(solvedState()) in SHAPE_COLORS;
  const markers = new Array(slashPositions.length).fill('');

  for (let si = 0; si < slashPositions.length; si++) {
    const key = getShapeKey(statesAfter[si]);
    const isSquare = key in SHAPE_COLORS;
    console.log(`After slash ${si + 1}: ${key} → ${isSquare ? 'square ✓' : 'NOT square ✗'}`);

    if (wasSquare && !isSquare) {
      // leaving square — color from state just before this slash
      const beforeKey = getShapeKey(stateBefore(si));
      const color = SHAPE_COLORS[beforeKey];
      markers[si] = color === 'blue' ? '\\' : color === 'red' ? '|' : '?';
    } else if (!wasSquare && isSquare) {
      // entering square — color from state just after this slash
      const color = SHAPE_COLORS[key];
      markers[si] = color === 'blue' ? '\\' : color === 'red' ? '|' : '?';
    }

    wasSquare = isSquare;
  }

  // Insert markers back into scramble (right to left to preserve positions)
  let result = scramble;
  for (let si = slashPositions.length - 1; si >= 0; si--) {
    if (markers[si]) {
      const at = slashPositions[si] + 1;
      result = result.slice(0, at) + markers[si] + result.slice(at);
    }
  }
  return result;
}

const scramble = process.argv.slice(2).join(' ');
if (!scramble) { console.error('Usage: node sq1_tester.js "<scramble>"'); process.exit(1); }

console.log('Input:', scramble);
console.log('\nTracing slashes:\n');
console.log('\nOutput:', run(scramble));