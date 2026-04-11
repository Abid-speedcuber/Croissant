#include "sq1_logic.h"
#include "karnotation.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>
#include <string>
#include <map>

// ============================================================
// CONSTANTS & LOOKUP TABLES
// ============================================================

static const std::map<int, int> CLOSEST_MAP = {
    {-5, -6}, {-4, -3}, {-3, -3}, {-2, -3}, {-1, 0}, {0, 0}, {1, 0}, {2, 3}, {3, 3}, {4, 3}, {5, 6}, {6, 6}};

// MOVE_VALUES table from alg_rater.html.
// Key format: "<A|a><slash|backslash><top>,<bot>"  or  "<slash|backslash><top>,<bot>"
// A=aligned top (top%3==0), a=not. slash=upslice(odd), backslash=downslice(even).
static const std::map<std::string, int> MOVE_VALUES = {
    {"A/0,3", 16}, {"a/0,3", 5}, {"A\\0,3", 17}, {"a\\0,3", 5}, {"A/0,6", 1}, {"a/0,6", 5}, {"A\\0,6", 1}, {"a\\0,6", 5}, {"A/0,-3", 18}, {"a/0,-3", 12}, {"A\\0,-3", 8}, {"a\\0,-3", 5}, {"A/3,0", 16}, {"a/3,0", 17}, {"A\\3,0", 6}, {"a\\3,0", 16}, {"A/3,3", 12}, {"a/3,3", 10}, {"A\\3,3", 14}, {"a\\3,3", 11}, {"A/3,6", 0}, {"a/3,6", 5}, {"A\\3,6", 1}, {"a\\3,6", 4}, {"A/3,-3", 13}, {"a/3,-3", 7}, {"A\\3,-3", 12}, {"a\\3,-3", 6}, {"A/6,0", 12}, {"a/6,0", 4}, {"A\\6,0", 14}, {"a\\6,0", 4}, {"A/6,3", 11}, {"a/6,3", 2}, {"A\\6,3", 11}, {"a\\6,3", 2}, {"A/6,6", 2}, {"a/6,6", 0}, {"A\\6,6", 5}, {"a\\6,6", 0}, {"A/6,-3", 12}, {"a/6,-3", 3}, {"A\\6,-3", 8}, {"a\\6,-3", 1}, {"A/-3,0", 9}, {"a/-3,0", 18}, {"A\\-3,0", 11}, {"a\\-3,0", 15}, {"A/-3,3", 13}, {"a/-3,3", 12}, {"A\\-3,3", 14}, {"a\\-3,3", 10}, {"A/-3,6", 4}, {"a/-3,6", 7}, {"A\\-3,6", 6}, {"a\\-3,6", 2}, {"A/-3,-3", 12}, {"a/-3,-3", 11}, {"A\\-3,-3", 9}, {"a\\-3,-3", 5}, {"/1,-2", 4}, {"\\1,-2", 17}, {"/-1,2", 15}, {"\\-1,2", 14}, {"/1,-5", 3}, {"\\1,-5", 1}, {"/-1,5", 8}, {"\\-1,5", 3}, {"/1,4", 7}, {"\\1,4", 14}, {"/-1,-4", 12}, {"\\-1,-4", 9}, {"/1,1", 11}, {"\\1,1", 20}, {"/-1,-1", 20}, {"\\-1,-1", 10}, {"/2,-1", 20}, {"\\2,-1", 12}, {"/-2,1", 14}, {"\\-2,1", 18}, {"/2,2", 12}, {"\\2,2", 13}, {"/-2,-2", 14}, {"\\-2,-2", 8}, {"/2,5", 5}, {"\\2,5", 3}, {"/-2,-5", 4}, {"\\-2,-5", 3}, {"/2,-4", 14}, {"\\2,-4", 6}, {"/-2,4", 13}, {"\\-2,4", 13}, {"/4,4", 5}, {"\\4,4", 12}, {"/-4,-4", 12}, {"\\-4,-4", 4}, {"/4,1", 6}, {"\\4,1", 13}, {"/-4,-1", 16}, {"\\-4,-1", 6}, {"/4,-2", 12}, {"\\4,-2", 9}, {"/-4,2", 16}, {"\\-4,2", 13}, {"/4,-5", 2}, {"\\4,-5", 5}, {"/-4,5", 13}, {"\\-4,5", 3}, {"/5,5", 1}, {"\\5,5", 4}, {"/-5,-5", 2}, {"\\-5,-5", 0}, {"/5,2", 6}, {"\\5,2", 10}, {"/-5,-2", 12}, {"\\-5,-2", 13}, {"/5,-1", 11}, {"\\5,-1", 7}, {"/-5,1", 14}, {"\\-5,1", 15}, {"/5,-4", 2}, {"\\5,-4", 2}, {"/-5,4", 12}, {"\\-5,4", 14}};

