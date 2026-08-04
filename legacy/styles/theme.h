#pragma once
#include <QColor>
#include <QString>
#include <QMap>

namespace Theme {
    extern const QMap<QString, QString> COLORS;

    inline QString primaryBg()    { return "#1a1a2e"; }
    inline QString secondaryBg()  { return "#0d1117"; }
    inline QString tertiaryBg()   { return "#2a2a3e"; }
    inline QString darkBg()       { return "#0e0e1c"; }
    inline QString disabledBg()   { return "#1e1e30"; }
    inline QString hoverBg()      { return "#3a3a5e"; }
    inline QString pressedBg()    { return "#1a1a2e"; }

    inline QString textPrimary()   { return "#e0e0e0"; }
    inline QString textSecondary() { return "#aaaaaa"; }
    inline QString textMuted()     { return "#888888"; }
    inline QString textDisabled()  { return "#555555"; }
    inline QString textError()     { return "#ff5555"; }
    inline QString textCyan()      { return "#7fdbff"; }
    inline QString textTerminal()  { return "#7ec8e3"; }
    inline QString textSolution()  { return "#7abfe8"; }

    inline QString borderLight()   { return "#555555"; }
    inline QString borderDark()    { return "#3a3a4e"; }
    inline QString borderAccent()  { return "#777777"; }
    inline QString borderGroup()   { return "#444444"; }
    inline QString borderBottom()  { return "#2a2a4a"; }

    inline QString checkboxBg()          { return "#2a2a3e"; }
    inline QString checkboxCheckBlue()   { return "#4a90d9"; }
    inline QString checkboxCheckOrange() { return "#d97a4a"; }
    inline QString checkboxBorder()      { return "#666666"; }

    inline QString buttonBg()            { return "#2a2a3e"; }
    inline QString buttonBorder()        { return "#555555"; }
    inline QString buttonText()          { return "#dddddd"; }
    inline QString buttonSecondaryText() { return "#aaaaaa"; }

    inline QString buttonSolveBg()     { return "#1a6b3c"; }
    inline QString buttonSolveBorder() { return "#2db570"; }
    inline QString buttonSolveHover()  { return "#227a47"; }

    inline QString buttonResetBg()     { return "#2a2a3e"; }
    inline QString buttonResetBorder() { return "#555555"; }
    inline QString buttonResetHover()  { return "#3a3a5e"; }

    inline QString buttonStopBg()     { return "#3d1616"; }
    inline QString buttonStopBorder() { return "#7a2e2e"; }
    inline QString buttonStopHover()  { return "#4d1e1e"; }
    inline QString buttonStopText()   { return "#c89898"; }

    inline QString buttonUtilBg()          { return "#1e1e30"; }
    inline QString buttonUtilBorder()      { return "#3a3a5e"; }
    inline QString buttonUtilText()        { return "#7a7aaa"; }
    inline QString buttonUtilHoverBg()     { return "#2a2a4a"; }
    inline QString buttonUtilHoverBorder() { return "#5a5a8a"; }
    inline QString buttonUtilHoverText()   { return "#b0b0dd"; }

    inline QString buttonAboutBg()          { return "#2a2a3e"; }
    inline QString buttonAboutBorder()      { return "#4a4a6a"; }
    inline QString buttonAboutText()        { return "#9090bb"; }
    inline QString buttonAboutHoverBg()     { return "#3a3a5e"; }
    inline QString buttonAboutHoverBorder() { return "#7a7aaa"; }
    inline QString buttonAboutHoverText()   { return "#e0e0ff"; }

    inline QString topBarBg()     { return "#0f0e2d"; }

    inline QString inputPanelBg()     { return "#0d1a2e"; }
    inline QString inputModeBg()      { return "#0d1a2e"; }
    inline QString inputModeBorder()  { return "#1e3a5a"; }
    inline QString inputModeHover()   { return "#142440"; }
    inline QString inputModeText()    { return "#60a8e0"; }
    inline QString inputArrowBg()     { return "#0d1a2e"; }
    inline QString inputArrowBorder() { return "#1e3a5a"; }
    inline QString inputArrowHover()  { return "#142440"; }
    inline QString inputApplyBg()        { return "#0f3060"; }
    inline QString inputApplyBorder()    { return "#2060b0"; }
    inline QString inputApplyHover()     { return "#1a4a8a"; }
    inline QString inputApplyText()      { return "#7abfe8"; }
    inline QString inputApplyHoverText() { return "#ffffff"; }
    inline QString inputFieldBg()     { return "#060e1a"; }
    inline QString inputFieldBorder() { return "#1a3050"; }
    inline QString inputFieldText()   { return "#5a8ab0"; }

