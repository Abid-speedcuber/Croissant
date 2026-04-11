#include "stylesheet.h"
#include "theme.h"
#include <QString>
#include <QFile>

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

    // Load stylesheet from file
    QFile styleFile(":/styles/stylesheet.qss");
    if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return ""; // Return empty if file not found
    }
    QString rawStyle = styleFile.readAll();
    styleFile.close();

    return rawStyle
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
