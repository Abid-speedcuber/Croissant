#include "stylesheet.h"
#include "theme.h"
#include <QString>

QString buildStyleSheet(bool lightTheme) {
    bool L = lightTheme;
    auto b = [&](const char* dark, const char* light) -> QString {
        return L ? QString(light) : QString(dark);
    };

    QString PRIMARY_BG      = b(Theme::PRIMARY_BG,       Theme::LIGHT_PRIMARY_BG);
    QString SECONDARY_BG    = b(Theme::SECONDARY_BG,     Theme::LIGHT_TABLE_BG);
    QString TERTIARY_BG     = b(Theme::TERTIARY_BG,      Theme::LIGHT_TERTIARY_BG);
    QString DARK_BG         = b(Theme::DARK_BG,          Theme::LIGHT_DARK_BG);
    QString DISABLED_BG     = b(Theme::DISABLED_BG,      Theme::LIGHT_DISABLED_BG);
    QString HOVER_BG        = b(Theme::HOVER_BG,          Theme::LIGHT_HOVER_BG);
    QString PRESSED_BG      = b(Theme::PRESSED_BG,        Theme::LIGHT_PRESSED_BG);
    QString TEXT_PRIMARY    = b(Theme::TEXT_PRIMARY,      Theme::LIGHT_TEXT_PRIMARY);
    QString TEXT_SECONDARY  = b(Theme::TEXT_SECONDARY,    Theme::LIGHT_TEXT_SECONDARY);
    QString TEXT_MUTED      = b(Theme::TEXT_MUTED,        Theme::LIGHT_TEXT_MUTED);
    QString TEXT_DISABLED   = b(Theme::TEXT_DISABLED,     Theme::LIGHT_TEXT_DISABLED);
    QString TEXT_ERROR      = b(Theme::TEXT_ERROR,        Theme::LIGHT_TEXT_ERROR);
    QString TEXT_CYAN       = b(Theme::TEXT_CYAN,         Theme::LIGHT_TEXT_CYAN);
    QString TEXT_TERMINAL   = b(Theme::TEXT_TERMINAL,     Theme::LIGHT_TEXT_TERMINAL);
    QString BORDER_LIGHT    = b(Theme::BORDER_LIGHT,      Theme::LIGHT_BORDER_LIGHT);
    QString BORDER_DARK     = b(Theme::BORDER_DARK,       Theme::LIGHT_BORDER_DARK);
    QString BORDER_GROUP    = b(Theme::BORDER_GROUP,      Theme::LIGHT_BORDER_GROUP);
    QString BORDER_BOTTOM   = b(Theme::BORDER_BOTTOM,     Theme::LIGHT_BORDER_BOTTOM);
    QString CHECKBOX_BG     = b(Theme::CHECKBOX_BG,       Theme::LIGHT_CHECKBOX_BG);
    QString CHECKBOX_BORDER = b(Theme::CHECKBOX_BORDER,   Theme::LIGHT_BORDER_LIGHT);
    QString BUTTON_BG       = b(Theme::BUTTON_BG,         Theme::LIGHT_BUTTON_BG);
    QString BUTTON_BORDER   = b(Theme::BUTTON_BORDER,     Theme::LIGHT_BUTTON_BORDER);
    QString BUTTON_TEXT     = b(Theme::BUTTON_TEXT,       Theme::LIGHT_BUTTON_TEXT);
    QString SCROLLBAR_BG    = b(Theme::SCROLLBAR_BG,      Theme::LIGHT_SCROLLBAR_BG);
    QString SCROLLBAR_HANDLE= b(Theme::SCROLLBAR_HANDLE,  Theme::LIGHT_SCROLLBAR_HANDLE);
    QString SCROLLBAR_HOVER = b(Theme::SCROLLBAR_HOVER,   Theme::LIGHT_SCROLLBAR_HOVER);
    QString TABLE_BG        = b(Theme::TABLE_BG,          Theme::LIGHT_TABLE_BG);
    QString TABLE_BORDER    = b(Theme::TABLE_BORDER,      Theme::LIGHT_TABLE_BORDER);
    QString TABLE_HEADER_BG = b(Theme::TABLE_HEADER_BG,   Theme::LIGHT_TABLE_HEADER_BG);
    QString TABLE_HEADER_TEXT= b(Theme::TABLE_HEADER_TEXT, Theme::LIGHT_TABLE_HEADER_TEXT);
    QString TABLE_SEL_BG    = b(Theme::TABLE_SELECTED_BG, Theme::LIGHT_TABLE_SELECTED_BG);
    QString TABLE_SEL_TEXT  = b(Theme::TABLE_SELECTED_TEXT, Theme::LIGHT_TABLE_SELECTED_TEXT);
    QString TOOLTIP_BG      = b(Theme::TOOLTIP_BG,        Theme::LIGHT_TOOLTIP_BG);
    QString TOOLTIP_TEXT    = b(Theme::TOOLTIP_TEXT,      Theme::LIGHT_TOOLTIP_TEXT);
    QString TOOLTIP_BORDER  = b(Theme::TOOLTIP_BORDER,    Theme::LIGHT_TOOLTIP_BORDER);
    QString PROGRESS_BG     = b(Theme::PROGRESS_BG,       Theme::LIGHT_PROGRESS_BG);
    QString PROGRESS_FILL   = b(Theme::PROGRESS_FILL,     Theme::LIGHT_PROGRESS_FILL);
    QString UTIL_BG         = b(Theme::BUTTON_UTIL_BG,    Theme::LIGHT_BUTTON_BG);
    QString UTIL_BORDER     = b(Theme::BUTTON_UTIL_BORDER, Theme::LIGHT_BUTTON_BORDER);
    QString UTIL_TEXT       = b(Theme::BUTTON_UTIL_TEXT,  Theme::LIGHT_TEXT_SECONDARY);
    QString UTIL_HOVER_BG   = b(Theme::BUTTON_UTIL_HOVER_BG, Theme::LIGHT_HOVER_BG);
    QString UTIL_HOVER_BORDER= b(Theme::BUTTON_UTIL_HOVER_BORDER, Theme::LIGHT_BORDER_LIGHT);
    QString UTIL_HOVER_TEXT = b(Theme::BUTTON_UTIL_HOVER_TEXT, Theme::LIGHT_TEXT_PRIMARY);
    QString ABOUT_BG        = b(Theme::BUTTON_ABOUT_BG,   Theme::LIGHT_BUTTON_BG);
    QString ABOUT_BORDER    = b(Theme::BUTTON_ABOUT_BORDER, Theme::LIGHT_BUTTON_BORDER);
    QString ABOUT_TEXT      = b(Theme::BUTTON_ABOUT_TEXT, Theme::LIGHT_TEXT_SECONDARY);
    QString ABOUT_HOVER_BG  = b(Theme::BUTTON_ABOUT_HOVER_BG, Theme::LIGHT_HOVER_BG);
    QString ABOUT_HOVER_BORDER = b(Theme::BUTTON_ABOUT_HOVER_BORDER, Theme::LIGHT_BORDER_LIGHT);
    QString ABOUT_HOVER_TEXT= b(Theme::BUTTON_ABOUT_HOVER_TEXT, Theme::LIGHT_TEXT_PRIMARY);
    QString RESET_BG        = b(Theme::BUTTON_RESET_BG,   Theme::LIGHT_BUTTON_BG);
    QString RESET_BORDER    = b(Theme::BUTTON_RESET_BORDER, Theme::LIGHT_BUTTON_BORDER);
    QString RESET_HOVER     = b(Theme::BUTTON_RESET_HOVER, Theme::LIGHT_HOVER_BG);
    
    // Input bar unified colors
    QString INPUT_MODE_BG   = b(Theme::INPUT_MODE_BG,      Theme::LIGHT_INPUT_MODE_BG);
    QString INPUT_MODE_BORDER = b(Theme::INPUT_MODE_BORDER, Theme::LIGHT_INPUT_MODE_BORDER);
    QString INPUT_MODE_HOVER = b(Theme::INPUT_MODE_HOVER,    Theme::LIGHT_INPUT_MODE_HOVER);
    QString INPUT_ARROW_BG  = b(Theme::INPUT_ARROW_BG,     Theme::LIGHT_INPUT_ARROW_BG);
    QString INPUT_ARROW_BORDER = b(Theme::INPUT_ARROW_BORDER, Theme::LIGHT_INPUT_ARROW_BORDER);
    QString INPUT_ARROW_HOVER = b(Theme::INPUT_ARROW_HOVER,  Theme::LIGHT_INPUT_ARROW_HOVER);
    QString INPUT_APPLY_BG  = b(Theme::INPUT_APPLY_BG,     Theme::LIGHT_INPUT_APPLY_BG);
    QString INPUT_APPLY_BORDER = b(Theme::INPUT_APPLY_BORDER, Theme::LIGHT_INPUT_APPLY_BORDER);
    QString INPUT_APPLY_HOVER = b(Theme::INPUT_APPLY_HOVER,  Theme::LIGHT_INPUT_APPLY_HOVER);
    QString INPUT_FIELD_BG  = b(Theme::INPUT_FIELD_BG,     Theme::LIGHT_INPUT_FIELD_BG);
    QString INPUT_FIELD_BORDER = b(Theme::INPUT_FIELD_BORDER, Theme::LIGHT_INPUT_FIELD_BORDER);
    QString INPUT_FIELD_TEXT = b(Theme::INPUT_FIELD_TEXT,   Theme::LIGHT_INPUT_FIELD_TEXT);
    
    QString UNDO_REDO_DISABLED_BG = b("#1e1e2a", Theme::LIGHT_DISABLED_BG);
    QString UNDO_REDO_DISABLED_BORDER = b("#333", Theme::LIGHT_BORDER_DARK);
    QString UNDO_REDO_DISABLED_TEXT   = b("#444", Theme::LIGHT_TEXT_DISABLED);
    QString INPUT_TEXT = b("#fff", Theme::LIGHT_TEXT_PRIMARY);

    return QString(R"(
        /* ========== Main Window & Base Styling ========== */
        QMainWindow, QWidget { 
            background: %1; 
            color: %2; 
            font-family: 'Segoe UI', Arial; 
            font-size: 13px; 
        }

        /* ========== GroupBox ========== */
        QGroupBox { 
            border: 1px solid %3; 
            border-radius: 6px; 
            margin-top: 8px; 
            padding-top: 8px; 
            color: %4; 
        }

        QGroupBox::title { 
            subcontrol-origin: margin; 
            left: 8px; 
            padding: 0 4px; 
        }

        /* ========== CheckBox ========== */
        QCheckBox { 
            spacing: 6px;
        }
        QCheckBox::indicator { 
            width:14px;
            height:14px;
            border-radius:3px;
            border:1px solid %5;
            background:%6;
        }

        QCheckBox::indicator:checked { 
            background: %7;
            border-color: %7;
        }

        QCheckBox#chkRankErgo::indicator:checked { 
            background: %8;
            border-color: %8;
        }

        QCheckBox:disabled { 
            color: #4a4a5a;
        }

        QCheckBox::indicator:disabled { 
            border-color: %9;
            background: %10;
        }

        /* ========== Input Fields (LineEdit & SpinBox) ========== */
        QLineEdit { 
            background: %11;
            border: 1px solid %12;
            border-radius: 4px;
            padding: 3px 6px;
            color: %60;
        }

        QLineEdit:disabled { 
            color: %13;
            background: %14;
            border-color: %15;
        }

        QLineEdit#txtCommand { 
            font-family: monospace;
            color: %16;
            font-size: 12px;
            border-radius: 0;
            border-top-left-radius: 4px;
            border-bottom-left-radius: 4px;
            border-right: none;
        }

        QSpinBox {
            background: %11;
            border: 1px solid %12;
            border-radius: 4px;
            padding: 2px 4px 2px 6px;
            color: %60;
            min-width: 48px;
        }

        QSpinBox:disabled { 
            color: %13;
            background: %14;
            border-color: %15;
        }

        QSpinBox::up-button   { 
            width: 0;
            border: none;
        }

        QSpinBox::down-button { 
            width: 0;
            border: none;
        }

        QSpinBox::up-arrow    { 
            width: 0;
            height: 0;
        }

        QSpinBox::down-arrow  { 
            width: 0;
            height: 0;
        }

        /* ========== TextEdit ========== */
        QTextEdit#txtOutput { 
            background: %17;
            border: 1px solid %3;
            border-radius: 4px;
            font-family: monospace;
            font-size: 12px;
            color: %18;
            padding: 4px;
        }

        QTextEdit#txtOutput > QWidget { 
            background: %17;
        }

        /* ========== Buttons ========== */
        QPushButton { 
            background: %19;
            border: 1px solid %20;
            border-radius: 5px;
            padding: 5px 12px;
            color: %21;
        }

        QPushButton:hover { 
            background: %22;
            border-color: %20;
        }

        QPushButton:pressed { 
            background: %23;
        }

        QPushButton#btnSolve { 
            background: %24;
            border-color: %25;
            color: #fff;
            font-size: 13px;
            font-weight: bold;
        }

        QPushButton#btnSolve:hover { 
            background: %26;
        }

        QPushButton#btnSolve:disabled { 
            background: #333;
            border-color: #444;
            color: #666;
        }

        QPushButton#btnCopy { 
            background: %11;
            border: 1px solid %12;
            border-left: none;
            border-radius: 4px;
            border-top-left-radius: 0px;
            border-bottom-left-radius: 0px;
            color: %4;
            font-size: 14px;
            padding: 0;
            margin-right: 5px;
        }

        QPushButton#btnCopy:hover { 
            background: %22;
            color: %21;
        }

        QPushButton#btnReset {
            background: %27;
            border: 1px solid %28;
            border-radius: 26px;
            color: %4;
            font-size: 11px;
            font-weight: bold;
            padding: 0;
            letter-spacing: -0.5px;
        }

        QPushButton#btnReset:hover { 
            background: %29;
            border-color: %20;
            color: %21;
        }

        QPushButton#btnUndo, QPushButton#btnRedo {
            background: %19;
            border-color: %20;
            color: %21;
        }

        QPushButton#btnUndo:hover, QPushButton#btnRedo:hover {
            background: %22;
            border-color: %20;
        }

        QPushButton#btnUndo:pressed, QPushButton#btnRedo:pressed {
            background: %23;
        }

        QPushButton#btnUndo:disabled, QPushButton#btnRedo:disabled {
            background: %63;
            border-color: %64;
            color: %65;
        }

        QPushButton#btnApplyScramble {
            background: %11;
            border: 1px solid %12;
            border-radius: 4px;
            color: %4;
            padding: 4px 8px;
        }

        QPushButton#btnApplyScramble:hover { 
            background: %22;
            border-color: %20;
        }

        QPushButton#btnScrambleMode {
            background: #333350;
            border: 1px solid %12;
            border-radius: 0;
            border-top-left-radius: 4px;
            border-bottom-left-radius: 4px;
            border-right: none;
            color: #fff;
            padding: 0 6px;
            font-size: 11px;
        }

        QPushButton#btnScrambleMode:checked { 
            color: #fff;
            background: #333350;
        }

        QPushButton#btnScrambleMode:hover { 
            background: %22;
        }

        QPushButton#btnInputMode {
            background: %66;
            border: 1px solid %67;
            border-radius: 4px 0 0 4px;
            border-right: none;
            color: #fff;
            padding: 0 10px;
            font-size: 11px;
            font-weight: bold;
        }

        QPushButton#btnInputMode:hover { 
            background: %68;
            border-color: %67;
        }

        QPushButton#btnInputModeArrow {
            background: %69;
            border: 1px solid %70;
            border-radius: 0 4px 4px 0;
            border-left: none;
            color: #fff;
            padding: 0 6px;
            font-size: 11px;
        }

        QPushButton#btnInputModeArrow:hover { 
            background: %71;
            border-color: %70;
        }

        QPushButton#btnApply {
            background: %72;
            border: 1px solid %73;
            border-radius: 4px;
            margin-left: 8px;
            color: #fff;
            font-size: 11px;
            font-weight: bold;
            padding: 0 12px;
            min-width: 52px;
        }

        QPushButton#btnApply:hover { 
            background: %74;
            border-color: %73;
        }

        QLineEdit#txtMainInput {
            border-radius: 4px;
            border: 1px solid %75;
            margin-left: 6px;
            font-family: monospace;
            font-size: 12px;
            background: %76;
            color: %77;
        }

        QLineEdit#txtMainInput[hasError="true"] {
            border-color: %39;
        }

        QPushButton#btnExpand, QPushButton#btnCopyTerminal, QPushButton#btnTableMode {
            background: %30;
            border: 1px solid %31;
            border-radius: 4px;
            color: %32;
            font-size: 13px;
            padding: 0;
        }

        QPushButton#btnExpand:hover, QPushButton#btnCopyTerminal:hover, QPushButton#btnTableMode:hover {
            background: %33;
            border-color: %34;
            color: %35;
        }

        QPushButton#btnExpand:pressed, QPushButton#btnCopyTerminal:pressed, QPushButton#btnTableMode:pressed {
            background: %23;
        }

        /* ========== ProgressBar ========== */
        QProgressBar { 
            border: none;
            background: %36;
            border-radius: 3px;
        }

        QProgressBar::chunk { 
            background: %37;
            border-radius: 3px;
        }

        /* ========== Labels ========== */
        QLabel#lblStatus { 
            color: %38;
            font-size: 11px;
        }

        QLabel#lblScrambleError { 
            color: %39;
            font-size: 11px;
            padding: 2px 2px;
        }

        QLabel#lblCommandError  { 
            color: %39;
            font-size: 11px;
            padding: 2px 2px;
        }

        /* ========== ScrollBars ========== */
        QScrollBar:vertical { 
            background: %40;
            width: 8px;
            border-radius: 4px;
            margin: 0;
        }

        QScrollBar::handle:vertical { 
            background: %41;
            border-radius: 4px;
            min-height: 24px;
        }

        QScrollBar::handle:vertical:hover { 
            background: %42;
        }

        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { 
            height: 0px;
            border: 0;
        }

        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { 
            background: none;
        }

        QScrollBar:horizontal { 
            background: %40;
            height: 8px;
            border-radius: 4px;
            margin: 0;
        }

        QScrollBar::handle:horizontal { 
            background: %41;
            border-radius: 4px;
            min-width: 24px;
        }

        QScrollBar::handle:horizontal:hover { 
            background: %42;
        }

        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { 
            width: 0px;
            border: 0;
        }

        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { 
            background: none;
        }

        /* ========== TableWidget ========== */
        QTableWidget#m_solutionTable {
            background: %43;
            border: 1px solid %44;
            border-radius: 4px;
            gridline-color: %45;
            font-family: monospace;
            font-size: 12px;
            color: %2;
        }

        QTableWidget#m_solutionTable QHeaderView::section {
            background: %46;
            color: %47;
            border: none;
            border-bottom: 1px solid %44;
            padding: 4px
            font-size: 11px;
            font-weight: bold;
        }

        QTableWidget#m_solutionTable::item:selected {
            background: %48;
            color: %49;
        }

        /* ========== TopBar ========== */
        QWidget#topBar {
            background: %50;
            border-bottom: 2px solid %51;
            min-height: 52px;
            max-height: 52px;
        }

        QLabel#logoLabel {
            background: transparent;
            padding: 0;
        }

        QPushButton#btnAbout, QPushButton#btnHamburger {
            background: %52;
            border: 1px solid %53;
            border-radius: 15px;
            color: %54;
            font-size: 15px;
            font-weight: bold;
            padding: 0;
            min-width: 30px;
            max-width: 30px;
            min-height: 30px;
            max-height: 30px;
        }

        QPushButton#btnAbout:hover, QPushButton#btnHamburger:hover {
            background: %55;
            border-color: %56;
            color: %57;
        }

        /* ========== Tooltips ========== */
        QToolTip {
            background: %58;
            color: %59;
            border: 1px solid %58;
            border-radius: 5px;
            padding: 6px 10px;
            font-size: 12px;
            opacity: 230;
        }

        /* ========== Cursor ========== */
        QPushButton, QCheckBox, QAbstractItemView::item {
            cursor: pointer;
        }
    )")
        .arg(PRIMARY_BG)          // %1  main bg
        .arg(TEXT_PRIMARY)        // %2  main text
        .arg(BORDER_GROUP)        // %3  group border
        .arg(TEXT_SECONDARY)      // %4  secondary text
        .arg(CHECKBOX_BORDER)     // %5
        .arg(CHECKBOX_BG)         // %6
        .arg(Theme::CHECKBOX_CHECK_BLUE)  // %7
        .arg(Theme::CHECKBOX_CHECK_ORANGE)// %8
        .arg(BORDER_DARK)         // %9
        .arg(DISABLED_BG)         // %10
        .arg(TERTIARY_BG)         // %11 input/button bg
        .arg(BORDER_LIGHT)        // %12 input border
        .arg(TEXT_DISABLED)       // %13
        .arg(DISABLED_BG)         // %14
        .arg(BORDER_DARK)         // %15
        .arg(TEXT_CYAN)           // %16 command line text
        .arg(SECONDARY_BG)        // %17 terminal bg
        .arg(TEXT_TERMINAL)       // %18
        .arg(BUTTON_BG)           // %19
        .arg(BUTTON_BORDER)       // %20
        .arg(BUTTON_TEXT)         // %21
        .arg(HOVER_BG)            // %22
        .arg(PRESSED_BG)          // %23
        .arg(Theme::BUTTON_SOLVE_BG)     // %24
        .arg(Theme::BUTTON_SOLVE_BORDER) // %25
        .arg(Theme::BUTTON_SOLVE_HOVER)  // %26
        .arg(RESET_BG)            // %27
        .arg(RESET_BORDER)        // %28
        .arg(RESET_HOVER)         // %29
        .arg(UTIL_BG)             // %30 floating btns bg
        .arg(UTIL_BORDER)         // %31
        .arg(UTIL_TEXT)           // %32
        .arg(UTIL_HOVER_BG)       // %33
        .arg(UTIL_HOVER_BORDER)   // %34
        .arg(UTIL_HOVER_TEXT)     // %35
        .arg(PROGRESS_BG)         // %36
        .arg(PROGRESS_FILL)       // %37
        .arg(TEXT_MUTED)          // %38 status text
        .arg(TEXT_ERROR)          // %39
        .arg(SCROLLBAR_BG)        // %40
        .arg(SCROLLBAR_HANDLE)    // %41
        .arg(SCROLLBAR_HOVER)     // %42
        .arg(TABLE_BG)            // %43
        .arg(TABLE_BORDER)        // %44
        .arg(TABLE_BORDER)        // %45 gridline
        .arg(TABLE_HEADER_BG)     // %46
        .arg(TABLE_HEADER_TEXT)   // %47
        .arg(TABLE_SEL_BG)        // %48
        .arg(TABLE_SEL_TEXT)      // %49
        .arg(DARK_BG)             // %50 topbar bg
        .arg(BORDER_BOTTOM)       // %51
        .arg(ABOUT_BG)            // %52
        .arg(ABOUT_BORDER)        // %53
        .arg(ABOUT_TEXT)          // %54
        .arg(ABOUT_HOVER_BG)      // %55
        .arg(ABOUT_HOVER_BORDER)  // %56
        .arg(ABOUT_HOVER_TEXT)    // %57
        .arg(TOOLTIP_BG)          // %58
        .arg(TOOLTIP_TEXT)        // %59
        .arg(INPUT_TEXT)          // %60 input text color
        .arg(UNDO_REDO_DISABLED_BG)    // %61
        .arg(UNDO_REDO_DISABLED_BORDER)// %62
        .arg(UNDO_REDO_DISABLED_TEXT)  // %63
        .arg(INPUT_MODE_BG)       // %64 input mode button bg
        .arg(INPUT_MODE_BORDER)   // %65 input mode button border
        .arg(INPUT_MODE_HOVER)    // %66 input mode button hover
        .arg(INPUT_ARROW_BG)      // %67 input arrow button bg
        .arg(INPUT_ARROW_BORDER)  // %68 input arrow button border
        .arg(INPUT_ARROW_HOVER)   // %69 input arrow button hover
        .arg(INPUT_APPLY_BG)      // %70 input apply button bg
        .arg(INPUT_APPLY_BORDER)  // %71 input apply button border
        .arg(INPUT_APPLY_HOVER)   // %72 input apply button hover
        .arg(INPUT_FIELD_BORDER)  // %73 input field border
        .arg(INPUT_FIELD_BG)      // %74 input field background
        .arg(INPUT_FIELD_TEXT);   // %75 input field text color
}
