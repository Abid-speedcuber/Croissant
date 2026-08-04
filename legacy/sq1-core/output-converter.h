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
    // Inverse of abidifyDisplay: map Kompact PUA glyphs back to ASCII (digits and
    // '-'). Non-PUA characters pass through unchanged, so it's a no-op on plain
    // ASCII. Negatives come back as "-d" (the comma-collapsed/karn-compact form).
    static QString deabidify(const QString& display);
};