#pragma once
// Used by sq1opt.cpp (alg printing) and mainwindow.cpp (unkarnify / ergonomics rating).
// Edit this file once to affect both.
// Public API (all inline):
//   karnify(algWCASlash)        — WCA numeric slash-format → Karnotation display string
//   unkarnify(algIn)            — Karnotation / shorthand → WCA numeric slash-format
//   karnifycs(alg, stateHex, generatorMode)
#include <string>
#include <map>
#include <unordered_map>
#include <set>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>
#include <stdexcept>

// ---------------------------------------------------------------------------
// KARN_TO_WCA  — Karnotation name (space-padded) -> numeric WCA slash format
// ---------------------------------------------------------------------------
static const std::map<std::string, std::string> KARN_TO_WCA = {
    {"U4", "U U' U U'"}, {"U4'", "U' U U' U"},
    {"D4", "D D' D D'"}, {"D4'", "D' D D' D"},
    {"u4", "u u' u u'"}, {"u4'", "u' u u' u"},
    {"d4", "d d' d d'"}, {"d4'", "d' d d' d"},

    {"U3", "U U' U"}, {"U3'", "U' U U'"},
    {"D3", "D D' D"}, {"D3'", "D' D D'"},
    {"u3", "u u' u"}, {"u3'", "u' u u'"},
    {"d3", "d d' d"}, {"d3'", "d' d d'"},
    {"F3", "F F' F"}, {"F3'", "F' F F'"},
    {"f3", "f f' f"}, {"f3'", "f' f f'"},

    {"W", "U U'"}, {"W'", "U' U"},
    {"B", "D D'"}, {"B'", "D' D"},
    {"w", "u u'"}, {"w'", "u' u"},
    {"b", "d d'"}, {"b'", "d' d"},
    {"F2", "F F'"}, {"F2'", "F' F"},
    {"f2", "f f'"}, {"f2'", "f' f"},
    {"UU", "U U"}, {"UU'", "U' U'"},
    {"DD", "D D"}, {"DD'", "D' D'"},
    {"T2", "T T'"}, {"T2'", "T' T"},
    {"t2", "t t'"}, {"t2'", "t' t"},
    {"E2", "E E'"}, {"E2'", "E' E"},
    {"\xc9\x87", "U D"}, {"\xc9\x87'", "U' D'"},
    {"\xc9\x86", "U D'"}, {"\xc9\x86'", "U' D"},

    {"U2", "6,0"}, {"U2'", "6,0"},
    {"D2", "0,6"},
    {"U2D", "6,3"}, {"U2D'", "6,-3"},
    {"U2'D", "6,3"}, {"U2'D'", "6,-3"},
    {"U2D2", "6,6"},
    {"UD2", "3,6"}, {"U'D2", "-3,6"},

    {"U", "3,0"}, {"U'", "-3,0"},
    {"D", "0,3"}, {"D'", "0,-3"},
    {"E", "3,-3"}, {"E'", "-3,3"},
    {"e", "3,3"}, {"e'", "-3,-3"},
    {"u", "2,-1"}, {"u'", "-2,1"},
    {"d", "-1,2"}, {"d'", "1,-2"},
    {"F", "4,1"}, {"F'", "-4,-1"},
    {"f", "1,4"}, {"f'", "-1,-4"},
    {"T", "2,-4"}, {"T'", "-2,4"},
    {"t", "4,-2"}, {"t'", "-4,2"},
    {"m", "2,2"}, {"m'", "-2,-2"},
    {"M", "1,1"}, {"M'", "-1,-1"},
    {"u2", "5,-1"}, {"u2'", "-5,1"},
    {"d2", "-1,5"}, {"d2'", "1,-5"},
    {"K", "5,2"}, {"K'", "-5,-2"},
    {"k", "2,5"}, {"k'", "-2,-5"},
};

