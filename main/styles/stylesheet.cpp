#include "stylesheet.h"
#include "theme.h"
#include <QString>
#include <QFile>
#include <QMap>

QString buildStyleSheet(bool lightTheme) {
    // Helper lambda to choose between dark and light versions
    auto color = [&](const QString& darkColor, const QString& lightColor) -> QString {
        return lightTheme ? lightColor : darkColor;
    };

    // Build a map of all CSS variable names to their values
    QMap<QString, QString> variables;
    
    // ========== DARK MODE / PRIMARY BACKGROUND COLORS ==========
    variables["--primary-bg"] = color(Theme::primaryBg(false), Theme::primaryBg(true));
    variables["--secondary-bg"] = color(Theme::secondaryBg(false), Theme::secondaryBg(true));
    variables["--tertiary-bg"] = color(Theme::tertiaryBg(false), Theme::tertiaryBg(true));
    variables["--dark-bg"] = color(Theme::darkBg(false), Theme::darkBg(true));
    variables["--disabled-bg"] = color(Theme::disabledBg(false), Theme::disabledBg(true));
    variables["--hover-bg"] = color(Theme::hoverBg(false), Theme::hoverBg(true));
    variables["--pressed-bg"] = color(Theme::pressedBg(false), Theme::pressedBg(true));

    // ========== TEXT / FOREGROUND COLORS ==========
    variables["--text-primary"] = color(Theme::textPrimary(false), Theme::textPrimary(true));
    variables["--text-secondary"] = color(Theme::textSecondary(false), Theme::textSecondary(true));
    variables["--text-muted"] = color(Theme::textMuted(false), Theme::textMuted(true));
    variables["--text-disabled"] = color(Theme::textDisabled(false), Theme::textDisabled(true));
    variables["--text-error"] = color(Theme::textError(false), Theme::textError(true));
    variables["--text-cyan"] = color(Theme::textCyan(false), Theme::textCyan(true));
    variables["--text-terminal"] = color(Theme::textTerminal(false), Theme::textTerminal(true));
    variables["--text-solution"] = color(Theme::textSolution(false), Theme::textSolution(true));

    // ========== BORDER / OUTLINE COLORS ==========
    variables["--border-light"] = color(Theme::borderLight(false), Theme::borderLight(true));
    variables["--border-dark"] = color(Theme::borderDark(false), Theme::borderDark(true));
    variables["--border-accent"] = color(Theme::borderAccent(false), Theme::borderAccent(true));
    variables["--border-group"] = color(Theme::borderGroup(false), Theme::borderGroup(true));
    variables["--border-bottom"] = color(Theme::borderBottom(false), Theme::borderBottom(true));

    // ========== CHECKBOX / INDICATOR COLORS ==========
    variables["--checkbox-bg"] = color(Theme::checkboxBg(false), Theme::checkboxBg(true));
    variables["--checkbox-check-blue"] = color(Theme::checkboxCheckBlue(false), Theme::checkboxCheckBlue(true));
    variables["--checkbox-check-orange"] = color(Theme::checkboxCheckOrange(false), Theme::checkboxCheckOrange(true));
    variables["--checkbox-border"] = color(Theme::checkboxBorder(false), Theme::checkboxBorder(true));

    // ========== BUTTON COLORS ==========
    variables["--button-bg"] = color(Theme::buttonBg(false), Theme::buttonBg(true));
    variables["--button-border"] = color(Theme::buttonBorder(false), Theme::buttonBorder(true));
    variables["--button-text"] = color(Theme::buttonText(false), Theme::buttonText(true));
    variables["--button-secondary-text"] = color(Theme::buttonSecondaryText(false), Theme::buttonSecondaryText(true));

    variables["--button-solve-bg"] = color(Theme::buttonSolveBg(false), Theme::buttonSolveBg(true));
    variables["--button-solve-border"] = color(Theme::buttonSolveBorder(false), Theme::buttonSolveBorder(true));
    variables["--button-solve-hover"] = color(Theme::buttonSolveHover(false), Theme::buttonSolveHover(true));

    // ========== TOP BAR ===================
    variables["--topbar-bg"] = color(Theme::topBarBg(false), Theme::topBarBg(true));

    // ========== INPUT BAR COLORS ==========
    variables["--input-mode-bg"] = color(Theme::inputModeBg(false), Theme::inputModeBg(true));
    variables["--input-mode-border"] = color(Theme::inputModeBorder(false), Theme::inputModeBorder(true));
    variables["--input-mode-hover"] = color(Theme::inputModeHover(false), Theme::inputModeHover(true));
    variables["--input-mode-text"] = color(Theme::inputModeText(false), Theme::inputModeText(true));

    variables["--input-arrow-bg"] = color(Theme::inputArrowBg(false), Theme::inputArrowBg(true));
    variables["--input-arrow-border"] = color(Theme::inputArrowBorder(false), Theme::inputArrowBorder(true));
    variables["--input-arrow-hover"] = color(Theme::inputArrowHover(false), Theme::inputArrowHover(true));

    variables["--input-apply-bg"] = color(Theme::inputApplyBg(false), Theme::inputApplyBg(true));
    variables["--input-apply-border"] = color(Theme::inputApplyBorder(false), Theme::inputApplyBorder(true));
    variables["--input-apply-hover"] = color(Theme::inputApplyHover(false), Theme::inputApplyHover(true));
    variables["--input-apply-text"] = color(Theme::inputApplyText(false), Theme::inputApplyText(true));
    variables["--input-apply-hover-text"] = color(Theme::inputApplyHoverText(false), Theme::inputApplyHoverText(true));

    variables["--input-field-bg"] = color(Theme::inputFieldBg(false), Theme::inputFieldBg(true));
    variables["--input-field-border"] = color(Theme::inputFieldBorder(false), Theme::inputFieldBorder(true));
    variables["--input-field-text"] = color(Theme::inputFieldText(false), Theme::inputFieldText(true));
    variables["--input-panel-bg"] = color(Theme::inputPanelBg(false), Theme::inputPanelBg(true));

    // ========== RESET/STOP/UTIL BUTTON COLORS ==========
    variables["--button-reset-bg"] = color(Theme::buttonResetBg(false), Theme::buttonResetBg(true));
    variables["--button-reset-border"] = color(Theme::buttonResetBorder(false), Theme::buttonResetBorder(true));
    variables["--button-reset-hover"] = color(Theme::buttonResetHover(false), Theme::buttonResetHover(true));

    variables["--button-stop-bg"] = color(Theme::buttonStopBg(false), Theme::buttonStopBg(true));
    variables["--button-stop-border"] = color(Theme::buttonStopBorder(false), Theme::buttonStopBorder(true));
    variables["--button-stop-hover"] = color(Theme::buttonStopHover(false), Theme::buttonStopHover(true));
    variables["--button-stop-text"] = color(Theme::buttonStopText(false), Theme::buttonStopText(true));

    variables["--button-util-bg"] = color(Theme::buttonUtilBg(false), Theme::buttonUtilBg(true));
    variables["--button-util-border"] = color(Theme::buttonUtilBorder(false), Theme::buttonUtilBorder(true));
    variables["--button-util-text"] = color(Theme::buttonUtilText(false), Theme::buttonUtilText(true));
    variables["--button-util-hover-bg"] = color(Theme::buttonUtilHoverBg(false), Theme::buttonUtilHoverBg(true));
    variables["--button-util-hover-border"] = color(Theme::buttonUtilHoverBorder(false), Theme::buttonUtilHoverBorder(true));
    variables["--button-util-hover-text"] = color(Theme::buttonUtilHoverText(false), Theme::buttonUtilHoverText(true));

    variables["--button-about-bg"] = color(Theme::buttonAboutBg(false), Theme::buttonAboutBg(true));
    variables["--button-about-border"] = color(Theme::buttonAboutBorder(false), Theme::buttonAboutBorder(true));
    variables["--button-about-text"] = color(Theme::buttonAboutText(false), Theme::buttonAboutText(true));
    variables["--button-about-hover-bg"] = color(Theme::buttonAboutHoverBg(false), Theme::buttonAboutHoverBg(true));
    variables["--button-about-hover-border"] = color(Theme::buttonAboutHoverBorder(false), Theme::buttonAboutHoverBorder(true));
    variables["--button-about-hover-text"] = color(Theme::buttonAboutHoverText(false), Theme::buttonAboutHoverText(true));

    // ========== PROGRESS / STATUS COLORS ==========
    variables["--progress-bg"] = color(Theme::progressBg(false), Theme::progressBg(true));
    variables["--progress-fill"] = color(Theme::progressFill(false), Theme::progressFill(true));
    variables["--status-text"] = color(Theme::statusText(false), Theme::statusText(true));

    // ========== SCROLLBAR COLORS ==========
    variables["--scrollbar-bg"] = color(Theme::scrollbarBg(false), Theme::scrollbarBg(true));
    variables["--scrollbar-handle"] = color(Theme::scrollbarHandle(false), Theme::scrollbarHandle(true));
    variables["--scrollbar-hover"] = color(Theme::scrollbarHover(false), Theme::scrollbarHover(true));

    // ========== TABLE COLORS ==========
    variables["--table-bg"] = color(Theme::tableBg(false), Theme::tableBg(true));
    variables["--table-border"] = color(Theme::tableBorder(false), Theme::tableBorder(true));
    variables["--table-header-bg"] = color(Theme::tableHeaderBg(false), Theme::tableHeaderBg(true));
    variables["--table-header-text"] = color(Theme::tableHeaderText(false), Theme::tableHeaderText(true));
    variables["--table-selected-bg"] = color(Theme::tableSelectedBg(false), Theme::tableSelectedBg(true));
    variables["--table-selected-text"] = color(Theme::tableSelectedText(false), Theme::tableSelectedText(true));

    // ========== ALTERNATING ROW COLORS ==========
    variables["--row-alt-dark"] = color(Theme::rowAltDark(false), Theme::rowAltDark(true));
    variables["--row-alt-light"] = color(Theme::rowAltLight(false), Theme::rowAltLight(true));

    // ========== TOOLTIP COLORS ==========
    variables["--tooltip-bg"] = color(Theme::tooltipBg(false), Theme::tooltipBg(true));
    variables["--tooltip-text"] = color(Theme::tooltipText(false), Theme::tooltipText(true));
    variables["--tooltip-border"] = color(Theme::tooltipBorder(false), Theme::tooltipBorder(true));

    // ========== CUBE COLORS ==========
    variables["--cube-top-face"] = Theme::cubeTopFace(false);
    variables["--cube-bot-face"] = Theme::cubeBotFace(false);
    variables["--cube-red"] = Theme::cubeRed(false);
    variables["--cube-blue"] = Theme::cubeBlue(false);
    variables["--cube-orange"] = Theme::cubeOrange(false);
    variables["--cube-green"] = Theme::cubeGreen(false);
    variables["--cube-partial"] = Theme::cubePartial(false);
    variables["--cube-border"] = Theme::cubeBorder(false);
    variables["--cube-selection"] = Theme::cubeSelection(false);
    variables["--canvas-bg"] = color(Theme::canvasBg(false), Theme::canvasBg(true));
    variables["--cube-with-reset-border"] = Theme::cubeWithResetBorder();
    variables["--stop-solve-disabled-bg"]     = Theme::stopSolveDisabledBg();
    variables["--stop-solve-disabled-border"] = Theme::stopSolveDisabledBorder();
    variables["--stop-solve-disabled-text"]   = Theme::stopSolveDisabledText();

    // ========== SIDEBAR / MODAL COLORS ==========
    variables["--sidebar-bg"] = color(Theme::sidebarBg(false), Theme::sidebarBg(true));
    variables["--sidebar-border"] = color(Theme::sidebarBorder(false), Theme::sidebarBorder(true));
    variables["--modal-bg"] = color(Theme::modalBg(false), Theme::modalBg(true));
    variables["--modal-border"] = color(Theme::modalBorder(false), Theme::modalBorder(true));
    variables["--modal-overlay"] = color(Theme::modalOverlay(false), Theme::modalOverlay(true));

    // Load stylesheet from resource file
    QFile styleFile(":/stylesheet.qss");
    if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return ""; // Return empty if main stylesheet not found
    }
    QString rawStyle = styleFile.readAll();
    styleFile.close();

    // Now substitute all CSS variables in the stylesheet with their actual values
    QString finalStyle = rawStyle;
    for (auto it = variables.begin(); it != variables.end(); ++it) {
        finalStyle.replace(it.key(), it.value());
    }

    return finalStyle;
}
