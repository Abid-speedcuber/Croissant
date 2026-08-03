import { useRef } from "react";
import { t, LANGUAGES, LangCode } from "../i18n";
import type { Modal } from "../utils";
import { Icon } from "./Icon";

export function Modal({
  type,
  close,
  settings,
  debugStats,
}: {
  type: Exclude<Modal, null>;
  close: () => void;
  settings?: {
    smartKarn: boolean; setSmartKarn: (value: boolean) => void;
    abidNotation: boolean; setAbidNotation: (value: boolean) => void;
    ignoreTransforms: boolean; setIgnoreTransforms: (value: boolean) => void;
    debugOutput: boolean; setDebugOutput: (value: boolean) => void;
    zoom: number; setZoom: (value: number) => void;
    pageSize: number; setPageSize: (value: number) => void;
    showAll: boolean; setShowAll: (value: boolean) => void;
    useLessRam: boolean; setUseLessRam: (value: boolean) => void;
    pageSizeOptions: number[];
    onRequestShowAll: () => void;
    onOpenDiskSpace?: () => void;
    disabled: boolean;
    hasMaxTurn: boolean;
    language?: LangCode;
    setLanguage?: (code: LangCode) => void;
  };
  debugStats?: { elapsed: string; solutionCount: number; rollingRate: number; avgRate: number; stddevRate: number; nodesSearched: number; searchDepth: number; nodeRate: number } | null;
}) {
  const content =
    type === "settings" ? (
      <div className="modal-article">
        <h2>{t('modal.settings.title')}</h2>
        <div className="settings-list">
          <label className="modal-check">
            <input type="checkbox" checked={settings?.smartKarn ?? true} disabled={settings?.disabled} onChange={(e) => settings?.setSmartKarn(e.target.checked)} />
            <span>{t('modal.settings.smartKarn')}</span>
          </label>
          <label className="modal-check">
            <input type="checkbox" checked={settings?.abidNotation ?? false} disabled={settings?.disabled} onChange={(e) => settings?.setAbidNotation(e.target.checked)} />
            <span>{t('modal.settings.abidNotation')}</span>
          </label>
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
        <button className="settings-disk-space" onClick={() => settings?.onOpenDiskSpace?.()}>{t('modal.disk.manage')}</button>
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
    ) : type === "debug" ? (
      <div className="modal-article">
        <h2>{t('modal.debug.title')}</h2>
        <div className="debug-stats-grid">
          <span className="debug-label">{t('modal.debug.elapsed')}</span>
          <span className="debug-value">{debugStats?.elapsed ?? t('modal.debug.placeholder')}s</span>
          <span className="debug-label">{t('modal.debug.solutions')}</span>
          <span className="debug-value">{debugStats?.solutionCount ?? t('modal.debug.placeholder')}</span>
          <span className="debug-label">{t('modal.debug.solutionsPerMin')}</span>
          <span className="debug-value">{debugStats?.rollingRate != null ? debugStats.rollingRate.toFixed(1) : t('modal.debug.placeholder')}</span>
          <span className="debug-label">{t('modal.debug.avgRate')}</span>
          <span className="debug-value">{debugStats?.avgRate != null ? debugStats.avgRate.toFixed(1) : t('modal.debug.placeholder')}</span>
          <span className="debug-label">{t('modal.debug.stddevRate')}</span>
          <span className="debug-value">{debugStats?.stddevRate != null ? debugStats.stddevRate.toFixed(1) : t('modal.debug.placeholder')}</span>
          <span className="debug-label">{t('modal.debug.nodesSearched')}</span>
          <span className="debug-value">{debugStats?.nodesSearched != null ? debugStats.nodesSearched.toLocaleString() : t('modal.debug.placeholder')}</span>
          <span className="debug-label">{t('modal.debug.depth')}</span>
          <span className="debug-value">{debugStats?.searchDepth ?? t('modal.debug.placeholder')}</span>
          <span className="debug-label">{t('modal.debug.nodesPerSec')}</span>
          <span className="debug-value">{debugStats?.nodeRate != null ? Math.round(debugStats.nodeRate).toLocaleString() : t('modal.debug.placeholder')}</span>
        </div>
      </div>
    ) : type === "about" ? (
      <div className="modal-article">
        <h2>{t('modal.about.title')}</h2>
        <p>
          {t('modal.about.p1').split(t('modal.about.linkJaap'))[0]}
          <a href="https://www.jaapsch.net/puzzles/" target="_blank" rel="noreferrer">{t('modal.about.linkJaap')}</a>
          {t('modal.about.p1').split(t('modal.about.linkJaap'))[1]}
        </p>
        <p>
          {t('modal.about.p2').split(`(${t('modal.about.linkGithub')}, ${t('modal.about.linkWCA')})`)[0]}
          {'('}
          <a href="https://github.com/qqwref" target="_blank" rel="noreferrer">{t('modal.about.linkGithub')}</a>
          {', '}
          <a href="https://www.worldcubeassociation.org/persons/2006GOTT01" target="_blank" rel="noreferrer">{t('modal.about.linkWCA')}</a>
          {')' + t('modal.about.p2').split(`(${t('modal.about.linkGithub')}, ${t('modal.about.linkWCA')})`)[1]}
        </p>
        <p>
          {t('modal.about.p3').split(t('modal.about.linkHere'))[0]}
          <a href="https://github.com/abid/croissant/blob/main/docs/sq1opt_old.txt" target="_blank" rel="noreferrer">{t('modal.about.linkHere')}</a>
          {t('modal.about.p3').split(t('modal.about.linkHere'))[1]}
        </p>
        <p>
          {t('modal.about.p4').split('v3')[0]}
          <strong>v3</strong>
          {t('modal.about.p4').split('v3').slice(1).join('v3')}
        </p>
        <ul>
          <li>{t('modal.about.li1')}</li>
          <li>{t('modal.about.li2')}</li>
          <li>{t('modal.about.li3')}</li>
          <li>{t('modal.about.li4')}</li>
        </ul>
        <p>
          {t('modal.about.p5').split(t('modal.about.linkAbid'))[0]}
          <a href="https://www.worldcubeassociation.org/persons/2024ASHR02" target="_blank" rel="noreferrer">{t('modal.about.linkAbid')}</a>
          {t('modal.about.p5').split(t('modal.about.linkAbid'))[1].split(t('modal.about.linkMatt'))[0]}
          <a href="https://www.worldcubeassociation.org/persons/2023MAOS01" target="_blank" rel="noreferrer">{t('modal.about.linkMatt')}</a>
          {t('modal.about.p5').split(t('modal.about.linkAbid'))[1].split(t('modal.about.linkMatt'))[1]}
        </p>
      </div>
    ) : (
      <div className="modal-article how-to-use">
        <h2>{t('modal.howToUse.title')}</h2>
        <div className="modal-section-title">{t('modal.howToUse.sectionKeyboard')}</div>
        <ul>
          <li><strong>Z</strong>{t('modal.howToUse.shortcut1').split('Z')[1].split('Y')[0]}<strong>Y</strong>{t('modal.howToUse.shortcut1').split('Y')[1]}</li>
          <li><strong>Esc</strong>{t('modal.howToUse.shortcut2').replace('Esc', '')}</li>
          <li><strong>Ctrl + Enter</strong>{t('modal.howToUse.shortcut3').replace('Ctrl + Enter', '')}</li>
          <li><strong>Ctrl + Z</strong>{t('modal.howToUse.shortcut4').split('Ctrl + Z')[1].split('Ctrl + Y')[0]}<strong>Ctrl + Y</strong>{t('modal.howToUse.shortcut4').split('Ctrl + Y')[1]}</li>
          <li><strong>Ctrl + =</strong>{t('modal.howToUse.shortcut5').split('Ctrl + =')[1].split('Ctrl + -')[0]}<strong>Ctrl + -</strong>{t('modal.howToUse.shortcut5').split('Ctrl + -')[1]}</li>
          <li><strong>Ctrl + 0</strong>{t('modal.howToUse.shortcut6').replace('Ctrl + 0', '')}</li>
        </ul>
        <p>
          {t('modal.howToUse.swapHint').split('swap')[0]}
          <strong>swap</strong>
          {t('modal.howToUse.swapHint').split('swap')[1]}
        </p>
        <ul>
          <li><strong>J</strong>{t('modal.howToUse.shortcutJ').split('J')[1].split('F')[0]}<strong>F</strong>{t('modal.howToUse.shortcutJ').split('F')[1]}</li>
          <li><strong>S</strong>{t('modal.howToUse.shortcutS').split('S')[1].split('L')[0]}<strong>L</strong>{t('modal.howToUse.shortcutS').split('L')[1]}</li>
          <li><strong>I</strong>{t('modal.howToUse.shortcutI').split('I')[1].split('K')[0]}<strong>K</strong>{t('modal.howToUse.shortcutI').split('K')[1]}</li>
          <li><strong>H</strong>{t('modal.howToUse.shortcutH').split('H')[1].split('G')[0]}<strong>G</strong>{t('modal.howToUse.shortcutH').split('G')[1]}</li>
          <li><strong>W</strong>{t('modal.howToUse.shortcutW').split('W')[1].split('O')[0]}<strong>O</strong>{t('modal.howToUse.shortcutW').split('O')[1]}</li>
        </ul>
        <div className="modal-section-title">{t('modal.howToUse.sectionScrambleInput')}</div>
        <p>
          {t('modal.howToUse.applyHint').split('Apply')[0]}
          <strong>Apply</strong>
          {t('modal.howToUse.applyHint').split('Apply')[1]}
        </p>
        <p>{t('modal.howToUse.modeHint')}</p>
        <ul>
          <li><strong>{t('modal.howToUse.modeScram').split(':')[0]}</strong>:{t('modal.howToUse.modeScram').split(':').slice(1).join(':')}</li>
          <li><strong>{t('modal.howToUse.modeAlg').split(':')[0]}</strong>:{t('modal.howToUse.modeAlg').split(':').slice(1).join(':')}</li>
          <li><strong>{t('modal.howToUse.modePos').split(':')[0]}</strong>:{t('modal.howToUse.modePos').split(':').slice(1).join(':')}</li>
        </ul>
        <p>
          {t('modal.howToUse.enterHint').split('Shift + Enter')[0].split('Enter')[0]}
          <strong>Enter</strong>
          {t('modal.howToUse.enterHint').split('Shift + Enter')[0].split('Enter')[1]}
          <strong>Shift + Enter</strong>
          {t('modal.howToUse.enterHint').split('Shift + Enter')[1]}
        </p>
        <div className="modal-section-title">{t('modal.howToUse.sectionOptions')}</div>
        <p>{t('modal.howToUse.hoverHint')}</p>
        <ul>
          <li><strong>{t('modal.howToUse.descOutput').split(':')[0]}</strong>:{t('modal.howToUse.descOutput').split(':').slice(1).join(':').split('Solution')[0]}<em>Solution</em>{t('modal.howToUse.descOutput').split('Solution')[1].split('Scramble')[0]}<em>Scramble</em>{t('modal.howToUse.descOutput').split('Scramble')[1]}</li>
          <li><strong>{t('modal.howToUse.descMetric').split(':')[0]}</strong>:{t('modal.howToUse.descMetric').split(':').slice(1).join(':').split('Slice')[0]}<strong>Slice</strong>{t('modal.howToUse.descMetric').split('Slice')[1].split('Move')[0]}<strong>Move</strong>{t('modal.howToUse.descMetric').split('Move')[1].split('Angle')[0]}<strong>Angle</strong>{t('modal.howToUse.descMetric').split('Angle')[1]}</li>
          <li><strong>{t('modal.howToUse.desc2Gen').split(':')[0]}</strong>:{t('modal.howToUse.desc2Gen').split(':').slice(1).join(':')}</li>
          <li>
            <strong>{t('modal.howToUse.descAll').split(':')[0]}</strong>:{t('modal.howToUse.descAll').split(':').slice(1).join(':').split('−/+')[0]}<strong>−</strong>/<strong>+</strong>{t('modal.howToUse.descAll').split('−/+')[1]}
          </li>
          <li><strong>{t('modal.howToUse.descDepths').split(':')[0]}</strong>:{t('modal.howToUse.descDepths').split(':').slice(1).join(':')}</li>
          <li><strong>{t('modal.howToUse.descCubeshape').split(':')[0]}</strong>:{t('modal.howToUse.descCubeshape').split(':').slice(1).join(':')}</li>
          <li><strong>{t('modal.howToUse.descAngle').split(':')[0]}</strong>:{t('modal.howToUse.descAngle').split(':').slice(1).join(':')}</li>
          <li><strong>{t('modal.howToUse.descNormalize').split(':')[0]}</strong>:{t('modal.howToUse.descNormalize').split(':').slice(1).join(':')}</li>
          <li><strong>{t('modal.howToUse.descLimits').split(':')[0]}</strong>:{t('modal.howToUse.descLimits').split(':').slice(1).join(':')}</li>
        </ul>
        <div className="modal-section-title">{t('modal.howToUse.sectionSettings')}</div>
        <p>
          {t('modal.howToUse.descSettingsOpen').split('⋮')[0]}
          <strong className="howto-icon"><Icon name="dots" size={12} /></strong>
          {t('modal.howToUse.descSettingsOpen').split('⋮')[1]}
        </p>
        <ul>
          <li><strong>{t('modal.howToUse.descSmartKarn').split(':')[0]}</strong>:{t('modal.howToUse.descSmartKarn').split(':').slice(1).join(':')}</li>
          <li><strong>{t('modal.howToUse.descAbidNotation').split(':')[0]}</strong>:{t('modal.howToUse.descAbidNotation').split(':').slice(1).join(':')}</li>
          <li><strong>{t('modal.howToUse.descIgnoreTransforms').split(':')[0]}</strong>:{t('modal.howToUse.descIgnoreTransforms').split(':').slice(1).join(':')}</li>
          <li><strong>{t('modal.howToUse.descDebugOutput').split(':')[0]}</strong>:{t('modal.howToUse.descDebugOutput').split(':').slice(1).join(':')}</li>
          <li><strong>{t('modal.howToUse.descUiScale').split(':')[0]}</strong>:{t('modal.howToUse.descUiScale').split(':').slice(1).join(':')}</li>
        </ul>
        <div className="modal-section-title">{t('modal.howToUse.sectionOutput')}</div>
        <p>{t('modal.howToUse.outputIntro')}</p>
        <ul>
          <li><strong>{t('modal.howToUse.descTerminalView').split(':')[0]}</strong>:{t('modal.howToUse.descTerminalView').split(':').slice(1).join(':')}</li>
          <li><strong>{t('modal.howToUse.descTableView').split(':')[0]}</strong>:{t('modal.howToUse.descTableView').split(':').slice(1).join(':')}</li>
        </ul>
        <p>{t('modal.howToUse.descContextMenu')}</p>
        <ul>
          <li><strong>{t('modal.howToUse.descCopyAlg').split('—')[0].trimEnd()}</strong>{' — '}{t('modal.howToUse.descCopyAlg').split('—').slice(1).join('—').trimStart()}</li>
          <li><strong>{t('modal.howToUse.descAddFav').split('—')[0].trimEnd()}</strong>{' — '}{t('modal.howToUse.descAddFav').split('—').slice(1).join('—').trimStart()}</li>
        </ul>
        <p>Other buttons in the terminal area:</p>
        <ul>
          <li><strong>{t('modal.howToUse.outputTools1').split(' / ')[0].split(' ')[0]}</strong>{' / '}<strong>{t('modal.howToUse.outputTools1').split(' / ')[1].split(' ')[0]}</strong>{t('modal.howToUse.outputTools1').split('Normal / Karn')[1]}</li>
          <li><strong>{t('modal.howToUse.outputTools2').split('—')[0].trimEnd()}</strong>{' — '}{t('modal.howToUse.outputTools2').split('—').slice(1).join('—').trimStart()}</li>
          <li><strong>{t('modal.howToUse.outputTools3').split(' / ')[0].trim()}</strong>{' / '}<strong>{t('modal.howToUse.outputTools3').split(' / ')[1].split(' —')[0].trim()}</strong>{' ' + t('modal.howToUse.outputTools3').split('—').slice(1).join('—').trimStart()}</li>
          <li><strong>{t('modal.howToUse.outputTools4').split('—')[0].trimEnd()}</strong>{' — '}{t('modal.howToUse.outputTools4').split('—').slice(1).join('—').trimStart()}</li>
        </ul>
        <p>
          {t('modal.howToUse.descErgo').split('Stay in cubeshape')[0]}
          <strong>Stay in cubeshape</strong>
          {t('modal.howToUse.descErgo').split('Stay in cubeshape')[1].split('ergonomics')[0]}
          <strong>ergonomics</strong>
          {t('modal.howToUse.descErgo').split('Stay in cubeshape')[1].split('ergonomics')[1]}
        </p>
        <div className="modal-section-title">{t('modal.howToUse.sectionFavorites')}</div>
        <p>
          {t('modal.howToUse.favIntro').split('♥')[0]}
          <strong className="howto-icon"><Icon name="heart" size={12} /></strong>
          {t('modal.howToUse.favIntro').split('♥')[1]}
        </p>
        <p>{t('modal.howToUse.favAdd')}</p>
        <p>
          {t('modal.howToUse.favBinId').split('configurations')[0]}
          <strong>configurations</strong>
          {t('modal.howToUse.favBinId').split('configurations')[1]}
        </p>
        <p>{t('modal.howToUse.favInside')}</p>
        <ul>
          <li>{t('modal.howToUse.favApply').split('Apply setup')[0]}<strong>Apply setup</strong>{t('modal.howToUse.favApply').split('Apply setup')[1]}</li>
          <li>{t('modal.howToUse.favRename').split('✏')[0]}<strong>✏</strong>{t('modal.howToUse.favRename').split('✏')[1]}</li>
          <li><strong className="howto-icon"><Icon name="copy" size={12} /></strong>{t('modal.howToUse.favCopy').split('⧉')[1]}</li>
          <li><strong className="howto-icon"><Icon name="trash" size={12} /></strong>{t('modal.howToUse.favDelete').split('🗑')[1]}</li>
          <li>{t('modal.howToUse.favRemove').split('✕')[0]}<strong className="howto-icon"><Icon name="close" size={12} /></strong>{t('modal.howToUse.favRemove').split('✕')[1]}</li>
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
      <div className="modal" onClick={(e) => e.stopPropagation()}>
        <button className="modal-close" onClick={close}>
          <Icon name="close" />
        </button>
        {content}
      </div>
    </div>
  );
}
