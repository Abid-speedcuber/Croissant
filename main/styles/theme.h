#pragma once
#include <QColor>
#include <QString>
#include <QMap>

// ============================================================
// THEME.H - CSS-like Color System for the entire application
// ============================================================
// Colors are organized like CSS variables (--var-name format)
// Use Theme::getColor() to access colors in C++
// Colors are automatically available in QSS via --var-name
// ============================================================

namespace Theme {
    // CSS-like variable colors - organized by category
    
    // These will be injected into QSS as --var-name variables
    extern const QMap<QString, QString> COLORS;

    // ========== DARK MODE / PRIMARY BACKGROUND COLORS ==========
    inline QString primaryBg(bool light = false) { return light ? "#f0f0f8" : "#1a1a2e"; }
    inline QString secondaryBg(bool light = false) { return light ? "#e0e0f0" : "#0d1117"; }
    inline QString tertiaryBg(bool light = false) { return light ? "#d8d8ee" : "#2a2a3e"; }
    inline QString darkBg(bool light = false) { return light ? "#c8c8e0" : "#0e0e1c"; }
    inline QString disabledBg(bool light = false) { return light ? "#e8e8f4" : "#1e1e30"; }
    inline QString hoverBg(bool light = false) { return light ? "#d0d0e8" : "#3a3a5e"; }
    inline QString pressedBg(bool light = false) { return light ? "#c0c0d8" : "#1a1a2e"; }

    // ========== TEXT / FOREGROUND COLORS ==========
    inline QString textPrimary(bool light = false) { return light ? "#1a1a3e" : "#e0e0e0"; }
    inline QString textSecondary(bool light = false) { return light ? "#444466" : "#aaa"; }
    inline QString textMuted(bool light = false) { return light ? "#888" : "#888"; }
    inline QString textDisabled(bool light = false) { return light ? "#aaa" : "#555"; }
    inline QString textError(bool light = false) { return light ? "#cc2222" : "#ff5555"; }
    inline QString textCyan(bool light = false) { return light ? "#1a5fa8" : "#7fdbff"; }
    inline QString textTerminal(bool light = false) { return light ? "#1a5080" : "#7ec8e3"; }
    inline QString textSolution(bool light = false) { return light ? "#1a5fa8" : "#7abfe8"; }

    // ========== BORDER / OUTLINE COLORS ==========
    inline QString borderLight(bool light = false) { return light ? "#aaa" : "#555"; }
    inline QString borderDark(bool light = false) { return light ? "#c8c8de" : "#3a3a4e"; }
    inline QString borderAccent(bool light = false) { return light ? "#888" : "#777"; }
    inline QString borderGroup(bool light = false) { return light ? "#aaa" : "#444"; }
    inline QString borderBottom(bool light = false) { return light ? "#c0c0da" : "#2a2a4a"; }

    // ========== CHECKBOX / INDICATOR COLORS ==========
    inline QString checkboxBg(bool light = false) { return light ? "#d8d8ee" : "#2a2a3e"; }
    inline QString checkboxCheckBlue(bool light = false) { return light ? "#4a80c4" : "#4a90d9"; }
    inline QString checkboxCheckOrange(bool light = false) { return light ? "#c87030" : "#d97a4a"; }
    inline QString checkboxBorder(bool light = false) { return light ? "#aaa" : "#666"; }

    // ========== BUTTON COLORS ==========
    inline QString buttonBg(bool light = false) { return light ? "#d8d8ee" : "#2a2a3e"; }
    inline QString buttonBorder(bool light = false) { return light ? "#aaa" : "#555"; }
    inline QString buttonText(bool light = false) { return light ? "#1a1a3e" : "#ddd"; }
    inline QString buttonSecondaryText(bool light = false) { return light ? "#444466" : "#aaa"; }

    // Solve Button (green)
    inline QString buttonSolveBg(bool light = false) { return light ? "#1a6b3c" : "#1a6b3c"; }
    inline QString buttonSolveBorder(bool light = false) { return light ? "#2db570" : "#2db570"; }
    inline QString buttonSolveHover(bool light = false) { return light ? "#227a47" : "#227a47"; }