// ---------------------------------------------------------------------------
// WCA_TO_KARN  — numeric WCA slash format (space-padded) -> Karnotation name
// Used when cube IS in cubeshape.
// ---------------------------------------------------------------------------
static const std::vector<std::pair<std::string, std::string>> WCA_TO_KARN = {
    {" 6,0 ", " U2 "},
    {" 6,3 ", " U2D "},
    {" 6,-3 ", " U2D' "},
    {" 6,6 ", " U2D2 "},
    {" 0,6 ", " D2 "},
    {" 3,6 ", " UD2 "},
    {" -3,6 ", " U'D2 "},
    {" 3,0 ", " U "},
    {" -3,0 ", " U' "},
    {" 0,3 ", " D "},
    {" 0,-3 ", " D' "},
    {" 3,-3 ", " E "},
    {" -3,3 ", " E' "},
    {" 3,3 ", " e "},
    {" -3,-3 ", " e' "},
    {" 2,-1 ", " u "},
    {" -2,1 ", " u' "},
    {" -1,2 ", " d "},
    {" 1,-2 ", " d' "},
    {" 4,1 ", " F "},
    {" -4,-1 ", " F' "},
    {" 1,4 ", " f "},
    {" -1,-4 ", " f' "},
    {" 2,-4 ", " T "},
    {" -2,4 ", " T' "},
    {" 4,-2 ", " t "},
    {" -4,2 ", " t' "},
    {" 2,2 ", " m "},
    {" -2,-2 ", " m' "},
    {" 1,1 ", " M "},
    {" -1,-1 ", " M' "},
    {" 5,-1 ", " u2 "},
    {" -5,1 ", " u2' "},
    {" -1,5 ", " d2 "},
    {" 1,-5 ", " d2' "},
    {" 5,2 ", " K "},
    {" -5,-2 ", " K' "},
    {" 2,5 ", " k "},
    {" -2,-5 ", " k' "},
};

static const std::vector<std::pair<std::string, std::string>> KARN_TO_HIGHKARN = {
    {" U U' U U' ", " U4 "},
    {" U' U U' U ", " U4' "},
    {" D D' D D' ", " D4 "},
    {" D' D D' D ", " D4' "},
    {" u u' u u' ", " u4 "},
    {" u' u u' u ", " u4' "},
    {" d d' d d' ", " d4 "},
    {" d' d d' d ", " d4' "},
    {" U U' U ", " U3 "},
    {" U' U U' ", " U3' "},
    {" D D' D ", " D3 "},
    {" D' D D' ", " D3' "},
    {" u u' u ", " u3 "},
    {" u' u u' ", " u3' "},
    {" d d' d ", " d3 "},
    {" d' d d' ", " d3' "},
    {" F F' F ", " F3 "},
    {" F' F F' ", " F3' "},
    {" f f' f ", " f3 "},
    {" f' f f' ", " f3' "},
    {" U U' ", " W "},
    {" U' U ", " W' "},
    {" D D' ", " B "},
    {" D' D ", " B' "},
    {" u u' ", " w "},
    {" u' u ", " w' "},
    {" d d' ", " b "},
    {" d' d ", " b' "},
    {" F F' ", " F2 "},
    {" F' F ", " F2' "},
    {" f f' ", " f2 "},
    {" f' f ", " f2' "},
    {" U U ", " UU "},
    {" U' U' ", " UU' "},
    {" D D ", " DD "},
    {" D' D' ", " DD' "},
};

// ---------------------------------------------------------------------------
// WCA_TO_KARN_OCS  — WCA -> Karn replacements used when OUT of cubeshape.
// Empty for now; define entries here when OCS karnotation names are decided.
// ---------------------------------------------------------------------------
static const std::vector<std::pair<std::string, std::string>> WCA_TO_KARN_OCS = {
    {" 6,0 ", " U2 "},
    {" 6,3 ", " U2D "},
    {" 6,-3 ", " U2D' "},
    {" 6,6 ", " U2D2 "},
    {" 0,6 ", " D2 "},
    {" 3,6 ", " UD2 "},
    {" -3,6 ", " U'D2 "},
    {" 3,0 ", " U "},
    {" -3,0 ", " U' "},
    {" 0,3 ", " D "},
    {" 0,-3 ", " D' "},
    {" 3,-3 ", " E "},
    {" -3,3 ", " E' "},
    {" 3,3 ", " e "},
    {" -3,-3 ", " e' "},
    {" -2,1 ", " u' "},
    {" 2,-1 ", " u "},
};

static const std::vector<std::pair<std::string, std::string>> KARN_TO_HIGHKARN_OCS = {
    {" U U' U U' ", " U4 "},
    {" U' U U' U ", " U4' "},
    {" D D' D D' ", " D4 "},
    {" D' D D' D ", " D4' "},
    {" U U' U ", " U3 "},
    {" U' U U' ", " U3' "},
    {" D D' D ", " D3 "},
    {" D' D D' ", " D3' "},
    {" U U' ", " W "},
    {" U' U ", " W' "},
    {" D D' ", " B "},
    {" D' D ", " B' "},
    {" U U ", " UU "},
    {" U' U' ", " UU' "},
    {" D D ", " DD "},
    {" D' D' ", " DD' "},
    {" u u' ", " w "},
    {" u' u ", " w' "},
    {" u u' u ", " u3 "},
    {" u' u u' ", " u3' "},
    {" u u' u u' ", " u4 "},
    {" u' u u' u ", " u4' "},
};

