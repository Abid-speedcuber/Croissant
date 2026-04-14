#pragma once
#include <QColor>
#include <QString>
#include <QMap>

namespace Theme {
    extern const QMap<QString, QString> COLORS;

    // ========== BACKGROUNDS ==========
    inline QString primaryBg(bool light = false)    { return light ? "#ffffff" : "#1a1a2e"; }
    inline QString secondaryBg(bool light = false)  { return light ? "#f0f7ff" : "#0d1117"; }
    inline QString tertiaryBg(bool light = false)   { return light ? "#e4f0fc" : "#2a2a3e"; }
    inline QString darkBg(bool light = false)       { return light ? "#d8eaf8" : "#0e0e1c"; }
    inline QString disabledBg(bool light = false)   { return light ? "#eef5fc" : "#1e1e30"; }
    inline QString hoverBg(bool light = false)      { return light ? "#cce0f5" : "#3a3a5e"; }
    inline QString pressedBg(bool light = false)    { return light ? "#b0d0ee" : "#1a1a2e"; }

    // ========== TEXT ==========
    inline QString textPrimary(bool light = false)   { return light ? "#0a2a4a" : "#e0e0e0"; }
    inline QString textSecondary(bool light = false) { return light ? "#1a5080" : "#aaaaaa"; }
    inline QString textMuted(bool light = false)     { return light ? "#5a8ab0" : "#888888"; }
    inline QString textDisabled(bool light = false)  { return light ? "#8ab0cc" : "#555555"; }
    inline QString textError(bool light = false)     { return light ? "#cc2222" : "#ff5555"; }
    inline QString textCyan(bool light = false)      { return light ? "#0e60b0" : "#7fdbff"; }
    inline QString textTerminal(bool light = false)  { return light ? "#0e5c90" : "#7ec8e3"; }
    inline QString textSolution(bool light = false)  { return light ? "#0e60b0" : "#7abfe8"; }

    // ========== BORDERS ==========
    inline QString borderLight(bool light = false)   { return light ? "#88bce4" : "#555555"; }
    inline QString borderDark(bool light = false)    { return light ? "#b0d4ee" : "#3a3a4e"; }
    inline QString borderAccent(bool light = false)  { return light ? "#50a0d8" : "#777777"; }
    inline QString borderGroup(bool light = false)   { return light ? "#88bce4" : "#444444"; }
    inline QString borderBottom(bool light = false)  { return light ? "#a0cce8" : "#2a2a4a"; }

    // ========== CHECKBOXES ==========
    inline QString checkboxBg(bool light = false)          { return light ? "#e4f0fc" : "#2a2a3e"; }
    inline QString checkboxCheckBlue(bool light = false)   { return light ? "#1878d0" : "#4a90d9"; }
    inline QString checkboxCheckOrange(bool light = false) { return light ? "#c87030" : "#d97a4a"; }
    inline QString checkboxBorder(bool light = false)      { return light ? "#70aad8" : "#666666"; }

    // ========== BUTTONS ==========
    inline QString buttonBg(bool light = false)            { return light ? "#deeefa" : "#2a2a3e"; }
    inline QString buttonBorder(bool light = false)        { return light ? "#70aad8" : "#555555"; }
    inline QString buttonText(bool light = false)          { return light ? "#0a2a4a" : "#dddddd"; }
    inline QString buttonSecondaryText(bool light = false) { return light ? "#1a5080" : "#aaaaaa"; }

    inline QString buttonSolveBg(bool light = false)     { return light ? "#1a6b3c" : "#1a6b3c"; }
    inline QString buttonSolveBorder(bool light = false) { return light ? "#2db570" : "#2db570"; }
    inline QString buttonSolveHover(bool light = false)  { return light ? "#227a47" : "#227a47"; }

    inline QString buttonResetBg(bool light = false)     { return light ? "#deeefa" : "#2a2a3e"; }
    inline QString buttonResetBorder(bool light = false) { return light ? "#70aad8" : "#555555"; }
    inline QString buttonResetHover(bool light = false)  { return light ? "#cce0f5" : "#3a3a5e"; }

    inline QString buttonStopBg(bool light = false)     { return light ? "#fce8e8" : "#3d1616"; }
    inline QString buttonStopBorder(bool light = false) { return light ? "#e08888" : "#7a2e2e"; }
    inline QString buttonStopHover(bool light = false)  { return light ? "#f8d0d0" : "#4d1e1e"; }
    inline QString buttonStopText(bool light = false)   { return light ? "#8a1a1a" : "#c89898"; }

