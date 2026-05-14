#pragma once
#include <QString>

enum class OutputMode {
    Raw,
    Karnotation
};

class OutputConverter {
public:
    static QString s_abidFontFamily;

    static QString convert(const QString& rawLine, OutputMode mode);
    static void loadAbidFont();
    static QString abidifyDisplay(const QString& algOnly);
};