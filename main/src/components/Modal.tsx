import { useRef, type ReactNode } from "react";
import { t, LANGUAGES, LangCode } from "../i18n";
import type { Modal } from "../utils";
import type { OutputMode } from "../utils";
import { Icon, type IconName } from "./Icon";
import { DebugRateGraph, LiveDebugData } from "./DebugRateGraph";

const LINKS: Record<string, string> = {
  jaap: "https://www.jaapsch.net/puzzles/",
  github: "https://github.com/qqwref",
  wca: "https://www.worldcubeassociation.org/persons/2006GOTT01",
  abid: "https://www.worldcubeassociation.org/persons/2024ASHR02",
  matt: "https://www.worldcubeassociation.org/persons/2023MAOS01",
};

// Translated strings may embed <strong>, <em>, <link:name> and <icon:name>
// tags. Splitting on these universal tags (instead of English words) lets
// translations reorder words freely without breaking the styling.
function renderMarkup(src: string, linkHandlers?: Record<string, () => void>): ReactNode {
  const tokens: (string | { closing: boolean; kind: string; name?: string })[] = [];
  const re = /<(\/)?(strong|em|link:([a-zA-Z0-9]+)|icon:([a-zA-Z0-9]+))>/g;
  let last = 0;
  let m: RegExpExecArray | null;
  while ((m = re.exec(src))) {
    if (m.index > last) tokens.push(src.slice(last, m.index));
    tokens.push({ closing: !!m[1], kind: m[2], name: m[3] ?? m[4] });
    last = re.lastIndex;
  }
  if (last < src.length) tokens.push(src.slice(last));

  const build = (i: number, stop?: string): { nodes: ReactNode[]; next: number } => {
    const nodes: ReactNode[] = [];
    while (i < tokens.length) {
      const tok = tokens[i];
      if (typeof tok === "string") {
        nodes.push(tok);
        i++;
        continue;
      }
      if (tok.closing) {
        if (tok.kind === stop) return { nodes, next: i + 1 };
        i++;
        continue;
      }
      if (tok.kind === "strong" || tok.kind === "em") {
        const sub = build(i + 1, tok.kind);
        nodes.push(
          tok.kind === "strong"
            ? <strong key={nodes.length}>{sub.nodes}</strong>
            : <em key={nodes.length}>{sub.nodes}</em>,
        );
        i = sub.next;
        continue;
      }
      if (tok.kind.startsWith("link:")) {
        const sub = build(i + 1, tok.kind);
        const handler = linkHandlers?.[tok.name ?? ""];
        nodes.push(
          handler ? (
            <a key={nodes.length} href="#" className="modal-internal-link" onClick={(e) => { e.preventDefault(); handler(); }}>{sub.nodes}</a>
          ) : (
            <a key={nodes.length} href={LINKS[tok.name ?? ""]} target="_blank" rel="noreferrer">{sub.nodes}</a>
          ),
        );
        i = sub.next;
        continue;
      }
      if (tok.kind.startsWith("icon:")) {
        nodes.push(
          <strong key={nodes.length} className="howto-icon">
            <Icon name={tok.name as IconName} size={12} />
          </strong>,
        );
        i++;
        continue;
      }
      i++;
    }
    return { nodes, next: i };
  };
  return <>{build(0).nodes}</>;
}

