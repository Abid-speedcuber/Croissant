#pragma once
#include <QColor>

// ============================================================
// THEME.H - Centralized Color Palette for the entire application
// ============================================================
// All colors used in the UI, cube visualization, and styling
// are defined here. Modify this file to experiment with themes!
// ============================================================

namespace Theme {
    // ========== DARK MODE / PRIMARY BACKGROUND COLORS ==========
    static constexpr const char* PRIMARY_BG           = "#1a1a2e";  // Main window background
    static constexpr const char* SECONDARY_BG         = "#0d1117";  // Code/terminal background
    static constexpr const char* TERTIARY_BG          = "#2a2a3e";  // Buttons, inputs, components
    static constexpr const char* DARK_BG              = "#0e0e1c";  // Top bar, very dark areas
    static constexpr const char* DISABLED_BG          = "#1e1e30";  // Disabled component background
    static constexpr const char* HOVER_BG             = "#3a3a5e";  // Hover states
    static constexpr const char* PRESSED_BG           = "#1a1a2e";  // Pressed button state

    // ========== TEXT / FOREGROUND COLORS ==========
    static constexpr const char* TEXT_PRIMARY         = "#e0e0e0";  // Main text, labels
    static constexpr const char* TEXT_SECONDARY       = "#aaa";     // Secondary text, less important
    static constexpr const char* TEXT_MUTED           = "#888";     // Very muted text (status, hints)
    static constexpr const char* TEXT_DARK            = "#ddd";     // Slightly lighter than primary
    static constexpr const char* TEXT_DISABLED        = "#555";     // Disabled text
    static constexpr const char* TEXT_ERROR           = "#ff5555";  // Error messages
    static constexpr const char* TEXT_CYAN            = "#7fdbff";  // Monospace input (command line)
    static constexpr const char* TEXT_TERMINAL        = "#7ec8e3";  // Terminal output
    static constexpr const char* TEXT_SOLUTION        = "#7abfe8";  // Solution highlights

    // ========== BORDER / OUTLINE COLORS ==========
    static constexpr const char* BORDER_LIGHT         = "#555";     // Standard borders
    static constexpr const char* BORDER_DARK          = "#3a3a4e";  // Subtle borders
    static constexpr const char* BORDER_ACCENT        = "#777";     // Brighter borders on hover
    static constexpr const char* BORDER_GROUP         = "#444";     // GroupBox border
    static constexpr const char* BORDER_HEADER        = "#2a2a4a";  // Table header border
    static constexpr const char* BORDER_BOTTOM        = "#2a2a4a";  // Bottom bar/divider

    // ========== CHECKBOX / INDICATOR COLORS ==========
    static constexpr const char* CHECKBOX_BG          = "#2a2a3e";  // Unchecked checkbox background
    static constexpr const char* CHECKBOX_CHECK_BLUE  = "#4a90d9";  // Standard checked checkbox (blue)
    static constexpr const char* CHECKBOX_CHECK_ORANGE= "#d97a4a";  // Special checkbox (rank ergo - orange)
    static constexpr const char* CHECKBOX_BORDER      = "#666";     // Checkbox border

    // ========== BUTTON COLORS ==========
    static constexpr const char* BUTTON_BG            = "#2a2a3e";  // Default button background
    static constexpr const char* BUTTON_BORDER        = "#555";     // Default button border
    static constexpr const char* BUTTON_TEXT          = "#ddd";     // Default button text
    static constexpr const char* BUTTON_SECONDARY_TEXT= "#aaa";     // Secondary button text

    // Solve Button (green)
    static constexpr const char* BUTTON_SOLVE_BG      = "#1a6b3c";  // Solve button background
    static constexpr const char* BUTTON_SOLVE_BORDER  = "#2db570";  // Solve button border
    static constexpr const char* BUTTON_SOLVE_HOVER   = "#227a47";  // Solve button hover

    // Reset Button (red - alarming)
    static constexpr const char* BUTTON_RESET_BG      = "#6b1a1a";  // Reset button background (dark red)
    static constexpr const char* BUTTON_RESET_BORDER  = "#b52d2d";  // Reset button border (lighter red)
    static constexpr const char* BUTTON_RESET_HOVER   = "#7a2020";  // Reset button hover

    // Stop Button (muted red)
    static constexpr const char* BUTTON_STOP_BG       = "#3d1616";  // Stop button background
    static constexpr const char* BUTTON_STOP_BORDER   = "#7a2e2e";  // Stop button border
    static constexpr const char* BUTTON_STOP_HOVER    = "#4d1e1e";  // Stop button hover
    static constexpr const char* BUTTON_STOP_TEXT     = "#c89898";  // Stop button text