// ---------------------------------------------------------------------------
// SHORTHAND_TO_KARN  — shorthand key (alignment-suffixed) -> karn/numeric sequence
// ---------------------------------------------------------------------------
static const std::map<std::string, std::string> SHORTHAND_TO_KARN = {
    {"bjj", "U' e D'"}, {"fjj", "U e' D"},
    {"e2bjj", "U' e' U'"}, {"e2fjj", "U e U"},
    {"nn", "E E'"},
    {"jn", "D4'"}, {"nj", "U4"},
    {"jj", "U e' D"}, {"bjj+e2", "U' e' U'"},
    {"-nn", "E' E"},
    {"-jn", "D4"}, {"-nj", "D4'"},
    {"bpj10", "d m' U"}, {"bpj0-1", "u' m D'"},
    {"fpj10", "u m' D"}, {"fpj0-1", "d' m U'"},
    {"aa10", "u m' u T'"}, {"aa0-1", "U m' U t'"},
    {"fadj10", "D M' d'"}, {"dadj10", "D M' d'"},
    {"fadj0-1", "U' M u"}, {"u'adj0-1", "U' M u"},
    {"badj10", "U M' u'"}, {"uadj10", "U M' u'"},
    {"badj0-1", "D' M d"}, {"d'adj0-1", "D' M d"},
    {"bb10", "T u' e U'"}, {"bb0-1", "t d e' D"},
    {"fdd10", "D e' d t"}, {"fdd0-1", "U' e u' T"},
    {"bdd10", "U e' u T'"}, {"bdd0-1", "D' e d' t'"},
    {"ff10", "d m' d M E"}, {"ff0-1", "u' m U' M T"},
    {"fv10", "d4"}, {"fv0-1", "d4'"},
    {"vf10", "u4"}, {"vf0-1", "u4'"},
    {"y2fv10", "u d' u -5,4"},
    {"jf10", "w D' u T'"}, {"jf0-1", "w' D u' T"},
    {"fj10", "b U' d t"}, {"fj0-1", "b' U d' t'"},
    {"jr00", "e' w e"}, {"jr10", "e' b e"},
    {"jr0-1", "e' w' e"}, {"jr1-1", "e' b' e"},
    {"rj00", "e b' e'"}, {"rj10", "e w e'"},
    {"rj0-1", "e b' e'"}, {"rj1-1", "e w e'"},
    {"jv10", "b D d d2'"}, {"jv0-1", "b' D' d' d2"},
    {"vj10", "w U u u2'"}, {"vj0-1", "w' U' u' u2"},
    {"kk10", "u m' U E'"}, {"kk0-1", "U m' u E'"},
    {"opp10", "u2 u2'"}, {"opp0-1", "u2' u2"},
    {"pn10", "T T'"}, {"pn0-1", "t t'"},
    {"px10", "f' d3' f'"}, {"px0-1", "f d3 f"},
    {"xp10", "F' u3' F'"}, {"xp0-1", "F u3 F"},
    {"tt10", "d m' F' u2'"},
    {"fss10", "u M D' E'"}, {"fss0-1", "D' M u E'"},
    {"bss10", "D M' u' E"}, {"bss0-1", "U' M d E"},
    {"vv10", "u M u m' E'"},
    {"zz10", "u M t' M D'"}, {"zz0-1", "D' M t' M u"},
    {"30adj10", "U M' u'"}, {"-30adj0-1", "U' M u"},
    {"03adj10", "D M' d'"}, {"0-3adj0-1", "D' M d"},
};

// ---------------------------------------------------------------------------
// SHORTHAND_ALIGN_INDEPENDENT  — shorthands that don't need an alignment suffix
// ---------------------------------------------------------------------------
static const std::set<std::string> SHORTHAND_ALIGN_INDEPENDENT = {
    "bjj",
    "fjj",
    "nn",
    "jn",
    "nj",
    "e2bjj",
    "e2fjj",
    "jj",
    "bjj+e2",
    "-nn",
    "-jn",
    "-nj",
};

// ===========================================================================
// Low-level string utilities
// ===========================================================================