    inline QString progressBg()   { return "#2a2a3e"; }
    inline QString progressFill() { return "#4a90d9"; }
    inline QString statusText()   { return "#888888"; }

    inline QString scrollbarBg()     { return "#0d1117"; }
    inline QString scrollbarHandle() { return "#4a4a6e"; }
    inline QString scrollbarHover()  { return "#6a6aae"; }

    inline QString tableBg()           { return "#0d1117"; }
    inline QString tableBorder()       { return "#444444"; }
    inline QString tableHeaderBg()     { return "#1a1a2e"; }
    inline QString tableHeaderText()   { return "#7a9ab8"; }
    inline QString tableSelectedBg()   { return "#1e3a5a"; }
    inline QString tableSelectedText() { return "#ffffff"; }

    inline QString rowAltDark()  { return "#0d1117"; }
    inline QString rowAltLight() { return "#131c28"; }

    inline QString tooltipBg()     { return "#252540"; }
    inline QString tooltipText()   { return "#d8d8f0"; }
    inline QString tooltipBorder() { return "#5a5a8a"; }

    inline QString sidebarBg()     { return "#13132a"; }
    inline QString sidebarBorder() { return "#2a2a4a"; }
    inline QString modalBg()       { return "#1e1e34"; }
    inline QString modalBorder()   { return "#3a3a5e"; }
    inline QString modalOverlay()  { return "rgba(0,0,0,170)"; }

    inline QString depthsInactiveColor()  { return "#666666"; }
    inline QString depthsInactiveBg()     { return "#1e1e30"; }
    inline QString depthsInactiveBorder() { return "#3a3a4e"; }
    inline QString ergoAltTextDark()      { return "#cdcdcd"; }
    inline QString ergoAltMetaDark()      { return "#969696"; }

    inline QString fadingTooltipBg()               { return "#23233a"; }
    inline QString fadingTooltipBorder()           { return "#55557a"; }
    inline QString fadingTooltipText()             { return "#e0e0e0"; }
    inline QString cubeWithResetBorder()           { return "#27274d"; }
    inline QString solutionAltLight()              { return "#cbcbcb"; }
    inline QString solutionPrimary()               { return "#7abfe8"; }
    inline QString menuBg()                        { return "#1a1a2e"; }
    inline QString menuBorder()                    { return "#3a3a5e"; }
    inline QString menuItemSelected()              { return "#3a3a5e"; }
    inline QString menuItemChecked()               { return "#2db570"; }
    inline QString linkColor()                     { return "#7abfe8"; }
    inline QString radioPillBg()           { return "rgba(255,255,255,0.11)"; }
    inline QString radioBtnText()          { return "rgba(210,215,240,0.70)"; }
    inline QString radioBtnCheckedBg()     { return "rgba(78,112,175,0.55)"; }
    inline QString radioBtnCheckedText()   { return "#ccd6f0"; }
    inline QString radioBtnCheckedBorder() { return "transparent"; }
    inline QString radioBtnHoverBg()       { return "rgba(255,255,255,0.09)"; }
    inline QString stopSolveDisabledBg()           { return "#333333"; }
    inline QString stopSolveDisabledBorder()       { return "#444444"; }
    inline QString stopSolveDisabledText()         { return "#666666"; }

    inline QString cubeTopFace()   { return "#333333"; }
    inline QString cubeBotFace()   { return "#ffffff"; }
    inline QString cubeRed()       { return "#ff0000"; }
    inline QString cubeBlue()      { return "#0000ff"; }
    inline QString cubeOrange()    { return "#ff8600"; }
    inline QString cubeGreen()     { return "#00ff00"; }
    inline QString cubePartial()   { return "#888888"; }
    inline QString cubeBorder()    { return "#000000"; }
    inline QString cubeSelection() { return "#ffff00"; }
    inline QString canvasBg()      { return "#1a1a2e"; }

    QColor getColor(const QString& colorName);
    QString buildStyleSheet();
}