    // Input Bar - Mode Toggle
    inline QString inputModeBg(bool light = false) { return light ? "#d8e8f8" : "#0d1a2e"; }
    inline QString inputModeBorder(bool light = false) { return light ? "#6090c0" : "#1e3a5a"; }
    inline QString inputModeHover(bool light = false) { return light ? "#c4d8f0" : "#142440"; }
    inline QString inputModeText(bool light = false) { return light ? "#1a5fa8" : "#60a8e0"; }

    // Input Bar - Dropdown Arrow
    inline QString inputArrowBg(bool light = false) { return light ? "#d8e8f8" : "#0d1a2e"; }
    inline QString inputArrowBorder(bool light = false) { return light ? "#6090c0" : "#1e3a5a"; }
    inline QString inputArrowHover(bool light = false) { return light ? "#c4d8f0" : "#142440"; }

    // Input Bar - Apply Button
    inline QString inputApplyBg(bool light = false) { return light ? "#1a5fa8" : "#0f3060"; }
    inline QString inputApplyBorder(bool light = false) { return light ? "#0e4a8a" : "#2060b0"; }
    inline QString inputApplyHover(bool light = false) { return light ? "#154d90" : "#1a4a8a"; }
    inline QString inputApplyText(bool light = false) { return light ? "#ffffff" : "#7abfe8"; }
    inline QString inputApplyHoverText(bool light = false) { return light ? "#ffffff" : "#ffffff"; }

    // Input Bar - Field
    inline QString inputFieldBg(bool light = false) { return light ? "#eaf2fc" : "#060e1a"; }
    inline QString inputFieldBorder(bool light = false) { return light ? "#7aaad8" : "#1a3050"; }
    inline QString inputFieldText(bool light = false) { return light ? "#1a3a5a" : "#5a8ab0"; }
    inline QString inputPanelBg(bool light = false) { return light ? "#60a8e0" : "#60a8e0"; }

    // Reset Button
    inline QString buttonResetBg(bool light = false) { return light ? "#d8d8ee" : "#2a2a3e"; }
    inline QString buttonResetBorder(bool light = false) { return light ? "#aaa" : "#555"; }
    inline QString buttonResetHover(bool light = false) { return light ? "#c8c8de" : "#3a3a5e"; }

    // Stop Button (muted red)
    inline QString buttonStopBg(bool light = false) { return light ? "#d8d8ee" : "#3d1616"; }
    inline QString buttonStopBorder(bool light = false) { return light ? "#aaa" : "#7a2e2e"; }
    inline QString buttonStopHover(bool light = false) { return light ? "#c8c8de" : "#4d1e1e"; }
    inline QString buttonStopText(bool light = false) { return light ? "#1a1a3e" : "#c89898"; }

    // Utility Buttons
    inline QString buttonUtilBg(bool light = false) { return light ? "#d8d8ee" : "#1e1e30"; }
    inline QString buttonUtilBorder(bool light = false) { return light ? "#aaa" : "#3a3a5e"; }
    inline QString buttonUtilText(bool light = false) { return light ? "#444466" : "#7a7aaa"; }
    inline QString buttonUtilHoverBg(bool light = false) { return light ? "#c8c8de" : "#2a2a4a"; }
    inline QString buttonUtilHoverBorder(bool light = false) { return light ? "#888" : "#5a5a8a"; }
    inline QString buttonUtilHoverText(bool light = false) { return light ? "#1a1a3e" : "#b0b0dd"; }

    // About Button
    inline QString buttonAboutBg(bool light = false) { return light ? "#d8d8ee" : "#2a2a3e"; }
    inline QString buttonAboutBorder(bool light = false) { return light ? "#aaa" : "#4a4a6a"; }
    inline QString buttonAboutText(bool light = false) { return light ? "#444466" : "#9090bb"; }
    inline QString buttonAboutHoverBg(bool light = false) { return light ? "#c8c8de" : "#3a3a5e"; }
    inline QString buttonAboutHoverBorder(bool light = false) { return light ? "#888" : "#7a7aaa"; }
    inline QString buttonAboutHoverText(bool light = false) { return light ? "#1a1a3e" : "#e0e0ff"; }

    // ========== PROGRESS / STATUS COLORS ==========
    inline QString progressBg(bool light = false) { return light ? "#c8c8e0" : "#2a2a3e"; }
    inline QString progressFill(bool light = false) { return light ? "#4a80c4" : "#4a90d9"; }
    inline QString statusText(bool light = false) { return light ? "#888" : "#888"; }