inline std::string trimStr(const std::string &s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos)
        return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

inline std::string replaceAll(std::string str, const std::string &from, const std::string &to)
{
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos)
    {
        str.replace(pos, from.size(), to);
        pos += to.size();
    }
    return str;
}

inline std::string dictReplace(std::string str, const std::map<std::string, std::string> &dict)
{
    std::string prev;
    do
    {
        prev = str;
        for (const auto &[k, v] : dict)
        {
            str = replaceAll(str, k, v);
            if (str != prev)
                break;
        }
    } while (str != prev);
    return str;
}

inline std::string replaceWithVector(std::string str, const std::vector<std::pair<std::string, std::string>> &vec)
{
    std::string prev;
    do
    {
        prev = str;
        for (const auto &[k, v] : vec)
        {
            str = replaceAll(str, k, v);
            if (str != prev)
                break;
        }
    } while (str != prev);
    return str;
}

// ===========================================================================
// OPTIMIZED: Hash map versions for O(1) lookup instead of O(n) vector search
// ===========================================================================

// Lazy-initialized hash maps
inline std::unordered_map<std::string, std::string> &getWCAToKarnMap()
{
    static std::unordered_map<std::string, std::string> map;
    static bool initialized = false;
    if (!initialized)
    {
        for (const auto &[k, v] : WCA_TO_KARN)
        {
            map[k] = v;
        }
        initialized = true;
    }
    return map;
}

inline std::unordered_map<std::string, std::string> &getWCAToKarnOCSMap()
{
    static std::unordered_map<std::string, std::string> map;
    static bool initialized = false;
    if (!initialized)
    {
        for (const auto &[k, v] : WCA_TO_KARN_OCS)
        {
            map[k] = v;
        }
        initialized = true;
    }
    return map;
}

inline std::unordered_map<std::string, std::string> &getKarnToHighKarnMap()
{
    static std::unordered_map<std::string, std::string> map;
    static bool initialized = false;
    if (!initialized)
    {
        for (const auto &[k, v] : KARN_TO_HIGHKARN)
        {
            map[k] = v;
        }
        initialized = true;
    }
    return map;
}

inline std::unordered_map<std::string, std::string> &getKarnToHighKarnOCSMap()
{
    static std::unordered_map<std::string, std::string> map;
    static bool initialized = false;
    if (!initialized)
    {
        for (const auto &[k, v] : KARN_TO_HIGHKARN_OCS)
        {
            map[k] = v;
        }
        initialized = true;
    }
    return map;
}

// Helper: These KARN_TO_KARN replacements are applied longest-first so that a
// superstring key (e.g. " U U' U U' " -> U4) is tried before any substring key
// (e.g. " U U' " -> W). Hash-map iteration order is arbitrary, so sort keys by
// descending length before applying.
inline std::string applyHighKarnReplacements(std::string str, const std::unordered_map<std::string, std::string> &map)
{
    std::vector<std::string> keys;
    keys.reserve(map.size());
    for (const auto &entry : map) keys.push_back(entry.first);
    std::sort(keys.begin(), keys.end(), [](const std::string &a, const std::string &b) {
        return a.size() > b.size();
    });

    std::string prev;
    do
    {
        prev = str;
        for (const auto &k : keys)
        {
            auto it = map.find(k);
            str = replaceAll(str, k, it->second);
            if (str != prev) break;
        }
    } while (str != prev);
    return str;
}

// ===========================================================================
// splitStr / addCommasToMove / getAlignment — helpers shared with sq1_logic
// ===========================================================================

inline std::vector<std::string> splitStr(const std::string &s, char delim)
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

inline std::string addCommasToMove(const std::string &move)
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

// Applies addCommasToMove to every space-separated token in a string.
inline std::string addCommas(const std::string &spaced)
{
    std::istringstream iss(spaced);
    std::string tok, out;
    bool first = true;
    while (iss >> tok)
    {
        if (!first)
            out += ' ';
        out += addCommasToMove(tok);
        first = false;
    }
    return out;
}

inline std::string getAlignment(bool topA, bool bottomA)
{
    return std::string(topA ? "1" : "0") + std::string(bottomA ? "-1" : "0");
}

