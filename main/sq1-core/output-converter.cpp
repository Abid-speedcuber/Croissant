#include "output-converter.h"
#include "karnotation.h"
#include <string>
#include <QString>

static std::string applyKarnotation(const std::string& algPart) {
    std::string out = algPart;

    // Strip leading/trailing whitespace
    out = trimStr(out);

    // Determine if alg starts with a slice
    bool startSlice = !out.empty() && (out.front() == '/' || out.front() == '\\');

    // Replace "/" and "\" with spaces for dict replacement
    out = replaceAll(out, "/", " ");
    out = replaceAll(out, "\\", " ");

    // Apply WCA_TO_KARN replacements
    out = replaceWithVector(" " + trimStr(out) + " ", WCA_TO_KARN);
    out = trimStr(out);

    // Collapse multiple spaces
    std::string prev;
    do {
        prev = out;
        out = replaceAll(out, "  ", " ");
    } while (out != prev);

    // Remove commas
    out = replaceAll(out, ",", "");

    if (startSlice) out = "/" + out;
    return out;
}

QString OutputConverter::convert(const QString& rawLine, OutputMode mode) {
    if (mode == OutputMode::Raw)
        return rawLine;

    // Only process solution lines (contain [...])
    int lb = rawLine.lastIndexOf('[');
    int rb = rawLine.lastIndexOf(']');
    if (lb < 0 || rb < 0)
        return rawLine;

    QString algPart   = rawLine.left(lb).trimmed();
    QString bracketPart = rawLine.mid(lb); // "[x|y] " etc

    if (mode == OutputMode::Karnotation) {
        std::string converted = applyKarnotation(algPart.toStdString());
        return QString::fromStdString(converted) + "  " + bracketPart.trimmed();
    }

    return rawLine;
}