    // Utility Buttons
    static constexpr const char* BUTTON_UTIL_BG       = "#1e1e30";  // Utility button background
    static constexpr const char* BUTTON_UTIL_BORDER   = "#3a3a5e";  // Utility button border
    static constexpr const char* BUTTON_UTIL_TEXT     = "#7a7aaa";  // Utility button text
    static constexpr const char* BUTTON_UTIL_HOVER_BG = "#2a2a4a";  // Utility button hover bg
    static constexpr const char* BUTTON_UTIL_HOVER_BORDER = "#5a5a8a";  // Utility button hover border
    static constexpr const char* BUTTON_UTIL_HOVER_TEXT   = "#b0b0dd";  // Utility button hover text

    // About Button
    static constexpr const char* BUTTON_ABOUT_BG      = "#2a2a3e";  // About button background
    static constexpr const char* BUTTON_ABOUT_BORDER  = "#4a4a6a";  // About button border
    static constexpr const char* BUTTON_ABOUT_TEXT    = "#9090bb";  // About button text
    static constexpr const char* BUTTON_ABOUT_HOVER_BG = "#3a3a5e";  // About button hover bg
    static constexpr const char* BUTTON_ABOUT_HOVER_BORDER = "#7a7aaa";  // About button hover border
    static constexpr const char* BUTTON_ABOUT_HOVER_TEXT = "#e0e0ff";  // About button hover text

    // ========== PROGRESS / STATUS COLORS ==========
    static constexpr const char* PROGRESS_BG          = "#2a2a3e";  // Progress bar background
    static constexpr const char* PROGRESS_FILL        = "#4a90d9";  // Progress bar filled portion
    static constexpr const char* STATUS_TEXT          = "#888";     // Status label text

    // ========== SCROLLBAR COLORS ==========
    static constexpr const char* SCROLLBAR_BG         = "#0d1117";  // Scrollbar track background
    static constexpr const char* SCROLLBAR_HANDLE     = "#4a4a6e";  // Scrollbar handle
    static constexpr const char* SCROLLBAR_HOVER      = "#6a6aae";  // Scrollbar handle hover

    // ========== TABLE COLORS ==========
    static constexpr const char* TABLE_BG             = "#0d1117";  // Table background
    static constexpr const char* TABLE_BORDER         = "#444";     // Table border
    static constexpr const char* TABLE_HEADER_BG      = "#1a1a2e";  // Table header background
    static constexpr const char* TABLE_HEADER_TEXT    = "#7a9ab8";  // Table header text
    static constexpr const char* TABLE_SELECTED_BG    = "#1e3a5a";  // Selected row background
    static constexpr const char* TABLE_SELECTED_TEXT  = "#ffffff";  // Selected row text

    // ========== ALTERNATING ROW COLORS (for solutions) ==========
    static constexpr const char* ROW_ALT_DARK         = "#0d1117";  // Odd rows (dark)
    static constexpr const char* ROW_ALT_LIGHT        = "#131c28";  // Even rows (slightly lighter)

    // ========== TOOLTIP COLORS ==========
    static constexpr const char* TOOLTIP_BG           = "#252540";  // Tooltip background
    static constexpr const char* TOOLTIP_TEXT         = "#d8d8f0";  // Tooltip text
    static constexpr const char* TOOLTIP_BORDER       = "#5a5a8a";  // Tooltip border

    // ========== CUBE COLORS (piece colors) ==========
    // Index mapping: [0]=dark grey (top), [1]=white (bottom), [2]=red, [3]=blue,
    //                [4]=orange, [5]=green, [6]=grey (partial/incomplete pieces)
    static constexpr const char* CUBE_TOP_FACE        = "#333333";  // Dark grey (top face)
    static constexpr const char* CUBE_BOT_FACE        = "#ffffff";  // White (bottom face)
    static constexpr const char* CUBE_RED             = "#ff0000";  // Red piece
    static constexpr const char* CUBE_BLUE            = "#0000ff";  // Blue piece
    static constexpr const char* CUBE_ORANGE          = "#ff8600";  // Orange piece
    static constexpr const char* CUBE_GREEN           = "#00ff00";  // Green piece
    static constexpr const char* CUBE_PARTIAL         = "#888888";  // Partial/incomplete piece
    static constexpr const char* CUBE_BORDER          = "#000000";  // Piece border (black)
    static constexpr const char* CUBE_SELECTION       = "#ffff00";  // Selection highlight (yellow)
    static constexpr const char* CANVAS_BG            = "#000000";  // Canvas/widget background (black)

    // ========== UTILITY FUNCTION ==========
    // Returns a QColor from any theme constant
    inline QColor color(const char* hex) {
        return QColor(hex);
    }
}
