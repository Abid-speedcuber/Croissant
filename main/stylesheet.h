#pragma once
#include <QString>

class MainWindow;

/**
 * Build the complete stylesheet for the application.
 * This function generates a large, theme-aware stylesheet string.
 *
 * @param lightTheme Whether to use the light theme (true) or dark theme (false)
 * @return The complete stylesheet as a QString
 */
QString buildStyleSheet(bool lightTheme);
