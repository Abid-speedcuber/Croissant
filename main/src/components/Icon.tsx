export type IconName =
  | "search" | "close" | "timer" | "copy" | "list" | "grid"
  | "expand" | "collapse" | "chevronDown" | "chevronLeft" | "chevronRight"
  | "heart" | "dots" | "trash" | "negate" | "stop";

// Single source of truth for every UI icon so they share one stroke weight/style
// instead of relying on inconsistent Unicode glyph rendering across platforms.
export function Icon({ name, size = 14 }: { name: IconName; size?: number }) {
  const common = {
    width: size, height: size, viewBox: "0 0 24 24", fill: "none", stroke: "currentColor",
    strokeWidth: 2.2, strokeLinecap: "round" as const, strokeLinejoin: "round" as const,
    "aria-hidden": true as const,
  };
  switch (name) {
    case "search": return <svg {...common}><circle cx="11" cy="11" r="7" /><line x1="21" y1="21" x2="16.65" y2="16.65" /></svg>;
    case "negate": return <svg {...common}><circle cx="12" cy="12" r="9" /><line x1="5.5" y1="18.5" x2="18.5" y2="5.5" /></svg>;
    case "close": return <svg {...common}><line x1="6" y1="6" x2="18" y2="18" /><line x1="18" y1="6" x2="6" y2="18" /></svg>;
    case "timer": return <svg {...common}><circle cx="12" cy="13" r="8" /><line x1="12" y1="13" x2="12" y2="9" /><line x1="9" y1="3" x2="15" y2="3" /></svg>;
    case "copy": return <svg {...common}><rect x="3" y="3" width="12" height="12" rx="1.5" /><rect x="9" y="9" width="12" height="12" rx="1.5" /></svg>;
    case "list": return <svg {...common}><rect x="3" y="3" width="18" height="18" rx="2" /><line x1="6" y1="8" x2="18" y2="8" /><line x1="6" y1="12" x2="18" y2="12" /><line x1="6" y1="16" x2="18" y2="16" /></svg>;
    case "grid": return <svg {...common}><rect x="3" y="3" width="18" height="18" rx="2" /><line x1="12" y1="3" x2="12" y2="21" /><line x1="3" y1="12" x2="21" y2="12" /></svg>;
    case "expand": return <svg {...common}><polyline points="15 3 21 3 21 9" /><polyline points="9 21 3 21 3 15" /><line x1="21" y1="3" x2="14" y2="10" /><line x1="3" y1="21" x2="10" y2="14" /></svg>;
    case "collapse": return <svg {...common}><polyline points="4 9 9 9 9 4" /><polyline points="20 15 15 15 15 20" /><line x1="9" y1="9" x2="3" y2="3" /><line x1="15" y1="15" x2="21" y2="21" /></svg>;
    case "chevronDown": return <svg {...common}><polyline points="6 9 12 15 18 9" /></svg>;
    case "chevronLeft": return <svg {...common}><polyline points="15 18 9 12 15 6" /></svg>;
    case "chevronRight": return <svg {...common}><polyline points="9 18 15 12 9 6" /></svg>;
    case "heart": return <svg {...common}><path d="M12 21s-7.5-4.6-10-9.3C.5 8 2 4 6 4c2.2 0 3.7 1.2 6 3.4C14.3 5.2 15.8 4 18 4c4 0 5.5 4 4 7.7C19.5 16.4 12 21 12 21z" /></svg>;
    case "dots": return <svg {...common} fill="currentColor" stroke="none"><circle cx="12" cy="5" r="1.6" /><circle cx="12" cy="12" r="1.6" /><circle cx="12" cy="19" r="1.6" /></svg>;
    case "trash": return <svg {...common}><polyline points="4 7 20 7" /><path d="M6 7l1 13a2 2 0 0 0 2 2h6a2 2 0 0 0 2-2l1-13" /><path d="M9 7V4a1 1 0 0 1 1-1h4a1 1 0 0 1 1 1v3" /></svg>;
    case "stop": return <svg {...common} fill="currentColor" stroke="none"><rect x="5" y="5" width="14" height="14" rx="2.5" /></svg>;
  }
}
