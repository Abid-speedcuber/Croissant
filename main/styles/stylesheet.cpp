#include "stylesheet.h"
#include "theme.h"
#include <QString>
#include <QFile>
#include <QMap>

QString buildStyleSheet() {
    QMap<QString, QString> variables;

    variables["--primary-bg"]   = Theme::primaryBg();
    variables["--secondary-bg"] = Theme::secondaryBg();
    variables["--tertiary-bg"]  = Theme::tertiaryBg();
    variables["--dark-bg"]      = Theme::darkBg();
    variables["--disabled-bg"]  = Theme::disabledBg();
    variables["--hover-bg"]     = Theme::hoverBg();
    variables["--pressed-bg"]   = Theme::pressedBg();

    variables["--text-primary"]   = Theme::textPrimary();
    variables["--text-secondary"] = Theme::textSecondary();
    variables["--text-muted"]     = Theme::textMuted();
    variables["--text-disabled"]  = Theme::textDisabled();
    variables["--text-error"]     = Theme::textError();
    variables["--text-cyan"]      = Theme::textCyan();
    variables["--text-terminal"]  = Theme::textTerminal();
    variables["--text-solution"]  = Theme::textSolution();

    variables["--border-light"]  = Theme::borderLight();
    variables["--border-dark"]   = Theme::borderDark();
    variables["--border-accent"] = Theme::borderAccent();
    variables["--border-group"]  = Theme::borderGroup();
    variables["--border-bottom"] = Theme::borderBottom();

    variables["--checkbox-bg"]           = Theme::checkboxBg();
    variables["--checkbox-check-blue"]   = Theme::checkboxCheckBlue();
    variables["--checkbox-check-orange"] = Theme::checkboxCheckOrange();
    variables["--checkbox-border"]       = Theme::checkboxBorder();

    variables["--button-bg"]             = Theme::buttonBg();
    variables["--button-border"]         = Theme::buttonBorder();
    variables["--button-text"]           = Theme::buttonText();
    variables["--button-secondary-text"] = Theme::buttonSecondaryText();

    variables["--button-solve-bg"]     = Theme::buttonSolveBg();
    variables["--button-solve-border"] = Theme::buttonSolveBorder();
    variables["--button-solve-hover"]  = Theme::buttonSolveHover();

    variables["--topbar-bg"] = Theme::topBarBg();

    variables["--input-mode-bg"]     = Theme::inputModeBg();
    variables["--input-mode-border"] = Theme::inputModeBorder();
    variables["--input-mode-hover"]  = Theme::inputModeHover();
    variables["--input-mode-text"]   = Theme::inputModeText();

    variables["--input-arrow-bg"]     = Theme::inputArrowBg();
    variables["--input-arrow-border"] = Theme::inputArrowBorder();
    variables["--input-arrow-hover"]  = Theme::inputArrowHover();

    variables["--input-apply-bg"]        = Theme::inputApplyBg();
    variables["--input-apply-border"]    = Theme::inputApplyBorder();
    variables["--input-apply-hover"]     = Theme::inputApplyHover();
    variables["--input-apply-text"]      = Theme::inputApplyText();
    variables["--input-apply-hover-text"]= Theme::inputApplyHoverText();

    variables["--input-field-bg"]     = Theme::inputFieldBg();
    variables["--input-field-border"] = Theme::inputFieldBorder();
    variables["--input-field-text"]   = Theme::inputFieldText();
    variables["--input-panel-bg"]     = Theme::inputPanelBg();

    variables["--button-reset-bg"]     = Theme::buttonResetBg();
    variables["--button-reset-border"] = Theme::buttonResetBorder();
    variables["--button-reset-hover"]  = Theme::buttonResetHover();

    variables["--button-stop-bg"]     = Theme::buttonStopBg();
    variables["--button-stop-border"] = Theme::buttonStopBorder();
    variables["--button-stop-hover"]  = Theme::buttonStopHover();
    variables["--button-stop-text"]   = Theme::buttonStopText();

    variables["--button-util-bg"]           = Theme::buttonUtilBg();
    variables["--button-util-border"]       = Theme::buttonUtilBorder();
    variables["--button-util-text"]         = Theme::buttonUtilText();
    variables["--button-util-hover-bg"]     = Theme::buttonUtilHoverBg();
    variables["--button-util-hover-border"] = Theme::buttonUtilHoverBorder();
    variables["--button-util-hover-text"]   = Theme::buttonUtilHoverText();

    variables["--button-about-bg"]           = Theme::buttonAboutBg();
    variables["--button-about-border"]       = Theme::buttonAboutBorder();
    variables["--button-about-text"]         = Theme::buttonAboutText();
    variables["--button-about-hover-bg"]     = Theme::buttonAboutHoverBg();
    variables["--button-about-hover-border"] = Theme::buttonAboutHoverBorder();
    variables["--button-about-hover-text"]   = Theme::buttonAboutHoverText();

    variables["--progress-bg"]   = Theme::progressBg();
    variables["--progress-fill"] = Theme::progressFill();
    variables["--status-text"]   = Theme::statusText();

    variables["--scrollbar-bg"]     = Theme::scrollbarBg();
    variables["--scrollbar-handle"] = Theme::scrollbarHandle();
    variables["--scrollbar-hover"]  = Theme::scrollbarHover();

    variables["--table-bg"]            = Theme::tableBg();
    variables["--table-border"]        = Theme::tableBorder();
    variables["--table-header-bg"]     = Theme::tableHeaderBg();
    variables["--table-header-text"]   = Theme::tableHeaderText();
    variables["--table-selected-bg"]   = Theme::tableSelectedBg();
    variables["--table-selected-text"] = Theme::tableSelectedText();

    variables["--row-alt-dark"]  = Theme::rowAltDark();
    variables["--row-alt-light"] = Theme::rowAltLight();

    variables["--tooltip-bg"]     = Theme::tooltipBg();
    variables["--tooltip-text"]   = Theme::tooltipText();
    variables["--tooltip-border"] = Theme::tooltipBorder();

    variables["--cube-top-face"]          = Theme::cubeTopFace();
    variables["--cube-bot-face"]          = Theme::cubeBotFace();
    variables["--cube-red"]               = Theme::cubeRed();
    variables["--cube-blue"]              = Theme::cubeBlue();
    variables["--cube-orange"]            = Theme::cubeOrange();
    variables["--cube-green"]             = Theme::cubeGreen();
    variables["--cube-partial"]           = Theme::cubePartial();
    variables["--cube-border"]            = Theme::cubeBorder();
    variables["--cube-selection"]         = Theme::cubeSelection();
    variables["--canvas-bg"]              = Theme::canvasBg();
    variables["--cube-with-reset-border"] = Theme::cubeWithResetBorder();
    variables["--stop-solve-disabled-bg"]     = Theme::stopSolveDisabledBg();
    variables["--stop-solve-disabled-border"] = Theme::stopSolveDisabledBorder();
    variables["--stop-solve-disabled-text"]   = Theme::stopSolveDisabledText();

    variables["--radio-pill-bg"]            = Theme::radioPillBg();
    variables["--radio-btn-text"]           = Theme::radioBtnText();
    variables["--radio-btn-checked-bg"]     = Theme::radioBtnCheckedBg();
    variables["--radio-btn-checked-text"]   = Theme::radioBtnCheckedText();
    variables["--radio-btn-checked-border"] = Theme::radioBtnCheckedBorder();
    variables["--radio-btn-hover-bg"]       = Theme::radioBtnHoverBg();
    variables["--sidebar-bg"]     = Theme::sidebarBg();
    variables["--sidebar-border"] = Theme::sidebarBorder();
    variables["--modal-bg"]       = Theme::modalBg();
    variables["--modal-border"]   = Theme::modalBorder();
    variables["--modal-overlay"]  = Theme::modalOverlay();

    QFile styleFile(":/stylesheet.qss");
    if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return "";
    QString finalStyle = styleFile.readAll();
    styleFile.close();

    for (auto it = variables.begin(); it != variables.end(); ++it)
        finalStyle.replace(it.key(), it.value());

    return finalStyle;
}
