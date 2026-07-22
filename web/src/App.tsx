import { useEffect, useRef, useState } from "react";

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

function twistable(p: number[]) {
  return p[0] !== p[11] && p[5] !== p[6] && p[12] !== p[23] && p[17] !== p[18];
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
};
function Cube({
  onChange,
  actionsRef,
}: {
  onChange: (s: CubeState) => void;
  actionsRef: React.MutableRefObject<CubeActions | undefined>;
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
  const update = (n: CubeState) => {
    setS(n);
    onChange(n);
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
    const timer = window.setInterval(
      () =>
        setHoverProgress((old) => {
          const next: { [key: number]: number } = { ...old };
          for (const key of new Set([
            ...Object.keys(next).map(Number),
            hovered,
          ])) {
            const target = key === hovered ? 1 : 0;
            const value = next[key] || 0;
            next[key] = target
              ? Math.min(1, value + 0.08)
              : Math.max(0, value - 0.08);
          }
          return next;
        }),
      16,
    );
    return () => window.clearInterval(timer);
  }, [hovered]);
  const invoke = (key: keyof CubeActions) => {
    if (key === "reset") {
      setSelected(-1);
      update({
        position: [...solved],
        partial: Array(24).fill(0),
        middle: 1,
        middlePartial: 0,
      });
      return;
    }
    setSelected(-1);
    update(doMove(s, key));
  };
  actionsRef.current = {
    u: () => invoke("u"),
    up: () => invoke("up"),
    d: () => invoke("d"),
    dp: () => invoke("dp"),
    slice: () => invoke("slice"),
    reset: () => invoke("reset"),
  };
  useEffect(() => {
    const key = (e: KeyboardEvent) => {
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
        invoke(action);
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
      update({ ...s, middle });
      return;
    }
    if (piece < 0) return;
    if (e.button === 2) {
      const partial = [...s.partial];
      partial[piece] = (partial[piece] + 1) % 3;
      if (piece < 23 && s.position[piece] === s.position[piece + 1])
        partial[piece + 1] = partial[piece];
      update({ ...s, partial });
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
    update({ ...s, position: p, partial: q });
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
}: {
  type: Exclude<Modal, null>;
  close: () => void;
}) {
  const content =
    type === "settings" ? (
      <>
        <h2>Settings</h2>
        <label className="modal-check">
          <input type="checkbox" /> Use smarter karn
        </label>
        <label className="modal-check">
          <input type="checkbox" /> Abid's notation
        </label>
        <label className="modal-check">
          <input type="checkbox" /> Ignore move equivalences
        </label>
        <label className="modal-check">
          <input type="checkbox" /> Debug output
        </label>
      </>
    ) : type === "about" ? (
      <>
        <h2>About Solve-A-Squan</h2>
        <p>
          This program stemmed from the optimal Square-1 solver by Jaap
          Scherphuis.
        </p>
        <p>v3 is created by Abid Ibn Ashraf and Matt Mao.</p>
        <p>
          New in v3: graphical UI, improved karnotation support, and algorithm
          ergonomics tools.
        </p>
      </>
    ) : (
      <>
        <h2>How to Use</h2>
        <p>
          <b>Keyboard shortcuts</b>
        </p>
        <p>J = U, F = U′, S = D, L = D′, I/K = Slice, Esc = Reset.</p>
        <p>
          Click two pieces to swap them. Right-click a piece to cycle its
          partial definition.
        </p>
        <p>
          Use the input mode to apply a scramble, invert an algorithm, or enter
          a raw position.
        </p>
        <p>Hover over options for their descriptions.</p>
      </>
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
  const [menu, setMenu] = useState(false),
    [modal, setModal] = useState<Modal>(null),
    [modeMenu, setModeMenu] = useState(false),
    [input, setInput] = useState(""),
    [mode, setMode] = useState("SCRAMBLE"),
    [cubePos, setCubePos] = useState("A1B2C3D45E6F7G8H-"),
    [metric, setMetric] = useState("Slice"),
    [two, setTwo] = useState("None"),
    [angle, setAngle] = useState("None"),
    [normalize, setNormalize] = useState("None"),
    [all, setAll] = useState(false),
    [generator, setGenerator] = useState(false),
    [cubeShape, setCubeShape] = useState(false),
    [ignoreMiddle, setIgnoreMiddle] = useState(false),
    [karn, setKarn] = useState(true),
    [terminal, setTerminal] = useState<string[]>([]);
  const chooseMode = (next: string) => {
    setMode(next);
    setInput("");
    setModeMenu(false);
  };
  const cycleMode = () =>
    chooseMode(
      mode === "SCRAMBLE" ? "ALG" : mode === "ALG" ? "POSITION" : "SCRAMBLE",
    );
  const apply = () => {
    if (mode === "POSITION" && input.trim()) setCubePos(input.trim());
    setInput("");
  };
  return (
    <div className="app">
      <header>
        <button className="hamburger" onClick={() => setMenu(true)}>
          ☰
        </button>
        <div className="brand">
          <b>SOLVE-A-SQUAN</b>
          <small>by Abid and Matt</small>
        </div>
      </header>
      <div className="inputbar">
        <div className="mode-control">
          <button className="mode" onClick={cycleMode}>
            {mode}
          </button>
          <button
            className="arrow"
            aria-label="Choose input mode"
            onClick={() => setModeMenu((v) => !v)}
          >
            ▾
          </button>
          {modeMenu && (
            <div className="mode-menu">
              <button
                className={mode === "SCRAMBLE" ? "selected" : ""}
                onClick={() => chooseMode("SCRAMBLE")}
              >
                Scramble
              </button>
              <button
                className={mode === "ALG" ? "selected" : ""}
                onClick={() => chooseMode("ALG")}
              >
                Alg
              </button>
              <button
                className={mode === "POSITION" ? "selected" : ""}
                onClick={() => chooseMode("POSITION")}
              >
                Position
              </button>
            </div>
          )}
        </div>
        <div className="input-control">
          <input
            value={input}
            onChange={(e) => setInput(e.target.value)}
            onKeyDown={(e) => e.key === "Enter" && apply()}
            placeholder={
              mode === "POSITION"
                ? "ABCDEFGH12345678-"
                : "1,0 / 3,3 / 0,-3 / ...  (supports karn)"
            }
          />
          <button className="apply" onClick={apply}>
            Apply
          </button>
        </div>
      </div>
      <div className="main-grid">
        <aside className="cube-column">
          <Cube
            actionsRef={cubeActions}
            onChange={(s) =>
              setCubePos(
                s.position
                  .map((x, i) =>
                    s.partial[i]
                      ? /[A-H]/.test("ABCDEFGH"[x])
                        ? "W"
                        : "Z"
                      : "ABCDEFGH12345678"[x],
                  )
                  .join("") + (s.middlePartial ? "" : s.middle ? "/" : "-"),
              )
            }
          />
          <div className="moves">
            <button onClick={() => cubeActions.current?.up()}>U′</button>
            <button
              className="slice"
              onClick={() => cubeActions.current?.slice()}
            >
              Slice [I/K]
            </button>
            <button onClick={() => cubeActions.current?.u()}>U</button>
            <button onClick={() => cubeActions.current?.d()}>D</button>
            <button onClick={() => cubeActions.current?.dp()}>D′</button>
          </div>
          <div className="undo">
            <button>Undo (Ctrl+Z)</button>
            <button disabled>Redo (Ctrl+Y)</button>
          </div>
          <button className="solve">▶ Solve [Ctrl+Enter]</button>
        </aside>
        <section className="right-column">
          <div className="options-panel">
            <h2>Options</h2>
            <div className="select-grid">
              <label>
                Metric
                <select
                  value={metric}
                  onChange={(e) => setMetric(e.target.value)}
                >
                  <option>Slice</option>
                  <option>Move</option>
                  <option>Angle</option>
                </select>
              </label>
              <label>
                2 Gen
                <select value={two} onChange={(e) => setTwo(e.target.value)}>
                  <option>None</option>
                  <option>Pseudo 2 Gen</option>
                  <option>2 Gen</option>
                </select>
              </label>
              <label>
                Lock layer angle on preabf
                <select
                  value={angle}
                  onChange={(e) => setAngle(e.target.value)}
                >
                  <option>None</option>
                  <option>Both</option>
                  <option>Top</option>
                  <option>Bottom</option>
                </select>
              </label>
              <label>
                Normalize ABF
                <select
                  value={normalize}
                  onChange={(e) => setNormalize(e.target.value)}
                >
                  <option>None</option>
                  <option>Both</option>
                  <option>PreABF</option>
                  <option>PostABF</option>
                </select>
              </label>
            </div>
            <div className="check-grid">
              <label>
                <input
                  type="checkbox"
                  checked={all}
                  onChange={(e) => setAll(e.target.checked)}
                />{" "}
                All optimal
              </label>
              <label>
                <input
                  type="checkbox"
                  checked={generator}
                  onChange={(e) => setGenerator(e.target.checked)}
                />{" "}
                Generator alg
              </label>
              <label>
                <input
                  type="checkbox"
                  checked={cubeShape}
                  onChange={(e) => setCubeShape(e.target.checked)}
                />{" "}
                Stay in cubeshape
              </label>
              <label>
                <input
                  type="checkbox"
                  checked={ignoreMiddle}
                  onChange={(e) => setIgnoreMiddle(e.target.checked)}
                />{" "}
                Ignore equator
              </label>
              <label>
                <input
                  type="checkbox"
                  checked={karn}
                  onChange={(e) => setKarn(e.target.checked)}
                />{" "}
                Karn output
              </label>
            </div>
            <div className="limit-grid">
              <label>
                Max top turn:<input defaultValue="3" />
              </label>
              <label>
                Max bottom turn:<input defaultValue="3" />
              </label>
              <label>
                Max total turn:
                <input defaultValue="6" />
              </label>
              <label>
                Specific depths:
                <input placeholder="e.g. 8,9" />
              </label>
            </div>
          </div>
          <div className="terminal">
            <div className="terminal-head">
              <span>#</span>
              <b>Solution</b>
              <span>Slices</span>
            </div>
            {terminal.length ? (
              terminal.map((x, i) => (
                <div className="solution" key={i}>
                  <span>{i + 1}</span>
                  <code>{x}</code>
                  <span>9</span>
                </div>
              ))
            ) : (
              <div className="empty">Solver output will appear here.</div>
            )}
          </div>
        </section>
      </div>
      {menu && (
        <div className="shade" onClick={() => setMenu(false)}>
          <nav className="sidebar" onClick={(e) => e.stopPropagation()}>
            <div className="side-head">
              <b>Menu</b>
              <button onClick={() => setMenu(false)}>✕</button>
            </div>
            <button
              onClick={() => {
                setModal("settings");
                setMenu(false);
              }}
            >
              ⚙　Settings
            </button>
            <button
              onClick={() => {
                setModal("how");
                setMenu(false);
              }}
            >
              ?　 How to Use
            </button>
            <button
              onClick={() => {
                setModal("about");
                setMenu(false);
              }}
            >
              ℹ　About
            </button>
          </nav>
        </div>
      )}
      {modal && <Modal type={modal} close={() => setModal(null)} />}
    </div>
  );
}