    inline QString buttonUtilBg(bool light = false)          { return light ? "#e4f0fc" : "#1e1e30"; }
    inline QString buttonUtilBorder(bool light = false)      { return light ? "#88bce4" : "#3a3a5e"; }
    inline QString buttonUtilText(bool light = false)        { return light ? "#2a6090" : "#7a7aaa"; }
    inline QString buttonUtilHoverBg(bool light = false)     { return light ? "#cce0f5" : "#2a2a4a"; }
    inline QString buttonUtilHoverBorder(bool light = false) { return light ? "#50a0d8" : "#5a5a8a"; }
    inline QString buttonUtilHoverText(bool light = false)   { return light ? "#0a2a4a" : "#b0b0dd"; }

    inline QString buttonAboutBg(bool light = false)          { return light ? "#deeefa" : "#2a2a3e"; }
    inline QString buttonAboutBorder(bool light = false)      { return light ? "#88bce4" : "#4a4a6a"; }
    inline QString buttonAboutText(bool light = false)        { return light ? "#1a5080" : "#9090bb"; }
    inline QString buttonAboutHoverBg(bool light = false)     { return light ? "#cce0f5" : "#3a3a5e"; }
    inline QString buttonAboutHoverBorder(bool light = false) { return light ? "#50a0d8" : "#7a7aaa"; }
    inline QString buttonAboutHoverText(bool light = false)   { return light ? "#0a2a4a" : "#e0e0ff"; }

    // ========== TOP BAR ============
    inline QString topBarBg(bool light = false)     { return light ? "#a5d3fd" : "#0f0e2d"; }

    // ========== INPUT BAR ==========
    inline QString inputPanelBg(bool light = false)     { return light ? "#d2f7ff" : "#0d1a2e"; }

    inline QString inputModeBg(bool light = false)    { return light ? "#d8eaf8" : "#0d1a2e"; }
    inline QString inputModeBorder(bool light = false){ return light ? "#70aad8" : "#1e3a5a"; }
    inline QString inputModeHover(bool light = false) { return light ? "#c0d8f0" : "#142440"; }
    inline QString inputModeText(bool light = false)  { return light ? "#0e60b0" : "#60a8e0"; }

    inline QString inputArrowBg(bool light = false)    { return light ? "#d8eaf8" : "#0d1a2e"; }
    inline QString inputArrowBorder(bool light = false){ return light ? "#70aad8" : "#1e3a5a"; }
    inline QString inputArrowHover(bool light = false) { return light ? "#c0d8f0" : "#142440"; }

    inline QString inputApplyBg(bool light = false)        { return light ? "#1878d0" : "#0f3060"; }
    inline QString inputApplyBorder(bool light = false)    { return light ? "#0e60b0" : "#2060b0"; }
    inline QString inputApplyHover(bool light = false)     { return light ? "#1468b8" : "#1a4a8a"; }
    inline QString inputApplyText(bool light = false)      { return light ? "#ffffff" : "#7abfe8"; }
    inline QString inputApplyHoverText(bool light = false) { return light ? "#ffffff" : "#ffffff"; }

    inline QString inputFieldBg(bool light = false)     { return light ? "#f5faff" : "#060e1a"; }
    inline QString inputFieldBorder(bool light = false) { return light ? "#70aad8" : "#1a3050"; }
    inline QString inputFieldText(bool light = false)   { return light ? "#0a2a4a" : "#5a8ab0"; }

    // ========== PROGRESS / STATUS ==========
    inline QString progressBg(bool light = false)   { return light ? "#c8e0f4" : "#2a2a3e"; }
    inline QString progressFill(bool light = false) { return light ? "#1878d0" : "#4a90d9"; }
    inline QString statusText(bool light = false)   { return light ? "#5a8ab0" : "#888888"; }

    // ========== SCROLLBARS ==========
    inline QString scrollbarBg(bool light = false)     { return light ? "#e4f0fc" : "#0d1117"; }
    inline QString scrollbarHandle(bool light = false) { return light ? "#88bce4" : "#4a4a6e"; }
    inline QString scrollbarHover(bool light = false)  { return light ? "#50a0d8" : "#6a6aae"; }