// Splits on slice chars (/ \ |) and spaces, resolves each token through
// KARN_TO_WCA in two passes (high-karn -> move sequence, then leftover
// base-karn -> numeric), and rejoins with slashes. A leading/trailing slice
// is added automatically when the alg starts/ends on a recognized karn token.
inline std::string unkarnifyHelp(const std::string &algIn)
{
    std::string alg = trimStr(algIn);
    alg = replaceAll(alg, "(", "");
    alg = replaceAll(alg, ")", "");

    {
        std::string prev;
        do
        {
            prev = alg;
            alg = replaceAll(alg, " / ", "/");
            alg = replaceAll(alg, " \\ ", "\\");
            alg = replaceAll(alg, " | ", "|");
        } while (alg != prev);
    }

    auto isDelim = [](char c)
    { return c == '/' || c == '\\' || c == '|' || c == ' '; };

    bool hasDelim = false;
    for (char c : alg)
        if (isDelim(c)) { hasDelim = true; break; }

    std::string firstMove, lastMove;
    if (!hasDelim)
    {
        firstMove = lastMove = alg;
    }
    else
    {
        size_t p = 0;
        while (p < alg.size() && !isDelim(alg[p])) p++;
        firstMove = alg.substr(0, p);
        size_t q = alg.size();
        while (q > 0 && !isDelim(alg[q - 1])) q--;
        lastMove = alg.substr(q);
    }

    bool startsSliceChar = !alg.empty() && (alg[0] == '/' || alg[0] == '\\' || alg[0] == '|');
    std::string startingSlice = startsSliceChar ? std::string(1, alg[0])
                                                 : (KARN_TO_WCA.count(firstMove) ? "/" : "");

    bool endsSliceChar = !alg.empty() && alg.back() == '/';
    std::string endingSlice = endsSliceChar ? "/"
                                             : (KARN_TO_WCA.count(lastMove) ? "/" : "");

    // collapse delimiter runs to a single space
    std::string spaced;
    bool inRun = false;
    for (char c : alg)
    {
        if (isDelim(c))
        {
            if (!inRun) { spaced += ' '; inRun = true; }
        }
        else
        {
            spaced += c;
            inRun = false;
        }
    }
    spaced = addCommas(trimStr(spaced));

    std::vector<std::string> tokens;
    {
        std::istringstream iss(spaced);
        std::string t;
        while (iss >> t) tokens.push_back(t);
    }

    // pass 1: karn token -> its (possibly multi-token) WCA value
    std::vector<std::string> expanded;
    for (const auto &tok : tokens)
    {
        auto it = KARN_TO_WCA.find(tok);
        if (it != KARN_TO_WCA.end())
        {
            std::istringstream vs(it->second);
            std::string sub;
            while (vs >> sub) expanded.push_back(sub);
        }
        else
        {
            expanded.push_back(tok);
        }
    }

    // pass 2: resolve leftover base-karn names from pass 1 (e.g. "U" -> "3,0")
    for (auto &tok : expanded)
    {
        auto it = KARN_TO_WCA.find(tok);
        if (it != KARN_TO_WCA.end()) tok = it->second;
    }

    std::string joined;
    for (size_t i = 0; i < expanded.size(); i++)
    {
        if (i) joined += "/";
        joined += expanded[i];
    }

    std::string result = startingSlice + joined + endingSlice;
    result.erase(std::remove(result.begin(), result.end(), ' '), result.end());
    return result;
}

inline std::string replaceShorthands(std::string alg)
{
    std::vector<std::string> moves;
    {
        std::string cur;
        for (char c : alg)
        {
            if (c == '/' || c == '\\' || c == '|')
            {
                moves.push_back(cur);
                cur.clear();
            }
            else
                cur += c;
        }
        moves.push_back(cur);
    }

    bool topA = false, bottomA = false;

    for (const auto &move : moves)
    {
        if (move.empty())
            continue;

        size_t comma = move.find(',');
        if (comma != std::string::npos)
        {
            try
            {
                int u = std::stoi(move.substr(0, comma));
                int d = std::stoi(move.substr(comma + 1));
                if (u % 3 != 0) topA = !topA;
                if (d % 3 != 0) bottomA = !bottomA;
            }
            catch (...) {}
            continue;
        }

        std::string lower = move;
        for (char &c : lower) c = (char)std::tolower((unsigned char)c);
        std::string key = SHORTHAND_ALIGN_INDEPENDENT.count(lower)
            ? lower
            : lower + getAlignment(topA, bottomA);

        auto it = SHORTHAND_TO_KARN.find(key);
        if (it == SHORTHAND_TO_KARN.end())
            throw std::runtime_error("replaceShorthands: \"" + move + "\" with alignment " +
                                      getAlignment(topA, bottomA) + " is not defined.");

        const std::string &replacement = it->second;
        size_t pos = alg.find(move);
        if (pos != std::string::npos)
            alg = alg.substr(0, pos) + replacement + alg.substr(pos + move.size());

        for (const auto &sub : splitStr(unkarnifyHelp(replacement), '/'))
        {
            if (sub.empty()) continue;
            size_t c2 = sub.find(',');
            if (c2 == std::string::npos) continue;
            try
            {
                int u2 = std::stoi(sub.substr(0, c2));
                int d2 = std::stoi(sub.substr(c2 + 1));
                if (u2 % 3 != 0) topA = !topA;
                if (d2 % 3 != 0) bottomA = !bottomA;
            }
            catch (...) {}
        }
    }

    return unkarnifyHelp(alg);
}

