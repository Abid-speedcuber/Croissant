#include "output-converter.h"
#include "karnotation.h"
#include <QString>

QString OutputConverter::convert(const QString& rawLine, OutputMode mode) {
    if (mode == OutputMode::Raw)
        return rawLine;

    // Only process solution lines (contain [...])
    int lb = rawLine.lastIndexOf('[');
    int rb = rawLine.lastIndexOf(']');
    if (lb < 0 || rb < 0)
        return rawLine;

    QString algPart     = rawLine.left(lb).trimmed();
    QString bracketPart = rawLine.mid(lb); // "[x|y] " etc

    if (mode == OutputMode::Karnotation) {
        std::string converted = karnify(algPart.toStdString());
        return QString::fromStdString(converted) + "  " + bracketPart.trimmed();
    }

    return rawLine;
}