// ============================================================
// Helper Functions
// ============================================================

int getMoveValue(bool startA, bool upslice, const std::string &move)
{
    std::string key;
    auto comma = move.find(',');
    int topVal = std::stoi(move.substr(0, comma));
    if (topVal % 3 == 0)
    {
        key = (startA ? "A" : "a");
        key += (upslice ? "/" : "\\");
        key += move;
    }
    else
    {
        key = (upslice ? "/" : "\\");
        key += move;
    }
    auto it = MOVE_VALUES.find(key);
    return (it != MOVE_VALUES.end()) ? it->second : 5;
}

std::vector<std::string> splitStr(const std::string &s, char delim)
{
    std::vector<std::string> out;
    std::string cur;
    for (char c : s)
    {
        if (c == delim)
        {
            out.push_back(cur);
            cur.clear();
        }
        else
            cur += c;
    }
    out.push_back(cur);
    return out;
}

std::string addCommasToMove(const std::string &move)
{
    if (move.empty())
        return move;
    for (char c : move)
        if (c != '-' && !std::isdigit((unsigned char)c))
            return move;
    switch (move.size())
    {
    case 1:
        return move + ",0";
    case 2:
        return move[0] == '-' ? move + ",0"
                              : std::string(1, move[0]) + "," + std::string(1, move[1]);
    case 3:
        return move[0] == '-' ? move.substr(0, 2) + "," + std::string(1, move[2])
                              : std::string(1, move[0]) + "," + move.substr(1);
    case 4:
        return move.substr(0, 2) + "," + move.substr(2);
    default:
        return move;
    }
}

std::string getAlignment(bool topA, bool bottomA)
{
    return (topA ? "1" : "0") + std::string(bottomA ? "-1" : "0");
}

std::string unkarnifyHelp(const std::string &scramble)
{
    std::string s = dictReplace(" " + scramble + " ", KARN_TO_WCA);
    // trim
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos)
        return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    s = s.substr(a, b - a + 1);
    // collapse consecutive slashes and spaces-around-slashes
    // replace runs of "/ /" or " /" etc. → single "/"
    // simple approach: replace multiple-slash runs
    std::string out;
    for (size_t i = 0; i < s.size();)
    {
        bool isSlashOrSpace = (s[i] == '/' || s[i] == ' ');
        if (isSlashOrSpace)
        {
            // scan ahead for the whole run
            size_t j = i;
            bool sawSlash = false;
            while (j < s.size() && (s[j] == '/' || s[j] == ' '))
            {
                if (s[j] == '/')
                    sawSlash = true;
                j++;
            }
            if (sawSlash)
                out += '/';
            else
                out += ' ';
            i = j;
        }
        else
        {
            out += s[i++];
        }
    }
    // replace remaining spaces with '/'
    for (char &c : out)
        if (c == ' ')
            c = '/';
    // collapse double slashes
    while (out.find("//") != std::string::npos)
        out = replaceAll(out, "//", "/");
    return out;
}