// Expands "(X X')3" -> "X X' X X' X X'" style repeat groups.
inline std::string expandRepeatGroups(const std::string &algIn)
{
    static const std::regex re(R"(\(([^()]*)\)(\d+))");
    std::string result = algIn;
    std::smatch m;
    while (std::regex_search(result, m, re))
    {
        int count = std::stoi(m[2].str());
        const std::string &inner = m[1].str();
        std::string repeated;
        for (int i = 0; i < count; i++)
        {
            if (i) repeated += ' ';
            repeated += inner;
        }
        result = m.prefix().str() + repeated + m.suffix().str();
    }
    return result;
}

// ===========================================================================
// unkarnify — public: Karnotation / shorthand -> WCA numeric slash format
// ===========================================================================
inline std::string unkarnify(const std::string &algIn)
{
    std::string s = algIn;

    if (s.find("meow") != std::string::npos)
        return s;

    s = replaceAll(s, "&", "-1");
    s = replaceAll(s, "^", "-2");
    s = replaceAll(s, "9", "-3");
    s = replaceAll(s, "8", "-4");
    s = replaceAll(s, "7", "-5");

    s = expandRepeatGroups(s);

    std::string final_ = replaceShorthands(unkarnifyHelp(s));

    while (final_.find("//") != std::string::npos)
        final_ = replaceAll(final_, "//", "/");

    return final_;
}

// Splits into individual moves; each move is karnified to its base karn name
// (WCA_TO_KARN) except the alg's own first/last move when it isn't bordered
// by a slice — those stay numeric with commas stripped. Runs of base
// karns are then combined into high karns (KARN_TO_HIGHKARN).
inline std::string karnify(const std::string &algIn)
{
    std::string alg = trimStr(algIn);
    if (alg.empty())
        return alg;

    bool startsSlice = (alg[0] == '/' || alg[0] == '\\' || alg[0] == '|');
    std::string startingSlice = startsSlice ? std::string(1, alg[0]) : "";
    bool endsSlice = alg.back() == '/';
    std::string endingSlice = endsSlice ? "/" : "";

    auto isDelim = [](char c)
    { return c == '/' || c == '\\' || c == '|' || c == ' '; };
    std::string spaced;
    bool inRun = false;
    for (char c : alg)
    {
        if (isDelim(c))
        {
            if (!inRun) { spaced += ' '; inRun = true; }
        }
        else
        {
            spaced += c;
            inRun = false;
        }
    }

    std::vector<std::string> s;
    {
        std::istringstream iss(spaced);
        std::string t;
        while (iss >> t) s.push_back(t);
    }

    auto &wcaMap = getWCAToKarnMap();

    for (size_t i = 0; i < s.size(); i++)
    {
        if (i == 0 && !startsSlice)
        {
            s[i] = replaceAll(s[i], ",", "");
            continue;
        }
        if (i == s.size() - 1 && !endsSlice)
        {
            s[i] = replaceAll(s[i], ",", "");
            break;
        }

        auto it = wcaMap.find(" " + s[i] + " ");
        bool inBaseKarn = it != wcaMap.end();
        s[i] = inBaseKarn ? trimStr(it->second) : replaceAll(s[i], ",", "");

        if (inBaseKarn && i == 0 && startingSlice == "/")
            startingSlice = "";
        if (inBaseKarn && i == s.size() - 1)
            endingSlice = "";
    }

    std::string joined;
    for (size_t i = 0; i < s.size(); i++)
    {
        if (i) joined += ' ';
        joined += s[i];
    }

    std::string result = startingSlice + joined + endingSlice;
    auto &highKarnMap = getKarnToHighKarnMap();
    result = applyHighKarnReplacements(" " + result + " ", highKarnMap);
    result = trimStr(result);
    return result;
}

