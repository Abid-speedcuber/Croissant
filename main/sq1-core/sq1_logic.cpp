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
    {-5, -6}, {-4, -3}, {-3, -3}, {-2, -3}, {-1, 0}, {0, 0},
    {1, 0}, {2, 3}, {3, 3}, {4, 3}, {5, 6}, {6, 6}};

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

std::pair<int, int> getOverwork(const std::vector<std::string> &moves)
{
    std::vector<int> top, bot;
    for (auto &m : moves)
    {
        auto c = m.find(',');
        if (c == std::string::npos) { top.push_back(0); bot.push_back(0); continue; }
        try { top.push_back(std::stoi(m.substr(0, c))); } catch (...) { top.push_back(0); }
        try { bot.push_back(std::stoi(m.substr(c + 1))); } catch (...) { bot.push_back(0); }
    }

    int movement = 0, bonus = 0;
    int streak = 0, closestMovement = 0, buffer = 0;
    for (int t : top) {
        bool isLeft = (t == 6 || t < 0);
        if (isLeft) {
            streak++;
            auto it = CLOSEST_MAP.find(t);
            closestMovement += std::abs(it != CLOSEST_MAP.end() ? it->second : 0);
            buffer += std::abs(t);
            if (streak > 1 && closestMovement > 3) { movement += buffer; buffer = 0; }
        } else { streak = 0; closestMovement = 0; buffer = 0; }
    }
    streak = 0; closestMovement = 0; buffer = 0;
    for (int b : bot) {
        bool isLeft = (b > 0);
        if (isLeft) {
            streak++;
            auto it = CLOSEST_MAP.find(b);
            closestMovement += std::abs(it != CLOSEST_MAP.end() ? it->second : 0);
            buffer += std::abs(b);
            if (streak > 1 && closestMovement > 3) { movement += buffer; buffer = 0; }
        } else { streak = 0; closestMovement = 0; buffer = 0; }
    }
    for (size_t i = 0; i + 1 < top.size(); i++) {
        if (top[i] + top[i + 1] != 0) bonus++;
        if (bot[i] + bot[i + 1] != 0) bonus++;
    }
    return {movement, bonus};
}

// ============================================================
// Public API Functions
// ============================================================

AlgRating rateAlg(const std::string &algRaw, bool initial_top_A,
                  double W1, double W2, double W3, double W4, double W5)
{
    std::string a = algRaw;
    {
        size_t lb = a.find('[');
        if (lb != std::string::npos) a = a.substr(0, lb);
    }
    a = trimStr(a);
    bool isKarnAlg = false;
    for (char ch : a)
        if (std::isalpha(ch)) { isKarnAlg = true; break; }

    std::string numeric = isKarnAlg ? unkarnify(a) : replaceAll(a, " ", "");
    auto rawParts = splitStr(numeric, '/');
    std::vector<std::string> r;
    for (size_t i = 0; i < rawParts.size(); i++) {
        std::string pt = trimStr(rawParts[i]);
        if (i == 0 || !pt.empty()) r.push_back(pt);
    }
    if (r.size() < 2) return {W4, ""};

    int sliceCount = (int)r.size() - 1;
    if (sliceCount <= 0) return {W4, ""};

    double ergo_up = 0, ergo_down = 0;
    bool is_top_A = false, odd_slice = true;
    for (int i = 0; i < (int)r.size() - 1; i++) {
        if (i == 0) {
            auto c = r[i].find(',');
            int t = 0;
            if (c != std::string::npos)
                try { t = std::stoi(r[i].substr(0, c)); } catch (...) {}
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
            try { t = std::stoi(r[i].substr(0, c)); } catch (...) {}
        is_top_A = (is_top_A != (t % 3 != 0));
        odd_slice = !odd_slice;
    }
    double PHASE1 = W1 * std::max(ergo_up, ergo_down) / sliceCount;
    std::string sliceStart;
    if ((std::abs(ergo_up - ergo_down) / sliceCount) > 2)
        sliceStart = (ergo_up > ergo_down) ? "/" : "\\";
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
    if (!posHex.isEmpty()) {
        QChar first = posHex[0];
        initial_top_A = first.isDigit() || first == 'X' || first == 'Y' || first == 'Z';
    }

    const double W1 = 34, W2 = 100, W3 = 38, W4 = 500, W5 = 10;
    std::vector<std::pair<QString, double>> results;

    for (const QString &lineIn : solutionLines) {
        QString line = lineIn;
        std::string algStr = line.toStdString();
        auto bracket = algStr.find('[');
        std::string algOnly = bracket != std::string::npos
                                  ? trimStr(algStr.substr(0, bracket))
                                  : trimStr(algStr);
        double score = W4;
        AlgRating rating;
        bool rated = false;
        try {
            // Always rate on numeric alg — unkarnify if needed
            std::string numericAlg = algOnly;
            bool isKarn = false;
            for (char ch : algOnly)
                if (std::isalpha((unsigned char)ch)) { isKarn = true; break; }
            if (isKarn) numericAlg = unkarnify(algOnly);

            rating = rateAlg(numericAlg, initial_top_A, W1, W2, W3, W4, W5);
            score = rating.FINAL;
            rated = true;
        } catch (...) {}

        // Inject slice start indicator into display line
        if (rated) {
            QString sliceStr = QString::fromStdString(rating.sliceStart);
            if (sliceStr == "/" || sliceStr == "\\") {
                int slashPos = line.indexOf('/');
                if (slashPos >= 0)
                    line = line.left(slashPos) + sliceStr + line.mid(slashPos + 1);
                else {
                    int spacePos = line.indexOf(' ');
                    if (spacePos >= 0)
                        line = line.left(spacePos) + sliceStr + line.mid(spacePos + 1);
                }
            }
        }
        results.push_back({line, score});
    }
    std::sort(results.begin(), results.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });
    return results;
}