std::string replaceShorthands(const std::string &scrambleIn)
{
    // Fast path: if no alpha chars outside of numeric/slash context, skip
    bool hasAlpha = false;
    for (char c : scrambleIn)
        if (std::isalpha((unsigned char)c))
        {
            hasAlpha = true;
            break;
        }
    if (!hasAlpha)
        return unkarnifyHelp(scrambleIn);

    std::vector<std::string> moves = splitStr(scrambleIn, '/');

    bool topA = false, bottomA = false;
    std::string result = scrambleIn;

    for (const auto &move : moves)
    {
        std::string m = trimStr(move);
        if (m.empty())
            continue;

        if (m.find(',') != std::string::npos)
        {
            // Numeric turn — update alignment
            auto c = m.find(',');
            int t = 0;
            try
            {
                t = std::stoi(m.substr(0, c));
            }
            catch (...)
            {
            }
            int d = 0;
            try
            {
                d = std::stoi(m.substr(c + 1));
            }
            catch (...)
            {
            }
            if (t % 3 != 0)
                topA = !topA;
            if (d % 3 != 0)
                bottomA = !bottomA;
        }
        else
        {
            // Shorthand token
            std::string mLow = m;
            for (char &ch : mLow)
                ch = std::tolower((unsigned char)ch);

            std::string key;
            if (SHORTHAND_ALIGN_INDEPENDENT.count(mLow))
                key = mLow;
            else
                key = mLow + getAlignment(topA, bottomA);

            auto it = SHORTHAND_TO_KARN.find(key);
            if (it == SHORTHAND_TO_KARN.end())
            {
                // Unknown shorthand — return as-is (runtime-safe)
                return scrambleIn;
            }
            std::string repl = it->second;
            result = replaceAll(result, m, repl);

            // Update alignment based on replacement expansion
            std::string expanded = repl;
            if (!expanded.empty() && expanded.front() == '/')
                expanded = expanded.substr(1);
            if (!expanded.empty() && expanded.back() == '/')
                expanded.pop_back();
            for (const auto &sub : splitStr(unkarnifyHelp(expanded), '/'))
            {
                if (sub.empty())
                    continue;
                auto c2 = sub.find(',');
                if (c2 == std::string::npos)
                    continue;
                int t = 0, d = 0;
                try
                {
                    t = std::stoi(sub.substr(0, c2));
                }
                catch (...)
                {
                }
                try
                {
                    d = std::stoi(sub.substr(c2 + 1));
                }
                catch (...)
                {
                }
                if (t % 3 != 0)
                    topA = !topA;
                if (d % 3 != 0)
                    bottomA = !bottomA;
            }
        }
    }

    // Collapse double-slashes introduced by replacements
    result = replaceAll(result, " / ", "/");
    result = replaceAll(result, "  ", "/");
    while (result.find("//") != std::string::npos)
        result = replaceAll(result, "//", "/");
    return unkarnifyHelp(result);
}

std::pair<int, int> getOverwork(const std::vector<std::string> &moves)
{
    std::vector<int> top, bot;
    for (auto &m : moves)
    {
        auto c = m.find(',');
        if (c == std::string::npos)
        {
            top.push_back(0);
            bot.push_back(0);
            continue;
        }
        try
        {
            top.push_back(std::stoi(m.substr(0, c)));
        }
        catch (...)
        {
            top.push_back(0);
        }
        try
        {
            bot.push_back(std::stoi(m.substr(c + 1)));
        }
        catch (...)
        {
            bot.push_back(0);
        }
    }

    int movement = 0, bonus = 0;
    int streak = 0, closestMovement = 0, buffer = 0;
    for (int t : top)
    {
        bool isLeft = (t == 6 || t < 0);
        if (isLeft)
        {
            streak++;
            auto it = CLOSEST_MAP.find(t);
            closestMovement += std::abs(it != CLOSEST_MAP.end() ? it->second : 0);
            buffer += std::abs(t);
            if (streak > 1 && closestMovement > 3)
            {
                movement += buffer;
                buffer = 0;
            }
        }
        else
        {
            streak = 0;
            closestMovement = 0;
            buffer = 0;
        }
    }
    streak = 0;
    closestMovement = 0;
    buffer = 0;
    for (int b : bot)
    {
        bool isLeft = (b > 0);
        if (isLeft)
        {
            streak++;
            auto it = CLOSEST_MAP.find(b);
            closestMovement += std::abs(it != CLOSEST_MAP.end() ? it->second : 0);
            buffer += std::abs(b);
            if (streak > 1 && closestMovement > 3)
            {
                movement += buffer;
                buffer = 0;
            }
        }
        else
        {
            streak = 0;
            closestMovement = 0;
            buffer = 0;
        }
    }
    for (size_t i = 0; i + 1 < top.size(); i++)
    {
        if (top[i] + top[i + 1] != 0)
            bonus++;
        if (bot[i] + bot[i + 1] != 0)
            bonus++;
    }
    return {movement, bonus};
}

