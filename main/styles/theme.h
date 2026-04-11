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

    // Input Bar Buttons (mode toggle, dropdown, apply) - PRIMARY ACTION
    static constexpr const char* INPUT_MODE_BG        = "#1a6b3c";  // Input mode button background (green)
    static constexpr const char* INPUT_MODE_BORDER    = "#2db570";  // Input mode button border
    static constexpr const char* INPUT_MODE_HOVER     = "#227a47";  // Input mode button hover

    static constexpr const char* INPUT_ARROW_BG       = "#1a6b3c";  // Dropdown arrow background
    static constexpr const char* INPUT_ARROW_BORDER   = "#2db570";  // Dropdown arrow border
    static constexpr const char* INPUT_ARROW_HOVER    = "#227a47";  // Dropdown arrow hover

    static constexpr const char* INPUT_APPLY_BG       = "#1a6b3c";  // Apply button background
    static constexpr const char* INPUT_APPLY_BORDER   = "#2db570";  // Apply button border
    static constexpr const char* INPUT_APPLY_HOVER    = "#227a47";  // Apply button hover

    static constexpr const char* INPUT_FIELD_BG       = "#2a2a3e";  // Input field background
    static constexpr const char* INPUT_FIELD_BORDER   = "#555";     // Input field border
    static constexpr const char* INPUT_FIELD_TEXT     = "#e0e0e0";  // Input field text

    // Light theme variants for input bar
    static constexpr const char* LIGHT_INPUT_MODE_BG      = "#1a6b3c";  // Same green for visibility
    static constexpr const char* LIGHT_INPUT_MODE_BORDER  = "#2db570";
    static constexpr const char* LIGHT_INPUT_MODE_HOVER   = "#227a47";

    static constexpr const char* LIGHT_INPUT_ARROW_BG     = "#1a6b3c";
    static constexpr const char* LIGHT_INPUT_ARROW_BORDER = "#2db570";
    static constexpr const char* LIGHT_INPUT_ARROW_HOVER  = "#227a47";

    static constexpr const char* LIGHT_INPUT_APPLY_BG     = "#1a6b3c";
    static constexpr const char* LIGHT_INPUT_APPLY_BORDER = "#2db570";
    static constexpr const char* LIGHT_INPUT_APPLY_HOVER  = "#227a47";

    static constexpr const char* LIGHT_INPUT_FIELD_BG     = "#e0e0f0";  // Light background
    static constexpr const char* LIGHT_INPUT_FIELD_BORDER = "#aaa";     // Light border
    static constexpr const char* LIGHT_INPUT_FIELD_TEXT   = "#1a1a3e";  // Dark text for light bg

    // Reset Button (red - alarming)
    static constexpr const char* BUTTON_RESET_BG      = "#2a2a3e";  // Reset button background
    static constexpr const char* BUTTON_RESET_BORDER  = "#555";     // Reset button border
    static constexpr const char* BUTTON_RESET_HOVER   = "#3a3a5e";  // Reset button hover

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
    static constexpr const char* CANVAS_BG            = "#1a1a2e";  // Canvas/widget background

    // ========== SIDEBAR / MODAL COLORS ==========
    static constexpr const char* SIDEBAR_BG           = "#13132a";  // Sidebar background
    static constexpr const char* SIDEBAR_BORDER       = "#2a2a4a";  // Sidebar border
    static constexpr const char* MODAL_BG             = "#1e1e34";  // Modal card background
    static constexpr const char* MODAL_BORDER         = "#3a3a5e";  // Modal border
    static constexpr const char* MODAL_OVERLAY        = "rgba(0,0,0,170)"; // Modal overlay

    // ========== LIGHT THEME COLORS ==========
    static constexpr const char* LIGHT_PRIMARY_BG     = "#f0f0f8";
    static constexpr const char* LIGHT_SECONDARY_BG   = "#e0e0f0";
    static constexpr const char* LIGHT_TERTIARY_BG    = "#d8d8ee";
    static constexpr const char* LIGHT_DARK_BG        = "#c8c8e0";
    static constexpr const char* LIGHT_DISABLED_BG    = "#e8e8f4";
    static constexpr const char* LIGHT_HOVER_BG       = "#d0d0e8";
    static constexpr const char* LIGHT_PRESSED_BG     = "#c0c0d8";
    static constexpr const char* LIGHT_TEXT_PRIMARY   = "#1a1a3e";
    static constexpr const char* LIGHT_TEXT_SECONDARY = "#444466";
    static constexpr const char* LIGHT_TEXT_MUTED     = "#888";
    static constexpr const char* LIGHT_TEXT_DISABLED  = "#aaa";
    static constexpr const char* LIGHT_TEXT_ERROR     = "#cc2222";
    static constexpr const char* LIGHT_TEXT_CYAN      = "#1a5fa8";
    static constexpr const char* LIGHT_TEXT_TERMINAL  = "#1a5080";
    static constexpr const char* LIGHT_TEXT_SOLUTION  = "#1a5fa8";
    static constexpr const char* LIGHT_BORDER_LIGHT   = "#aaa";
    static constexpr const char* LIGHT_BORDER_DARK    = "#c8c8de";
    static constexpr const char* LIGHT_BORDER_GROUP   = "#aaa";
    static constexpr const char* LIGHT_BORDER_BOTTOM  = "#c0c0da";
    static constexpr const char* LIGHT_CHECKBOX_BG    = "#d8d8ee";
    static constexpr const char* LIGHT_BUTTON_BG      = "#d8d8ee";
    static constexpr const char* LIGHT_BUTTON_BORDER  = "#aaa";
    static constexpr const char* LIGHT_BUTTON_TEXT    = "#1a1a3e";
    static constexpr const char* LIGHT_SCROLLBAR_BG   = "#dcdcec";
    static constexpr const char* LIGHT_SCROLLBAR_HANDLE = "#a0a0c0";
    static constexpr const char* LIGHT_SCROLLBAR_HOVER = "#7878a8";
    static constexpr const char* LIGHT_TABLE_BG       = "#e8e8f8";
    static constexpr const char* LIGHT_TABLE_HEADER_BG= "#d8d8ee";
    static constexpr const char* LIGHT_TABLE_BORDER   = "#bbb";
    static constexpr const char* LIGHT_TABLE_SELECTED_BG  = "#4a80c4";
    static constexpr const char* LIGHT_TABLE_SELECTED_TEXT= "#ffffff";
    static constexpr const char* LIGHT_TABLE_HEADER_TEXT  = "#3a5a8a";
    static constexpr const char* LIGHT_ROW_ALT_DARK   = "#e0e8f8";
    static constexpr const char* LIGHT_ROW_ALT_LIGHT  = "#eceef8";
    static constexpr const char* LIGHT_TOOLTIP_BG     = "#2a2a50";
    static constexpr const char* LIGHT_TOOLTIP_TEXT   = "#e8e8ff";
    static constexpr const char* LIGHT_TOOLTIP_BORDER = "#5a5a8a";
    static constexpr const char* LIGHT_SOLVE_BG       = "#1a6b3c";
    static constexpr const char* LIGHT_SOLVE_BORDER   = "#2db570";
    static constexpr const char* LIGHT_SOLVE_HOVER    = "#227a47";
    static constexpr const char* LIGHT_PROGRESS_BG    = "#c8c8e0";
    static constexpr const char* LIGHT_PROGRESS_FILL  = "#4a80c4";
    static constexpr const char* LIGHT_SIDEBAR_BG     = "#e8e8f8";
    static constexpr const char* LIGHT_SIDEBAR_BORDER = "#c0c0da";
    static constexpr const char* LIGHT_CANVAS_BG      = "#f0f0f8";

    // ========== INLINE / RUNTIME DYNAMIC COLORS ==========
    static constexpr const char* DEPTHS_INACTIVE_COLOR  = "#666";
    static constexpr const char* DEPTHS_INACTIVE_BG     = "#1e1e30";
    static constexpr const char* DEPTHS_INACTIVE_BORDER = "#3a3a4e";
    static constexpr const char* ERGO_ALT_TEXT_DARK     = "#cdcdcd";
    static constexpr const char* ERGO_ALT_META_DARK     = "#969696";

    // ========== UTILITY FUNCTION ==========
    // Returns a QColor from any theme constant
    inline QColor color(const char* hex) {
        return QColor(hex);
    }
}