    // ========== TABLE ==========
    inline QString tableBg(bool light = false)           { return light ? "#f0f7ff" : "#0d1117"; }
    inline QString tableBorder(bool light = false)       { return light ? "#88bce4" : "#444444"; }
    inline QString tableHeaderBg(bool light = false)     { return light ? "#d8eaf8" : "#1a1a2e"; }
    inline QString tableHeaderText(bool light = false)   { return light ? "#1a5080" : "#7a9ab8"; }
    inline QString tableSelectedBg(bool light = false)   { return light ? "#1878d0" : "#1e3a5a"; }
    inline QString tableSelectedText(bool light = false) { return light ? "#ffffff" : "#ffffff"; }

    // ========== ALTERNATING ROWS ==========
    inline QString rowAltDark(bool light = false)  { return light ? "#e8f3fc" : "#0d1117"; }
    inline QString rowAltLight(bool light = false) { return light ? "#f5faff" : "#131c28"; }

    // ========== TOOLTIPS ==========
    inline QString tooltipBg(bool light = false)     { return light ? "#0a2a4a" : "#252540"; }
    inline QString tooltipText(bool light = false)   { return light ? "#e8f4ff" : "#d8d8f0"; }
    inline QString tooltipBorder(bool light = false) { return light ? "#3a7ab8" : "#5a5a8a"; }

    // ========== SIDEBAR / MODAL ==========
    inline QString sidebarBg(bool light = false)     { return light ? "#f0f7ff" : "#13132a"; }
    inline QString sidebarBorder(bool light = false) { return light ? "#a0cce8" : "#2a2a4a"; }
    inline QString modalBg(bool light = false)       { return light ? "#ffffff" : "#1e1e34"; }
    inline QString modalBorder(bool light = false)   { return light ? "#88bce4" : "#3a3a5e"; }
    inline QString modalOverlay(bool light = false)  { return light ? "rgba(0,40,80,100)" : "rgba(0,0,0,170)"; }

    // ========== INLINE DYNAMIC ==========
    inline QString depthsInactiveColor(bool light = false)  { return light ? "#5a8ab0" : "#666666"; }
    inline QString depthsInactiveBg(bool light = false)     { return light ? "#eef5fc" : "#1e1e30"; }
    inline QString depthsInactiveBorder(bool light = false) { return light ? "#88bce4" : "#3a3a4e"; }
    inline QString ergoAltTextDark(bool light = false)      { return light ? "#0a2a4a" : "#cdcdcd"; }
    inline QString ergoAltMetaDark(bool light = false)      { return light ? "#1a5080" : "#969696"; }

    // ========== STRAY COLORS (formerly hardcoded in mainwindow.cpp) ==========
    inline QString fadingTooltipBg()               { return "#23233a"; }
    inline QString fadingTooltipBorder()           { return "#55557a"; }
    inline QString fadingTooltipText()             { return "#e0e0e0"; }
    inline QString cubeWithResetBorder()           { return "#27274d"; }
    inline QString solutionAltLight(bool light = false) { return light ? "#1a6b3c" : "#cbcbcb"; }
    inline QString solutionPrimary(bool light = false)  { return light ? "#1060b0" : "#7abfe8"; }
    inline QString menuBg()                        { return "#1a1a2e"; }
    inline QString menuBorder()                    { return "#3a3a5e"; }
    inline QString menuItemSelected()              { return "#3a3a5e"; }
    inline QString menuItemChecked()               { return "#2db570"; }
    inline QString linkColor()                     { return "#7abfe8"; }
    inline QString stopSolveDisabledBg()           { return "#333333"; }
    inline QString stopSolveDisabledBorder()       { return "#444444"; }
    inline QString stopSolveDisabledText()         { return "#666666"; }

    // ========== CUBE/ WIDGET ==========
    inline QString cubeTopFace(bool light = false)  { return "#333333"; }
    inline QString cubeBotFace(bool light = false)  { return "#ffffff"; }
    inline QString cubeRed(bool light = false)      { return "#ff0000"; }
    inline QString cubeBlue(bool light = false)     { return "#0000ff"; }
    inline QString cubeOrange(bool light = false)   { return "#ff8600"; }
    inline QString cubeGreen(bool light = false)    { return "#00ff00"; }
    inline QString cubePartial(bool light = false)  { return "#888888"; }
    inline QString cubeBorder(bool light = false)   { return "#000000"; }
    inline QString cubeSelection(bool light = false){ return light ? "#1878d0" : "#ffff00"; }
    inline QString canvasBg(bool light = false)     { return light ? "#ffffff" : "#1a1a2e"; }

    QColor getColor(const QString& colorName, bool lightTheme = false);
    QString buildStyleSheet(bool lightTheme = false);
}