// ===========================================================================
// karnifycs — cubeshape-aware karnify.
//
// Parses algWCA (WCA slash numeric format) and applies WCA_TO_KARN for moves
// executed while the puzzle is in cubeshape, and WCA_TO_KARN_OCS otherwise.
// Commas are always stripped (matching karnify() behaviour).
//
// startStateHex: position string like "A1B2C3D45E6F7G8H-" (16-17 chars).
//   Uppercase letters A-H = corners; digits 1-8 = edges.
// generatorMode: if true, start from the solved state (all algs share the
//   same start, so compute slots once externally if calling in a batch).
// ===========================================================================
namespace karnifycs_detail
{

    // slotState[24]: 0 = corner slot, 1 = edge slot.
    // Layout matches sq1opt FullPosition: indices 0-11 = top layer, 12-23 = bottom.

    inline void kcTopTurn(int slotState[24], int t)
    {
        t = ((t % 12) + 12) % 12;
        for (int k = 0; k < t; k++)
        {
            int last = slotState[11];
            for (int i = 11; i > 0; i--)
                slotState[i] = slotState[i - 1];
            slotState[0] = last;
        }
    }

    inline void kcBotTurn(int slotState[24], int d)
    {
        d = ((d % 12) + 12) % 12;
        for (int k = 0; k < d; k++)
        {
            int last = slotState[23];
            for (int i = 23; i > 12; i--)
                slotState[i] = slotState[i - 1];
            slotState[12] = last;
        }
    }

    inline void kcSlice(int slotState[24])
    {
        for (int i = 6; i < 12; i++)
            std::swap(slotState[i], slotState[i + 6]);
    }

    // A square layer: 4 corners (2 slots each = 00) + 4 edges (1 slot = 1).
    // Valid 12-slot patterns are the 3 rotations of [0,0,1, 0,0,1, 0,0,1, 0,0,1]:
    //   edges fall at positions with remainder 0, 1, or 2 (mod 3).
    inline bool kcLayerIsSquare(const int slotState[], int base)
    {
        for (int rem = 0; rem < 3; rem++)
        {
            bool ok = true;
            for (int i = 0; i < 12; i++)
            {
                if (slotState[base + i] != (i % 3 == rem ? 1 : 0))
                {
                    ok = false;
                    break;
                }
            }
            if (ok)
                return true;
        }
        return false;
    }

    inline bool kcInCubeshape(const int slotState[24])
    {
        return kcLayerIsSquare(slotState, 0) && kcLayerIsSquare(slotState, 12);
    }

    // Parse a position hex string into slotState[24]. Returns true on success.
    inline bool kcParseState(const std::string &posHex, int slotState[24])
    {
        if (posHex.size() < 16)
            return false;
        int j = 0;
        for (int i = 0; i < 16 && j < 24; i++)
        {
            char c = posHex[i];
            bool isCorner = (c >= 'A' && c <= 'H') || (c >= 'a' && c <= 'h') ||
                            c == 'U' || c == 'V' || c == 'W';
            if (isCorner)
            {
                if (j + 1 >= 24)
                    return false;
                slotState[j++] = 0;
                slotState[j++] = 0;
            }
            else
            {
                if (j >= 24)
                    return false;
                slotState[j++] = 1;
            }
        }
        return j == 24;
    }

    // Apply a "t,d" move token to the slot state.
    inline void kcApplyTurnToken(int slotState[24], const std::string &token)
    {
        size_t comma = token.find(',');
        if (comma == std::string::npos)
            return;
        try
        {
            int u = std::stoi(token.substr(0, comma));
            int d = std::stoi(token.substr(comma + 1));
            kcTopTurn(slotState, u);
            kcBotTurn(slotState, d);
        }
        catch (...)
        {
        }
    }

    // Apply a single WCA_TO_KARN / WCA_TO_KARN_OCS substitution to one space-padded
    // numeric token, then strip commas.
    inline std::string kcSubstituteToken(
        const std::string &token,
        const std::vector<std::pair<std::string, std::string>> &table)
    {
        std::string out = replaceWithVector(" " + token + " ", table);
        out = trimStr(out);
        out = replaceAll(out, ",", "");
        return out;
    }

} // namespace karnifycs_detail