// ============================================================
// Public API Functions
// ============================================================

std::string unkarnify(const std::string &algIn)
{
    std::string s = algIn;

    // Easter egg passthrough
    if (s.find("meow") != std::string::npos)
        return s;

    // Legacy single-char substitutions (compact notation)
    s = replaceAll(s, "&", "-1");
    s = replaceAll(s, "^", "-2");
    s = replaceAll(s, "9", "-3");
    s = replaceAll(s, "8", "-4");
    s = replaceAll(s, "7", "-5");

    // Detect leading/trailing slice
    bool firstSlice = (!s.empty() && (s[0] == '/' || s[0] == '\\'));
    if (!firstSlice)
    {
        // Check if first token is a karn name that maps to something starting with /
        std::istringstream iss(s);
        std::string tok;
        if (iss >> tok)
        {
            auto it = KARN_TO_WCA.find(" " + tok + " ");
            if (it != KARN_TO_WCA.end())
                firstSlice = true;
        }
    }
    bool lastSlice = false;
    {
        std::istringstream iss(s);
        std::string last, tok;
        while (iss >> tok)
            last = tok;
        if (!last.empty())
        {
            auto it = KARN_TO_WCA.find(" " + last + " ");
            if (it != KARN_TO_WCA.end())
                lastSlice = true;
        }
    }

    // Normalise separators
    for (char &c : s)
        if (c == '\\' || c == '/')
            c = ' ';
    // collapse parens
    s = replaceAll(s, "(", "");
    s = replaceAll(s, ")", "");
    // collapse multiple spaces
    {
        std::string tmp;
        bool sp = false;
        for (char c : s)
        {
            if (c == ' ')
            {
                if (!sp)
                {
                    tmp += ' ';
                    sp = true;
                }
            }
            else
            {
                tmp += c;
                sp = false;
            }
        }
        s = trimStr(tmp);
    }

    // addCommas to each space-separated token
    {
        std::vector<std::string> tokens;
        std::istringstream iss(s);
        std::string tok;
        while (iss >> tok)
            tokens.push_back(tok);
        s.clear();
        for (size_t i = 0; i < tokens.size(); i++)
        {
            if (i)
                s += ' ';
            s += addCommasToMove(tokens[i]);
        }
    }

    // replaceShorthands then full dict-replace
    std::string final_ = replaceShorthands(unkarnifyHelp(s));

    // Re-attach leading/trailing slices
    if (firstSlice && (final_.empty() || final_[0] != '/'))
        final_ = "/" + final_;
    if (lastSlice && (final_.empty() || final_.back() != '/'))
        final_ = final_ + "/";
    // collapse double slashes
    while (final_.find("//") != std::string::npos)
        final_ = replaceAll(final_, "//", "/");

    // addCommas pass on each slash-segment
    auto parts = splitStr(final_, '/');
    final_.clear();
    for (size_t i = 0; i < parts.size(); ++i)
    {
        if (i)
            final_ += "/";
        final_ += addCommasToMove(parts[i]);
    }

    return final_;
}

