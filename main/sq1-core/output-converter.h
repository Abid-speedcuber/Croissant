#pragma once
#include <QString>

enum class OutputMode {
    Raw,
    Karnotation
};

class OutputConverter {
public:
    static QString convert(const QString& rawLine, OutputMode mode);
};