namespace karnifycs_detail
{
    // the slot layout of a solved cube
    inline const int kSolvedSlots[24] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0};

    // Shared implementation once slotState[24] is already built. Both public
    // overloads below funnel into this — one builds slotState from a hex
    // string (the C-bridge / Rust path), the other from a raw 24-slot pos[]
    // array (sq1opt's own FullPosition, no string round-trip needed).
    inline std::string karnifycsImpl(const std::string &algWCA, int slotState[24])
    {
        std::string alg = trimStr(algWCA);
        if (alg.empty())
            return alg;

        bool startsSlice = (alg.front() == '/' || alg.front() == '\\' || alg.front() == '|');
        std::string startingSlice = startsSlice ? std::string(1, alg.front()) : "";
        bool endsSlice = alg.size() > 1 && alg.back() == '/';
        std::string endingSlice = endsSlice ? "/" : "";

        if (startsSlice)
            kcSlice(slotState);

        std::string normalized = replaceAll(alg, "\\", "/");
        normalized = replaceAll(normalized, "|", "/");
        auto parts = splitStr(normalized, '/');

        // Flatten into individual moves, recording the cubeshape state before
        // each move, and applying a real slice (kcSlice) between slash-groups.
        std::vector<std::string> moveTok;
        std::vector<bool> moveInCS;
        bool firstPart = true;
        for (const auto &part : parts)
        {
            if (!firstPart)
                kcSlice(slotState);
            firstPart = false;

            std::string trimmedPart = trimStr(part);
            if (trimmedPart.empty())
                continue;

            std::istringstream iss(trimmedPart);
            std::string tok;
            while (iss >> tok)
            {
                moveInCS.push_back(kcInCubeshape(slotState));
                moveTok.push_back(tok);
                kcApplyTurnToken(slotState, tok);
            }
        }

        size_t n = moveTok.size();
        std::vector<std::string> out(n);

        auto &wcaMap = getWCAToKarnMap();
        auto &wcaOCSMap = getWCAToKarnOCSMap();

        for (size_t i = 0; i < n; i++)
        {
            if (i == 0 && !startsSlice)
            {
                out[i] = replaceAll(moveTok[i], ",", "");
                continue;
            }
            if (i == n - 1 && !endsSlice)
            {
                out[i] = replaceAll(moveTok[i], ",", "");
                break;
            }

            const auto &map = moveInCS[i] ? wcaMap : wcaOCSMap;
            auto it = map.find(" " + moveTok[i] + " ");
            bool inBaseKarn = it != map.end();
            out[i] = inBaseKarn ? trimStr(it->second) : replaceAll(moveTok[i], ",", "");

            if (inBaseKarn && i == 0 && startingSlice == "/")
                startingSlice = "";
            if (inBaseKarn && i == n - 1)
                endingSlice = "";
        }

        std::string joined;
        for (size_t i = 0; i < n; i++)
        {
            if (i) joined += ' ';
            joined += out[i];
        }

        std::string result = startingSlice + joined + endingSlice;
        auto &highKarnMap = getKarnToHighKarnMap();
        auto &highKarnOCSMap = getKarnToHighKarnOCSMap();
        result = applyHighKarnReplacements(" " + result + " ", highKarnMap);
        result = applyHighKarnReplacements(" " + trimStr(result) + " ", highKarnOCSMap);
        result = trimStr(result);

        return result;
    }
}

// cubeshape-aware karnify. String-based overload: parses startStateHex
// (e.g. "A1B2C3D45E6F7G8H", 16-17 chars) into slot state. Used by the
// C bridge (karn_bridge.cpp) / Rust side, where positions arrive as strings.
inline std::string karnifycs(
    const std::string &algWCA,
    const std::string &startStateHex,
    bool generatorMode)
{
    using namespace karnifycs_detail;
    int slotState[24];
    if (generatorMode || !kcParseState(startStateHex, slotState))
        for (int i = 0; i < 24; i++)
            slotState[i] = kSolvedSlots[i];
    return karnifycsImpl(algWCA, slotState);
}

// overload for callers that already have a raw 24-slot position array
inline std::string karnifycs(
    const std::string &algWCA,
    const int pos[24],
    bool generatorMode)
{
    using namespace karnifycs_detail;
    int slotState[24];
    if (generatorMode)
    {
        for (int i = 0; i < 24; i++)
            slotState[i] = kSolvedSlots[i];
    }
    else
    {
        for (int i = 0; i < 24; i++)
            slotState[i] = (pos[i] < 8) ? 0 : 1;
    }
    return karnifycsImpl(algWCA, slotState);
}
