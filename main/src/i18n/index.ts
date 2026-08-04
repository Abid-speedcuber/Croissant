import en from './en.json';

export const LANGUAGES: Record<string, string> = { en: 'English' };
export type LangCode = keyof typeof LANGUAGES;

const STORAGE_KEY = 'croissant-lang';

function loadLang(): LangCode {
  try {
    const stored = localStorage.getItem(STORAGE_KEY);
    if (stored && stored in LANGUAGES) return stored as LangCode;
  } catch {}
  return 'en';
}

let currentLang: LangCode = loadLang();

export function t(path: string): string {
  const keys = path.split('.');
  let val: any = translations[currentLang];
  for (const k of keys) {
    if (val == null || typeof val !== 'object') return path;
    val = val[k];
  }
  return typeof val === 'string' ? val : path;
}

export function tList(path: string): string[] {
  const keys = path.split('.');
  let val: any = translations[currentLang];
  for (const k of keys) {
    if (val == null || typeof val !== 'object') return [];
    val = val[k];
  }
  return Array.isArray(val) ? val.filter((entry): entry is string => typeof entry === 'string') : [];
}

export function setLang(code: LangCode) {
  currentLang = code;
  try { localStorage.setItem(STORAGE_KEY, code); } catch {}
}
export function getLang(): LangCode { return currentLang; }

const translations: Record<LangCode, Record<string, any>> = { en };
