#include "output-converter.h"
#include "karnotation.h"
#include <QString>
#include <QStringList>
#include <QFontDatabase>
#include <QRegularExpression>
#include <functional>

QString OutputConverter::s_abidFontFamily;

void OutputConverter::loadAbidFont()
{
    int id = QFontDatabase::addApplicationFont(":/kompact-font.ttf");
    if (id != -1) {
        QStringList families = QFontDatabase::applicationFontFamilies(id);
        if (!families.isEmpty())
            s_abidFontFamily = families.first();
    }
}

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

QString OutputConverter::abidifyDisplay(const QString& algOnly)
{
    if (s_abidFontFamily.isEmpty() || algOnly.isEmpty())
        return algOnly;

    // Codepoint helpers (values 0-6 only; square-1 never exceeds 6)
    auto normalCp  = [](int d) -> QChar { return QChar(0xe000 + d); };
    auto singleBar = [](int d) -> QChar { return QChar(0xe006 + d); }; // 1→E007…5→E00B
    auto barRight  = [](int d) -> QChar { return QChar(0xe00b + d); }; // 1→E00C…5→E010
    auto barLeft   = [](int d) -> QChar { return QChar(0xe010 + d); }; // 1→E011…5→E015

    auto mapDigits = [&](int absVal, std::function<QChar(int)> mapper) -> QString {
        QString s;
        for (QChar c : QString::number(absVal))
            if (c.isDigit()) s += mapper(c.digitValue());
        return s;
    };

    if (algOnly.contains(',')) {
        // ── WCA format: find every a,b token ─────────────────────────────────
        static const QRegularExpression pairRe(R"((-?\d+),(-?\d+))");
        QString result;
        int last = 0;
        auto it = pairRe.globalMatch(algOnly);
        while (it.hasNext()) {
            auto m = it.next();
            // Pass through non-numeric content (slashes, slice indicators, spaces)
            result += algOnly.mid(last, m.capturedStart() - last);
            int a = m.captured(1).toInt();
            int b = m.captured(2).toInt();
            if (a < 0 && b < 0) {
                result += mapDigits(qAbs(a), barRight);
                result += mapDigits(qAbs(b), barLeft);
            } else {
                result += (a < 0) ? mapDigits(qAbs(a), singleBar)
                                  : mapDigits(qAbs(a), normalCp);
                result += (b < 0) ? mapDigits(qAbs(b), singleBar)
                                  : mapDigits(qAbs(b), normalCp);
            }
            last = m.capturedEnd();
        }
        result += algOnly.mid(last);
        return result;
    } else {
        // ── Karn / stripped-comma format ──────────────────────────────────────
        QString result;
        int i = 0;
        while (i < algOnly.size()) {
            QChar c = algOnly[i];
            if (c == '-' && i + 1 < algOnly.size() && algOnly[i + 1].isDigit()) {
                bool bothNeg = (i + 2 < algOnly.size() && algOnly[i + 2] == '-' &&
                                i + 3 < algOnly.size() && algOnly[i + 3].isDigit());
                if (bothNeg) {
                    result += barRight(algOnly[i + 1].digitValue());
                    i += 2;
                    result += barLeft(algOnly[i + 1].digitValue());
                    i += 2;
                } else {
                    result += singleBar(algOnly[i + 1].digitValue());
                    i += 2;
                }
            } else if (c.isDigit()) {
                result += normalCp(c.digitValue());
                ++i;
            } else {
                result += c;
                ++i;
            }
        }
        return result;
    }
}

QString OutputConverter::deabidify(const QString& display)
{
    // PUA layout (see abidifyDisplay): normal digits 0-6 at E000-E006; single-bar
    // negatives 1-5 at E007-E00B; both-negative pair halves at E00C-E010 (first)
    // and E011-E015 (second). All barred glyphs decode to "-d".
    QString out;
    for (QChar c : display) {
        ushort u = c.unicode();
        if (u >= 0xe000 && u <= 0xe006)
            out += QChar('0' + (u - 0xe000));
        else if (u >= 0xe007 && u <= 0xe00b)
            out += '-', out += QChar('0' + (u - 0xe006));
        else if (u >= 0xe00c && u <= 0xe010)
            out += '-', out += QChar('0' + (u - 0xe00b));
        else if (u >= 0xe011 && u <= 0xe015)
            out += '-', out += QChar('0' + (u - 0xe010));
        else
            out += c;
    }
    return out;
}