AlgRating rateAlg(const std::string &algRaw, bool initial_top_A,
                  double W1, double W2, double W3, double W4, double W5)
{
    std::string a = algRaw;
    {
        size_t lb = a.find('[');
        if (lb != std::string::npos)
            a = a.substr(0, lb);
    }
    a = trimStr(a);
    bool isKarnAlg = false;
    for (char ch : a)
        if (std::isalpha(ch))
        {
            isKarnAlg = true;
            break;
        }
    std::string numeric = isKarnAlg ? unkarnify(a) : replaceAll(a, " ", "");
    auto rawParts = splitStr(numeric, '/');
    std::vector<std::string> r;
    for (size_t i = 0; i < rawParts.size(); i++)
    {
        std::string pt = trimStr(rawParts[i]);
        if (i == 0 || !pt.empty())
            r.push_back(pt);
    }
    if (r.size() < 2)
        return {W4, ""};

    int sliceCount = (int)r.size() - 1;
    if (sliceCount <= 0)
        return {W4, ""};

    double ergo_up = 0, ergo_down = 0;
    bool is_top_A = false, odd_slice = true;
    for (int i = 0; i < (int)r.size() - 1; i++)
    {
        if (i == 0)
        {
            auto c = r[i].find(',');
            int t = 0;
            if (c != std::string::npos)
                try
                {
                    t = std::stoi(r[i].substr(0, c));
                }
                catch (...)
                {
                }
            is_top_A = (initial_top_A != (t % 3 != 0));
            odd_slice = true;
            continue;
        }
        int vu = getMoveValue(is_top_A, odd_slice, r[i]);
        int vd = getMoveValue(is_top_A, !odd_slice, r[i]);
        ergo_up += vu;
        ergo_down += vd;
        auto c = r[i].find(',');
        int t = 0;
        if (c != std::string::npos)
            try
            {
                t = std::stoi(r[i].substr(0, c));
            }
            catch (...)
            {
            }
        is_top_A = (is_top_A != (t % 3 != 0));
        odd_slice = !odd_slice;
    }
    double PHASE1 = W1 * std::max(ergo_up, ergo_down) / sliceCount;
    std::string sliceStart;
    if ((std::abs(ergo_up - ergo_down) / sliceCount) > 2)
    {
        sliceStart = (ergo_up > ergo_down) ? "/" : "\\";
    }
    else
        sliceStart = " ";

    double PHASE2 = W2 * sliceCount;
    auto moves = std::vector<std::string>(r.begin() + 1, r.end() - 1);
    auto [movement, bonus] = getOverwork(moves);
    double PHASE3 = W3 * movement / sliceCount;
    double PHASE4 = bonus * W5 / sliceCount;

    double FINAL = PHASE1 - PHASE2 - PHASE3 + PHASE4 + W4;
    return {FINAL, sliceStart};
}

std::vector<std::pair<QString, double>>
rateAndSort(const QStringList &solutionLines, const QString &posHex, bool useKarnotation)
{
    Q_UNUSED(useKarnotation);
    bool initial_top_A = false;
    if (!posHex.isEmpty())
    {
        QChar first = posHex[0];
        initial_top_A = first.isDigit() ||
                        first == 'X' || first == 'Y' || first == 'Z';
    }

    const double W1 = 34, W2 = 100, W3 = 38, W4 = 500, W5 = 10;
    std::vector<std::pair<QString, double>> results;

    for (const QString &lineIn : solutionLines)
    {
        QString line = lineIn;
        std::string algStr = line.toStdString();
        auto bracket = algStr.find('[');
        std::string algOnly = bracket != std::string::npos
                                  ? trimStr(algStr.substr(0, bracket))
                                  : trimStr(algStr);
        double score = W4;
        AlgRating rating;
        bool rated = false;
        try
        {
            // Always convert to numeric for rating
            std::string numericAlg = algOnly;
            bool isKarn = false;
            for (char ch : algOnly)
                if (std::isalpha((unsigned char)ch))
                {
                    isKarn = true;
                    break;
                }
            if (isKarn)
                numericAlg = unkarnify(algOnly);

            rating = rateAlg(numericAlg, initial_top_A, W1, W2, W3, W4, W5);
            score = rating.FINAL;
            rated = true;
        }
        catch (...)
        {
        }

        // Inject slice start indicator into display line
        if (rated)
        {
            QString sliceStr = QString::fromStdString(rating.sliceStart);
            if (sliceStr == "/" || sliceStr == "\\")
            {
                // Find the first '/' in the line (can only be in the alg part)
                int slashPos = line.indexOf('/');
                if (slashPos >= 0)
                    line = line.left(slashPos) + sliceStr + line.mid(slashPos + 1);
                else
                {
                    int spacePos = line.indexOf(' ');
                    if (spacePos >= 0)
                        line = line.left(spacePos) + sliceStr + line.mid(spacePos + 1);
                }
            }
        }

        results.push_back({line, score});
    }
    std::sort(results.begin(), results.end(),
              [](const auto &a, const auto &b)
              { return a.second > b.second; });
    return results;
}
