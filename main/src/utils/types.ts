export type CubeState = {
  position: number[];
  partial: number[];
  middle: number;
  middlePartial: number;
};

export type Modal = "settings" | "how" | "about" | "debug" | null;

export type FavoriteBin = { name: string; algorithms: string[] };

export type Solution = {
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

export type OutputLine = { raw: string; karn: string; isSolution: boolean; algRaw?: string };

export type RatingResult = {
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

export type TwoGenStatus = { compatibility: number; cornersTwo: boolean; cornersPseudo: boolean };

export type DisplaySolution = Solution & { display: string; alg: string };

export type DropdownProps = {
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

export type CubeActions = {
  u: () => void;
  up: () => void;
  d: () => void;
  dp: () => void;
  slice: () => void;
  reset: () => void;
  set: (state: CubeState) => void;
};

export type TauriGlobal = {
  core?: { invoke: <T>(command: string, args?: Record<string, unknown>) => Promise<T> };
  event?: { listen: (name: string, handler: (event: { payload: unknown }) => void) => Promise<() => void> };
  Channel?: new <T>() => { onmessage: (message: T) => void };
};