    // ========== SCROLLBAR COLORS ==========
    inline QString scrollbarBg(bool light = false) { return light ? "#dcdcec" : "#0d1117"; }
    inline QString scrollbarHandle(bool light = false) { return light ? "#a0a0c0" : "#4a4a6e"; }
    inline QString scrollbarHover(bool light = false) { return light ? "#7878a8" : "#6a6aae"; }

    // ========== TABLE COLORS ==========
    inline QString tableBg(bool light = false) { return light ? "#e8e8f8" : "#0d1117"; }
    inline QString tableBorder(bool light = false) { return light ? "#bbb" : "#444"; }
    inline QString tableHeaderBg(bool light = false) { return light ? "#d8d8ee" : "#1a1a2e"; }
    inline QString tableHeaderText(bool light = false) { return light ? "#3a5a8a" : "#7a9ab8"; }
    inline QString tableSelectedBg(bool light = false) { return light ? "#4a80c4" : "#1e3a5a"; }
    inline QString tableSelectedText(bool light = false) { return light ? "#ffffff" : "#ffffff"; }

    // ========== ALTERNATING ROW COLORS ==========
    inline QString rowAltDark(bool light = false) { return light ? "#e0e8f8" : "#0d1117"; }
    inline QString rowAltLight(bool light = false) { return light ? "#eceef8" : "#131c28"; }

    // ========== TOOLTIP COLORS ==========
    inline QString tooltipBg(bool light = false) { return light ? "#2a2a50" : "#252540"; }
    inline QString tooltipText(bool light = false) { return light ? "#e8e8ff" : "#d8d8f0"; }
    inline QString tooltipBorder(bool light = false) { return light ? "#5a5a8a" : "#5a5a8a"; }

    // ========== CUBE COLORS ==========
    inline QString cubeTopFace(bool light = false) { return "#333333"; }
    inline QString cubeBotFace(bool light = false) { return "#ffffff"; }
    inline QString cubeRed(bool light = false) { return "#ff0000"; }
    inline QString cubeBlue(bool light = false) { return "#0000ff"; }
    inline QString cubeOrange(bool light = false) { return "#ff8600"; }
    inline QString cubeGreen(bool light = false) { return "#00ff00"; }
    inline QString cubePartial(bool light = false) { return "#888888"; }
    inline QString cubeBorder(bool light = false) { return "#000000"; }
    inline QString cubeSelection(bool light = false) { return "#ffff00"; }
    inline QString canvasBg(bool light = false) { return light ? "#f0f0f8" : "#1a1a2e"; }

    // ========== SIDEBAR / MODAL COLORS ==========
    inline QString sidebarBg(bool light = false) { return light ? "#e8e8f8" : "#13132a"; }
    inline QString sidebarBorder(bool light = false) { return light ? "#c0c0da" : "#2a2a4a"; }
    inline QString modalBg(bool light = false) { return light ? "#d8d8ee" : "#1e1e34"; }
    inline QString modalBorder(bool light = false) { return light ? "#a0a0c0" : "#3a3a5e"; }
    inline QString modalOverlay(bool light = false) { return light ? "rgba(0,0,0,100)" : "rgba(0,0,0,170)"; }

    // ========== INLINE / RUNTIME DYNAMIC COLORS ==========
    inline QString depthsInactiveColor(bool light = false) { return light ? "#888" : "#666"; }
    inline QString depthsInactiveBg(bool light = false) { return light ? "#d8d8ee" : "#1e1e30"; }
    inline QString depthsInactiveBorder(bool light = false) { return light ? "#aaa" : "#3a3a4e"; }
    inline QString ergoAltTextDark(bool light = false) { return light ? "#333333" : "#cdcdcd"; }
    inline QString ergoAltMetaDark(bool light = false) { return light ? "#555555" : "#969696"; }

    // ========== UTILITY FUNCTIONS ==========
    // Get a QColor from color name (e.g., "primary-bg", "button-text")
    QColor getColor(const QString& colorName, bool lightTheme = false);
    
    // Build stylesheet with CSS variables
    QString buildStyleSheet(bool lightTheme = false);
}