export function Modal({
  type,
  close,
  settings,
  notation,
  debugStats,
  liveDebug,
  docNav,
}: {
  type: Exclude<Modal, null>;
  close: () => void;
  docNav?: {
    openSq1optV2: () => void;
    openSq1optV1: () => void;
    openHowToUse: () => void;
  };
  settings?: {
    ignoreTransforms: boolean; setIgnoreTransforms: (value: boolean) => void;
    debugOutput: boolean; setDebugOutput: (value: boolean) => void;
    zoom: number; setZoom: (value: number) => void;
    pageSize: number; setPageSize: (value: number) => void;
    showAll: boolean; setShowAll: (value: boolean) => void;
    useLessRam: boolean; setUseLessRam: (value: boolean) => void;
    pageSizeOptions: number[];
    onRequestShowAll: () => void;
    onOpenDiskSpace?: () => void;
    onOpenWeights?: () => void;
    disabled: boolean;
    hasMaxTurn: boolean;
    language?: LangCode;
    setLanguage?: (code: LangCode) => void;
  };
  notation?: {
    outputMode: OutputMode; setOutputMode: (value: OutputMode) => void;
    abidNotation: boolean; setAbidNotation: (value: boolean) => void;
    disabled: boolean;
  };
  debugStats?: { elapsed: string; solutionCount: number; nodesSearched: number } | null;
  liveDebug?: (() => LiveDebugData) | null;
}) {
  const karnSelected = notation && (notation.outputMode === "karn" || notation.outputMode === "cskarn");
  const docLinkHandlers: Record<string, () => void> = {
    here: () => docNav?.openSq1optV2(),
    sq1optOld: () => docNav?.openSq1optV1(),
    howToUse: () => docNav?.openHowToUse(),
  };
  const content =
    type === "settings" ? (
      <div className="modal-article">
        <h2>{t('modal.settings.title')}</h2>
        <div className="settings-list">
          <label className="modal-check">
            <input type="checkbox" checked={(settings?.ignoreTransforms || settings?.hasMaxTurn) ?? false} disabled={settings?.disabled || settings?.hasMaxTurn} onChange={(e) => settings?.setIgnoreTransforms(e.target.checked)} />
            <span>{t('modal.settings.ignoreTransforms')}</span>
          </label>
          <label className="modal-check">
            <input type="checkbox" checked={settings?.debugOutput ?? false} disabled={settings?.disabled} onChange={(e) => settings?.setDebugOutput(e.target.checked)} />
            <span>{t('modal.settings.debugOutput')}</span>
          </label>
          <label className="modal-check">
            <input type="checkbox" checked={settings?.showAll ?? false} disabled={settings?.disabled} onChange={(e) => { if (e.target.checked) settings?.onRequestShowAll(); else settings?.setShowAll(false); }} />
            <span>{t('modal.settings.showAll')}</span>
          </label>
          <label className="modal-check">
            <input type="checkbox" checked={settings?.useLessRam ?? false} onChange={(e) => settings?.setUseLessRam(e.target.checked)} />
            <span>{t('modal.settings.useLessRam')}</span>
          </label>
        </div>
        <button className="setting-button" disabled={settings?.disabled} onClick={() => settings?.onOpenWeights?.()}>{t('modal.weights.manage')}</button>
        <button className="setting-button" onClick={() => settings?.onOpenDiskSpace?.()}>{t('modal.disk.manage')}</button>
        <div className="settings-slider">
          <span className="settings-slider-label">{t('modal.settings.pageSize')}</span>
          <input type="range" min="0" max={String((settings?.pageSizeOptions.length ?? 1) - 1)} step="1" value={Math.max(0, (settings?.pageSizeOptions ?? []).indexOf(settings?.pageSize ?? 1000))} disabled={settings?.disabled || settings?.showAll} onChange={(e) => settings?.setPageSize((settings?.pageSizeOptions ?? [])[Number(e.target.value)] ?? 1000)} />
          <span className="settings-slider-value">{(settings?.pageSize ?? 1000).toLocaleString()}</span>
        </div>
        <div className="settings-slider">
          <span className="settings-slider-label">{t('modal.settings.uiScale')}</span>
          <input type="range" min="0.5" max="2" step="0.1" value={settings?.zoom ?? 1} disabled={settings?.disabled} onChange={(e) => settings?.setZoom(Number(e.target.value))} />
          <span className="settings-slider-value">{Math.round((settings?.zoom ?? 1) * 100)}%</span>
          {(settings?.zoom ?? 1) !== 1 && <button className="settings-slider-reset" disabled={settings?.disabled} onClick={() => settings?.setZoom(1)}>{t('modal.settings.reset')}</button>}
        </div>
        <div className="language-select">
          <label>{t('modal.settings.language')}</label>
          <select value={settings?.language ?? 'en'} onChange={(e) => settings?.setLanguage?.(e.target.value as LangCode)}>
            {Object.entries(LANGUAGES).map(([code, name]) => (
              <option key={code} value={code}>{name}</option>
            ))}
          </select>
        </div>
      </div>
    ) : type === "notation" ? (
      <div className="modal-article">
        <h2>{t('modal.notation.title')}</h2>
        <div className="modal-section-title">{t('modal.notation.numericTitle')}</div>
        <div className="modal-radio-group">
          {(["default", "clean", "wca", "abid"] as OutputMode[]).map((mode) => (
            <label key={mode} className="modal-radio">
              <input type="radio" name="notation-numeric" value={mode} checked={notation?.outputMode === mode} disabled={notation?.disabled} onChange={() => notation?.setOutputMode(mode)} />
              <span>{t('modal.notation.' + mode)}</span>
            </label>
          ))}
        </div>
        <div className="modal-section-title">{t('modal.notation.karnTitle')}</div>
        <div className="modal-radio-group">
          <label className="modal-radio">
            <input type="radio" name="notation-karn" value="karn" checked={notation?.outputMode === "karn"} disabled={notation?.disabled} onChange={() => notation?.setOutputMode("karn")} />
            <span>{t('modal.notation.karnTraditional')}</span>
          </label>
          <label className="modal-radio">
            <input type="radio" name="notation-karn" value="cskarn" checked={notation?.outputMode === "cskarn"} disabled={notation?.disabled} onChange={() => notation?.setOutputMode("cskarn")} />
            <span>{t('modal.notation.karnSmart')}</span>
          </label>
        </div>
        <label className="modal-check modal-notation-abid">
          <input type="checkbox" checked={notation?.abidNotation ?? false} disabled={notation?.disabled || !karnSelected} onChange={(e) => notation?.setAbidNotation(e.target.checked)} />
          <span>{t('modal.notation.abidNegatives')}</span>
        </label>
      </div>
    ) : type === "debug" ? (
      <div className="modal-article">
        <h2>{t('modal.debug.title')}</h2>
        <div className="debug-stats-grid">
          <span className="debug-label">{t('modal.debug.elapsed')}</span>
          <span className="debug-value">{debugStats?.elapsed ?? t('modal.debug.placeholder')}s</span>
          <span className="debug-label">{t('modal.debug.solutions')}</span>
          <span className="debug-value">{debugStats?.solutionCount ?? t('modal.debug.placeholder')}</span>
          <span className="debug-label">{t('modal.debug.nodesSearched')}</span>
          <span className="debug-value">{debugStats?.nodesSearched != null ? debugStats.nodesSearched.toLocaleString() : t('modal.debug.placeholder')}</span>
          </div>
        {liveDebug && <DebugRateGraph
          getLive={liveDebug}
          solLabel={t('modal.debug.graphSolutions')}
          nodeLabel={t('modal.debug.graphNodes')}
          avgLabel={t('modal.debug.graphAvg')}
          stddevLabel={t('modal.debug.graphStddev')}
        />}
      </div>
    ) : type === "about" ? (
      <div className="modal-article">
        <h2>{t('modal.about.title')}</h2>
        <p>{renderMarkup(t('modal.about.p1'))}</p>
        <p>{renderMarkup(t('modal.about.p2'))}</p>
        <p>{renderMarkup(t('modal.about.p3'), docLinkHandlers)}</p>
        <p>{renderMarkup(t('modal.about.p4'))}</p>
        <ul>
          <li>{t('modal.about.li1')}</li>
          <li>{t('modal.about.li2')}</li>
          <li>{t('modal.about.li3')}</li>
          <li>{t('modal.about.li4')}</li>
        </ul>
        <p>{renderMarkup(t('modal.about.p5'))}</p>
      </div>
    ) : type === "sq1optv2" || type === "sq1optv1" ? (
      <div className="modal-article doc-viewer">
        <h2>{type === "sq1optv2" ? t('modal.sq1optv2.title') : t('modal.sq1optv1.title')}</h2>
        <p className="doc-banner">{renderMarkup(t('modal.docBanner'), docLinkHandlers)}</p>
        <pre className="doc-body">{renderMarkup(type === "sq1optv2" ? t('modal.sq1optv2.body') : t('modal.sq1optv1.body'), docLinkHandlers)}</pre>
      </div>
    ) : (
      <div className="modal-article how-to-use">
        <h2>{t('modal.howToUse.title')}</h2>
        <div className="modal-section-title">{t('modal.howToUse.sectionKeyboard')}</div>
        <ul>
          <li>{renderMarkup(t('modal.howToUse.shortcut1'))}</li>
          <li>{renderMarkup(t('modal.howToUse.shortcut2'))}</li>
          <li>{renderMarkup(t('modal.howToUse.shortcut3'))}</li>
          <li>{renderMarkup(t('modal.howToUse.shortcut4'))}</li>
          <li>{renderMarkup(t('modal.howToUse.shortcut5'))}</li>
          <li>{renderMarkup(t('modal.howToUse.shortcut6'))}</li>
        </ul>
        <p>{renderMarkup(t('modal.howToUse.swapHint'))}</p>
        <ul>
          <li>{renderMarkup(t('modal.howToUse.shortcutJ'))}</li>
          <li>{renderMarkup(t('modal.howToUse.shortcutS'))}</li>
          <li>{renderMarkup(t('modal.howToUse.shortcutI'))}</li>
          <li>{renderMarkup(t('modal.howToUse.shortcutH'))}</li>
          <li>{renderMarkup(t('modal.howToUse.shortcutW'))}</li>
        </ul>
        <div className="modal-section-title">{t('modal.howToUse.sectionScrambleInput')}</div>
        <p>{renderMarkup(t('modal.howToUse.applyHint'))}</p>
        <p>{t('modal.howToUse.modeHint')}</p>
        <ul>
          <li>{renderMarkup(t('modal.howToUse.modeScram'))}</li>
          <li>{renderMarkup(t('modal.howToUse.modeAlg'))}</li>
          <li>{renderMarkup(t('modal.howToUse.modePos'))}</li>
        </ul>
        <p>{renderMarkup(t('modal.howToUse.enterHint'))}</p>
        <div className="modal-section-title">{t('modal.howToUse.sectionOptions')}</div>
        <p>{t('modal.howToUse.hoverHint')}</p>
        <ul>
          <li>{renderMarkup(t('modal.howToUse.descOutput'))}</li>
          <li>{renderMarkup(t('modal.howToUse.descMetric'))}</li>
          <li>{renderMarkup(t('modal.howToUse.desc2Gen'))}</li>
          <li>{renderMarkup(t('modal.howToUse.descAll'))}</li>
          <li>{renderMarkup(t('modal.howToUse.descDepths'))}</li>
          <li>{renderMarkup(t('modal.howToUse.descCubeshape'))}</li>
          <li>{renderMarkup(t('modal.howToUse.descAngle'))}</li>
          <li>{renderMarkup(t('modal.howToUse.descNormalize'))}</li>
          <li>{renderMarkup(t('modal.howToUse.descLimits'))}</li>
        </ul>
        <div className="modal-section-title">{t('modal.howToUse.sectionSettings')}</div>
        <p>{renderMarkup(t('modal.howToUse.descSettingsOpen'))}</p>
        <ul>
          <li>{renderMarkup(t('modal.howToUse.descIgnoreTransforms'))}</li>
          <li>{renderMarkup(t('modal.howToUse.descDebugOutput'))}</li>
          <li>{renderMarkup(t('modal.howToUse.descUiScale'))}</li>
        </ul>
        <div className="modal-section-title">{t('modal.howToUse.sectionOutput')}</div>
        <p>{t('modal.howToUse.outputIntro')}</p>
        <ul>
          <li>{renderMarkup(t('modal.howToUse.descTerminalView'))}</li>
          <li>{renderMarkup(t('modal.howToUse.descTableView'))}</li>
        </ul>
        <p>{t('modal.howToUse.descContextMenu')}</p>
        <ul>
          <li>{renderMarkup(t('modal.howToUse.descCopyAlg'))}</li>
          <li>{renderMarkup(t('modal.howToUse.descAddFav'))}</li>
        </ul>
        <p>{t('modal.howToUse.otherButtons')}</p>
        <ul>
          <li>{t('modal.howToUse.outputTools1')}</li>
          <li>{renderMarkup(t('modal.howToUse.outputTools2'))}</li>
          <li>{renderMarkup(t('modal.howToUse.outputTools3'))}</li>
          <li>{renderMarkup(t('modal.howToUse.outputTools4'))}</li>
        </ul>
        <div className="modal-section-title">{t('modal.howToUse.sectionErgo')}</div>
        <p>{renderMarkup(t('modal.howToUse.ergoCond'))}</p>
        <p>{renderMarkup(t('modal.howToUse.ergoAdvice'))}</p>
        <p>{renderMarkup(t('modal.howToUse.ergoConfig'))}</p>
        <p>{renderMarkup(t('modal.howToUse.ergoSections'))}</p>
        <ul>
          <li>{renderMarkup(t('modal.howToUse.ergoSection1'))}</li>
          <li>{renderMarkup(t('modal.howToUse.ergoSection2'))}</li>
          <li>{renderMarkup(t('modal.howToUse.ergoSection3'))}</li>
          <li>{renderMarkup(t('modal.howToUse.ergoSection4'))}</li>
        </ul>
        <p>{renderMarkup(t('modal.howToUse.ergoWeights'))}</p>
        <div className="modal-section-title">{t('modal.howToUse.sectionFilter')}</div>
        <p>{renderMarkup(t('modal.howToUse.filterOpen'))}</p>
        <ul>
          <li>{renderMarkup(t('modal.howToUse.filterMatchCase'))}</li>
          <li>{renderMarkup(t('modal.howToUse.filterNegate'))}</li>
          <li>{renderMarkup(t('modal.howToUse.filterRegex'))}</li>
        </ul>
        <div className="modal-section-title">{t('modal.howToUse.sectionFavorites')}</div>
        <p>{renderMarkup(t('modal.howToUse.favIntro'))}</p>
        <p>{t('modal.howToUse.favAdd')}</p>
        <p>{renderMarkup(t('modal.howToUse.favSaveAll'))}</p>
        <p>{renderMarkup(t('modal.howToUse.favBinId'))}</p>
        <p>{t('modal.howToUse.favInside')}</p>
        <ul>
          <li>{renderMarkup(t('modal.howToUse.favApply'))}</li>
          <li>{renderMarkup(t('modal.howToUse.favRename'))}</li>
          <li>{renderMarkup(t('modal.howToUse.favCopy'))}</li>
          <li>{renderMarkup(t('modal.howToUse.favDelete'))}</li>
          <li>{renderMarkup(t('modal.howToUse.favRemove'))}</li>
        </ul>
        <p>{t('modal.howToUse.favStorage')}</p>
      </div>
    );
  const shadeStartRef = useRef<EventTarget | null>(null);
  const shadeEndRef = useRef<EventTarget | null>(null);
  return (
    <div className="modal-shade" onPointerDown={(e) => { shadeStartRef.current = e.target; }} onPointerUp={(e) => { shadeEndRef.current = e.target; }} onClick={() => {
      const startOutside = !shadeStartRef.current || !(shadeStartRef.current as Element).closest(".modal");
      const endOutside = !shadeEndRef.current || !(shadeEndRef.current as Element).closest(".modal");
      if (startOutside && endOutside) close();
      shadeStartRef.current = null;
      shadeEndRef.current = null;
    }}>
      <div className={"modal" + (type === "sq1optv2" || type === "sq1optv1" ? " modal-doc" : "")} onClick={(e) => e.stopPropagation()}>
        <button className="modal-close" onClick={close}>
          <Icon name="close" />
        </button>
        {content}
      </div>
    </div>
  );
}
