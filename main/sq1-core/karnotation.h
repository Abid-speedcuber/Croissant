#pragma once
// Shared Karnotation tables and conversion functions.
// Generated from karn.js by karnotation_gen.html
// Used by sq1opt.cpp (alg printing) and mainwindow.cpp (unkarnify / ergonomics rating).
// Edit this file once to affect both.
//
// Public API (all inline):
//   karnify(algWCASlash)        — WCA numeric slash-format → Karnotation display string
//   unkarnify(algIn)            — Karnotation / shorthand → WCA numeric slash-format
//   karnifycs(alg, stateHex, generatorMode)
//                               — Like karnify() but cubeshape-aware (not called yet)
#include <string>
#include <map>
#include <set>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>

// ---------------------------------------------------------------------------
// KARN_TO_WCA  — Karnotation name (space-padded) -> numeric WCA slash format
// ---------------------------------------------------------------------------
static const std::map<std::string,std::string> KARN_TO_WCA = {
    {" U4 ", " / U U' U U' / "},
    {" U4' ", " / U' U U' U / "},
    {" D4 ", " / D D' D D' / "},
    {" D4' ", " / D' D D' D / "},
    {" u4 ", " / u u' u u' / "},
    {" u4' ", " / u' u u' u / "},
    {" d4 ", " / d d' d d' / "},
    {" d4' ", " / d' d d' d / "},
    {" U3 ", " / U U' U / "},
    {" U3' ", " / U' U U' / "},
    {" D3 ", " / D D' D / "},
    {" D3' ", " / D' D D' / "},
    {" u3 ", " / u u' u / "},
    {" u3' ", " / u' u u' / "},
    {" d3 ", " / d d' d / "},
    {" d3' ", " / d' d d' / "},
    {" F3 ", " / F F' F / "},
    {" F3' ", " / F' F F' / "},
    {" f3 ", " / f f' f / "},
    {" f3' ", " / f' f f' / "},
    {" W ", " / U U' / "},
    {" W' ", " / U' U / "},
    {" B ", " / D D' / "},
    {" B' ", " / D' D / "},
    {" w ", " / u u' / "},
    {" w' ", " / u' u / "},
    {" b ", " / d d' / "},
    {" b' ", " / d' d / "},
    {" F2 ", " / F F' / "},
    {" F2' ", " / F' F / "},
    {" f2 ", " / f f' / "},
    {" f2' ", " / f' f / "},
    {" UU ", " / U U / "},
    {" UU' ", " / U' U' / "},
    {" DD ", " / D D / "},
    {" DD' ", " / D' D' / "},
    {" T2 ", " / T T' / "},
    {" T2' ", " / T' T / "},
    {" t2 ", " / t t' / "},
    {" t2' ", " / t' t / "},
    {" U2 ", " /6,0/ "},
    {" U2' ", " /6,0/ "},
    {" D2 ", " /0,6/ "},
    {" U2D ", " /6,3/ "},
    {" U2D' ", " /6,-3/ "},
    {" U2D2 ", " /6,6/ "},
    {" UD2 ", " /3,6/ "},
    {" U'D2 ", " /-3,6/ "},
    {" U ", " /3,0/ "},
    {" U' ", " /-3,0/ "},
    {" D ", " /0,3/ "},
    {" D' ", " /0,-3/ "},
    {" E ", " /3,-3/ "},
    {" E' ", " /-3,3/ "},
    {" e ", " /3,3/ "},
    {" e' ", " /-3,-3/ "},
    {" u ", " /2,-1/ "},
    {" u' ", " /-2,1/ "},
    {" d ", " /-1,2/ "},
    {" d' ", " /1,-2/ "},
    {" F ", " /4,1/ "},
    {" F' ", " /-4,-1/ "},
    {" f ", " /1,4/ "},
    {" f' ", " /-1,-4/ "},
    {" T ", " /2,-4/ "},
    {" T' ", " /-2,4/ "},
    {" t ", " /4,-2/ "},
    {" t' ", " /-4,2/ "},
    {" m ", " /2,2/ "},
    {" m' ", " /-2,-2/ "},
    {" M ", " /1,1/ "},
    {" M' ", " /-1,-1/ "},
    {" u2 ", " /5,-1/ "},
    {" u2' ", " /-5,1/ "},
    {" d2 ", " /-1,5/ "},
    {" d2' ", " /1,-5/ "},
    {" K ", " /5,2/ "},
    {" K' ", " /-5,-2/ "},
    {" k ", " /2,5/ "},
    {" k' ", " /-2,-5/ "},
    {" U2'D ", " /6,3/ "},
    {" U2'D' ", " /6,-3/ "},
    {" \xc9\x87 ", " / U D / "},
    {" \xc9\x87' ", " / U' D' / "},
    {" \xc9\x86 ", " / U D' / "},
    {" \xc9\x86' ", " / U' D / "},
};

// ---------------------------------------------------------------------------
// WCA_TO_KARN  — numeric WCA slash format (space-padded) -> Karnotation name
// Used when cube IS in cubeshape.
// ---------------------------------------------------------------------------
static const std::vector<std::pair<std::string,std::string>> WCA_TO_KARN = {
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

// ---------------------------------------------------------------------------
// WCA_TO_KARN_OCS  — WCA -> Karn replacements used when OUT of cubeshape.
// Empty for now; define entries here when OCS karnotation names are decided.
// ---------------------------------------------------------------------------
static const std::vector<std::pair<std::string,std::string>> WCA_TO_KARN_OCS = {
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
};

// ---------------------------------------------------------------------------
// SHORTHAND_TO_KARN  — shorthand key (alignment-suffixed) -> karn/numeric sequence
// ---------------------------------------------------------------------------
static const std::map<std::string,std::string> SHORTHAND_TO_KARN = {
    {"bjj", "/U' e D'/"},
    {"fjj", "/U e' D/"},
    {"e2bjj", "/U' e' U'/"},
    {"e2fjj", "/U e U/"},
    {"nn", "/E E'/"},
    {"jn", "/D4'/"},
    {"nj", "/U4/"},
    {"jj", "/U e' D/"},
    {"bjj+e2", "/U' e' U'/"},
    {"-nn", "/E' E/"},
    {"-jn", "/D4/"},
    {"-nj", "/D4'/"},
    {"bpj10", "/d m' U/"},
    {"bpj0-1", "/u' m D'/"},
    {"fpj10", "/u m' D/"},
    {"fpj0-1", "/d' m U'/"},
    {"aa10", "/u m' u T'/"},
    {"aa0-1", "/U m' U t'/"},
    {"fadj10", "/D M' d'/"},
    {"dadj10", "/D M' d'/"},
    {"fadj0-1", "/U' M u/"},
    {"u'adj0-1", "/U' M u/"},
    {"badj10", "/U M' u'/"},
    {"uadj10", "/U M' u'/"},
    {"badj0-1", "/D' M d/"},
    {"d'adj0-1", "/D' M d/"},
    {"bb10", "/T u' e U'/"},
    {"bb0-1", "/t d e' D/"},
    {"fdd10", "/D e' d t/"},
    {"fdd0-1", "/U' e u' T/"},
    {"bdd10", "/U e' u T'/"},
    {"bdd0-1", "/D' e d' t'/"},
    {"ff10", "/d m' d M E/"},
    {"ff0-1", "/u' m U' M T/"},
    {"fv10", "/d4/"},
    {"fv0-1", "/d4'/"},
    {"vf10", "/u4/"},
    {"vf0-1", "/u4'/"},
    {"y2fv10", "/u d' u -5,4/"},
    {"jf10", "/w D' u T'/"},
    {"jf0-1", "/w' D u' T/"},
    {"fj10", "/b U' d t/"},
    {"fj0-1", "/b' U d' t'/"},
    {"jr00", "/e' w e/"},
    {"jr10", "/e' b e/"},
    {"jr0-1", "/e' w' e/"},
    {"jr1-1", "/e' b' e/"},
    {"rj00", "/e b' e'/"},
    {"rj10", "/e w e'/"},
    {"rj0-1", "/e b' e'/"},
    {"rj1-1", "/e w e'/"},
    {"jv10", "/b D d d2'/"},
    {"jv0-1", "/b' D' d' d2/"},
    {"vj10", "/w U u u2'/"},
    {"vj0-1", "/w' U' u' u2/"},
    {"kk10", "/u m' U E'/"},
    {"kk0-1", "/U m' u E'/"},
    {"opp10", "/u2 u2'/"},
    {"opp0-1", "/u2' u2/"},
    {"pn10", "/T T'/"},
    {"pn0-1", "/t t'/"},
    {"px10", "/f' d3' f'/"},
    {"px0-1", "/f d3 f/"},
    {"xp10", "/F' u3' F'/"},
    {"xp0-1", "/F u3 F/"},
    {"tt10", "/d m' F' u2'/"},
    {"fss10", "/u M D' E'/"},
    {"fss0-1", "/D' M u E'/"},
    {"bss10", "/D M' u' E/"},
    {"bss0-1", "/U' M d E/"},
    {"vv10", "/u M u m' E'/"},
    {"zz10", "/u M t' M D'/"},
    {"zz0-1", "/D' M t' M u/"},
    {"30adj10", "/U M' u'/"},
    {"3adj10", "/U M' u'/"},
    {"03adj10", "/D M' d'/"},
    {"-30adj0-1", "/U' M u/"},
    {"obopp00", "1,0/M' F M' F M'/0,1"},
    {"oaopp1-1", "0,1/M' u' M' u' M'/0,1"},
    {"done!00", "0,0"},
};

// ---------------------------------------------------------------------------
// SHORTHAND_ALIGN_INDEPENDENT  — shorthands that don't need an alignment suffix
// ---------------------------------------------------------------------------
static const std::set<std::string> SHORTHAND_ALIGN_INDEPENDENT = {
    "bjj", "fjj", "nn", "jn", "nj", "e2bjj", "e2fjj", "jj", "bjj+e2",
    "-nn", "-jn", "-nj",
};

// ===========================================================================
// Low-level string utilities
// ===========================================================================

inline std::string trimStr(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

inline std::string replaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.size(), to);
        pos += to.size();
    }
    return str;
}

inline std::string dictReplace(std::string str, const std::map<std::string,std::string>& dict) {
    std::string prev;
    do {
        prev = str;
        for (const auto& [k, v] : dict) {
            str = replaceAll(str, k, v);
            if (str != prev) break;
        }
    } while (str != prev);
    return str;
}

inline std::string replaceWithVector(std::string str, const std::vector<std::pair<std::string,std::string>>& vec) {
    std::string prev;
    do {
        prev = str;
        for (const auto& [k, v] : vec) {
            str = replaceAll(str, k, v);
            if (str != prev) break;
        }
    } while (str != prev);
    return str;
}

// ===========================================================================
// splitStr / addCommasToMove / getAlignment — helpers shared with sq1_logic
// ===========================================================================

inline std::vector<std::string> splitStr(const std::string &s, char delim) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == delim) { out.push_back(cur); cur.clear(); }
        else cur += c;
    }
    out.push_back(cur);
    return out;
}

inline std::string addCommasToMove(const std::string &move) {
    if (move.empty()) return move;
    for (char c : move)
        if (c != '-' && !std::isdigit((unsigned char)c))
            return move;
    switch (move.size()) {
    case 1: return move + ",0";
    case 2: return move[0] == '-' ? move + ",0"
                                  : std::string(1, move[0]) + "," + std::string(1, move[1]);
    case 3: return move[0] == '-' ? move.substr(0, 2) + "," + std::string(1, move[2])
                                  : std::string(1, move[0]) + "," + move.substr(1);
    case 4: return move.substr(0, 2) + "," + move.substr(2);
    default: return move;
    }
}

inline std::string getAlignment(bool topA, bool bottomA) {
    return std::string(topA ? "1" : "0") + std::string(bottomA ? "-1" : "0");
}

// ---------------------------------------------------------------------------
// unkarnifyHelp — apply KARN_TO_WCA dict to a space-separated token string
// and normalise to a slash-separated numeric string.
// ---------------------------------------------------------------------------
inline std::string unkarnifyHelp(const std::string& scramble) {
    std::string s = dictReplace(" " + scramble + " ", KARN_TO_WCA);
    s = trimStr(s);

    std::string prev;
    do {
        prev = s;
        s = replaceAll(s, " / ", "/");
        s = replaceAll(s, "/ /", "/");
        s = replaceAll(s, " /",  "/");
        s = replaceAll(s, "/ ",  "/");
        s = replaceAll(s, "//",  "/");
    } while (s != prev);

    for (char& c : s) if (c == ' ') c = '/';

    do {
        prev = s;
        s = replaceAll(s, "//", "/");
    } while (s != prev);

    return s;
}

// ---------------------------------------------------------------------------
// replaceShorthands — resolve alignment-dependent shorthand tokens in a
// slash-separated string.
// ---------------------------------------------------------------------------
inline std::string replaceShorthands(std::string scramble) {
    std::vector<std::string> moves;
    {
        std::istringstream ss(scramble);
        std::string tok;
        while (std::getline(ss, tok, '/'))
            moves.push_back(tok);
    }

    bool allKnown = true;
    for (const auto& m : moves) {
        if (m.empty()) continue;
        bool numeric = std::isdigit((unsigned char)m[0]) || m[0] == '-';
        bool inDict  = KARN_TO_WCA.count(" " + m + " ") > 0;
        if (!numeric && !inDict) { allKnown = false; break; }
    }
    if (allKnown) {
        std::string spaced = scramble;
        for (char& c : spaced) if (c == '/') c = ' ';
        std::string prev;
        do { prev = spaced; spaced = replaceAll(spaced, "  ", " "); } while (spaced != prev);
        return unkarnifyHelp(trimStr(spaced));
    }

    bool topA = false, bottomA = false;
    for (const auto& move : moves) {
        if (move.empty()) continue;
        if (move.find(',') != std::string::npos) {
            size_t comma = move.find(',');
            try {
                int u = std::stoi(move.substr(0, comma));
                int d = std::stoi(move.substr(comma + 1));
                if (((u % 3) + 3) % 3 != 0) topA    = !topA;
                if (((d % 3) + 3) % 3 != 0) bottomA = !bottomA;
            } catch (...) {}
        } else {
            std::string lower = move;
            for (char& c : lower) c = (char)std::tolower((unsigned char)c);

            std::string key = SHORTHAND_ALIGN_INDEPENDENT.count(lower)
                                  ? lower
                                  : lower + getAlignment(topA, bottomA);

            if (!SHORTHAND_TO_KARN.count(key))
                return scramble;

            const std::string& repl = SHORTHAND_TO_KARN.at(key);
            scramble = replaceAll(scramble, move, repl);

            std::string inner = repl;
            if (!inner.empty() && inner.front() == '/') inner = inner.substr(1);
            if (!inner.empty() && inner.back()  == '/') inner.pop_back();
            std::string expanded = unkarnifyHelp(inner);
            std::istringstream ss2(expanded);
            std::string sub;
            while (std::getline(ss2, sub, '/')) {
                if (sub.empty()) continue;
                size_t c2 = sub.find(',');
                if (c2 == std::string::npos) continue;
                try {
                    int u2 = std::stoi(sub.substr(0, c2));
                    int d2 = std::stoi(sub.substr(c2 + 1));
                    if (((u2 % 3) + 3) % 3 != 0) topA    = !topA;
                    if (((d2 % 3) + 3) % 3 != 0) bottomA = !bottomA;
                } catch (...) {}
            }
        }
    }

    {
        std::string prev;
        do {
            prev = scramble;
            scramble = replaceAll(scramble, " /", "/");
            scramble = replaceAll(scramble, "/ ", "/");
            scramble = replaceAll(scramble, "//", "/");
        } while (scramble != prev);
    }
    for (char& c : scramble) if (c == '/') c = ' ';
    {
        std::string prev;
        do { prev = scramble; scramble = replaceAll(scramble, "  ", " "); } while (scramble != prev);
    }
    return unkarnifyHelp(trimStr(scramble));
}

// ===========================================================================
// unkarnify — public: Karnotation / shorthand -> WCA numeric slash format
// ===========================================================================
inline std::string unkarnify(const std::string &algIn) {
    std::string s = algIn;

    if (s.find("meow") != std::string::npos) return s;

    s = replaceAll(s, "&", "-1");
    s = replaceAll(s, "^", "-2");
    s = replaceAll(s, "9", "-3");
    s = replaceAll(s, "8", "-4");
    s = replaceAll(s, "7", "-5");

    bool firstSlice = (!s.empty() && (s[0] == '/' || s[0] == '\\'));
    if (!firstSlice) {
        std::istringstream iss(s);
        std::string tok;
        if (iss >> tok) {
            if (KARN_TO_WCA.count(" " + tok + " ")) firstSlice = true;
        }
    }
    bool lastSlice = false;
    {
        std::istringstream iss(s);
        std::string last, tok;
        while (iss >> tok) last = tok;
        if (!last.empty() && KARN_TO_WCA.count(" " + last + " ")) lastSlice = true;
    }

    for (char &c : s)
        if (c == '\\' || c == '/') c = ' ';
    s = replaceAll(s, "(", "");
    s = replaceAll(s, ")", "");
    {
        std::string tmp; bool sp = false;
        for (char c : s) {
            if (c == ' ') { if (!sp) { tmp += ' '; sp = true; } }
            else { tmp += c; sp = false; }
        }
        s = trimStr(tmp);
    }

    {
        std::vector<std::string> tokens;
        std::istringstream iss(s);
        std::string tok;
        while (iss >> tok) tokens.push_back(tok);
        s.clear();
        for (size_t i = 0; i < tokens.size(); i++) {
            if (i) s += ' ';
            s += addCommasToMove(tokens[i]);
        }
    }

    std::string final_ = replaceShorthands(unkarnifyHelp(s));

    if (firstSlice && (final_.empty() || final_[0] != '/'))
        final_ = "/" + final_;
    if (lastSlice && (final_.empty() || final_.back() != '/'))
        final_ = final_ + "/";
    while (final_.find("//") != std::string::npos)
        final_ = replaceAll(final_, "//", "/");

    auto parts = splitStr(final_, '/');
    final_.clear();
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) final_ += "/";
        final_ += addCommasToMove(parts[i]);
    }
    return final_;
}

// ===========================================================================
// karnify — WCA numeric slash-format alg part -> Karnotation display string.
// Accepts only the alg portion (no bracket suffix).
// Commas are stripped; numeric moves that have no Karn name stay as numeric
// but with commas removed (e.g. "-1,2" -> "-12").
// ===========================================================================
inline std::string karnify(const std::string& algPart) {
    std::string out = trimStr(algPart);

    bool startSlice = !out.empty() && (out.front() == '/' || out.front() == '\\');

    out = replaceAll(out, "/", " ");
    out = replaceAll(out, "\\", " ");

    out = replaceWithVector(" " + trimStr(out) + " ", WCA_TO_KARN);
    out = trimStr(out);

    std::string prev;
    do {
        prev = out;
        out = replaceAll(out, "  ", " ");
    } while (out != prev);

    out = replaceAll(out, ",", "");

    if (startSlice) out = "/" + out;
    return out;
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
//
// NOTE: not called anywhere yet.
// ===========================================================================
namespace karnifycs_detail {

// slotState[24]: 0 = corner slot, 1 = edge slot.
// Layout matches sq1opt FullPosition: indices 0-11 = top layer, 12-23 = bottom.

inline void kcTopTurn(int slotState[24], int t) {
    t = ((t % 12) + 12) % 12;
    for (int k = 0; k < t; k++) {
        int last = slotState[11];
        for (int i = 11; i > 0; i--) slotState[i] = slotState[i - 1];
        slotState[0] = last;
    }
}

inline void kcBotTurn(int slotState[24], int d) {
    d = ((d % 12) + 12) % 12;
    for (int k = 0; k < d; k++) {
        int last = slotState[23];
        for (int i = 23; i > 12; i--) slotState[i] = slotState[i - 1];
        slotState[12] = last;
    }
}

inline void kcSlice(int slotState[24]) {
    for (int i = 6; i < 12; i++)
        std::swap(slotState[i], slotState[i + 6]);
}

// A square layer: 4 corners (2 slots each = 00) + 4 edges (1 slot = 1).
// Valid 12-slot patterns are the 3 rotations of [0,0,1, 0,0,1, 0,0,1, 0,0,1]:
//   edges fall at positions with remainder 0, 1, or 2 (mod 3).
inline bool kcLayerIsSquare(const int slotState[], int base) {
    for (int rem = 0; rem < 3; rem++) {
        bool ok = true;
        for (int i = 0; i < 12; i++) {
            if (slotState[base + i] != (i % 3 == rem ? 1 : 0)) { ok = false; break; }
        }
        if (ok) return true;
    }
    return false;
}

inline bool kcInCubeshape(const int slotState[24]) {
    return kcLayerIsSquare(slotState, 0) && kcLayerIsSquare(slotState, 12);
}

// Parse a position hex string into slotState[24]. Returns true on success.
inline bool kcParseState(const std::string& posHex, int slotState[24]) {
    if (posHex.size() < 16) return false;
    int j = 0;
    for (int i = 0; i < 16 && j < 24; i++) {
        char c = posHex[i];
        bool isCorner = (c >= 'A' && c <= 'H') || (c >= 'a' && c <= 'h') ||
                        c == 'U' || c == 'V' || c == 'W';
        if (isCorner) {
            if (j + 1 >= 24) return false;
            slotState[j++] = 0; slotState[j++] = 0;
        } else {
            if (j >= 24) return false;
            slotState[j++] = 1;
        }
    }
    return j == 24;
}

// Apply a "t,d" move token to the slot state.
inline void kcApplyTurnToken(int slotState[24], const std::string& token) {
    size_t comma = token.find(',');
    if (comma == std::string::npos) return;
    try {
        int u = std::stoi(token.substr(0, comma));
        int d = std::stoi(token.substr(comma + 1));
        kcTopTurn(slotState, u);
        kcBotTurn(slotState, d);
    } catch (...) {}
}

// Apply a single WCA_TO_KARN / WCA_TO_KARN_OCS substitution to one space-padded
// numeric token, then strip commas.
inline std::string kcSubstituteToken(
    const std::string& token,
    const std::vector<std::pair<std::string,std::string>>& table)
{
    std::string out = replaceWithVector(" " + token + " ", table);
    out = trimStr(out);
    out = replaceAll(out, ",", "");
    return out;
}

} // namespace karnifycs_detail

inline std::string karnifycs(
    const std::string& algWCA,
    const std::string& startStateHex,
    bool generatorMode)
{
    using namespace karnifycs_detail;

    // --- initialise slot state ---
    int slotState[24];
    // Solved state: top = CC E CC E CC E CC E, bottom = E CC E CC E CC E CC
    const int solved[24] = {0,0,1,0,0,1,0,0,1,0,0,1, 1,0,0,1,0,0,1,0,0,1,0,0};
    if (generatorMode || !kcParseState(startStateHex, slotState)) {
        for (int i = 0; i < 24; i++) slotState[i] = solved[i];
    }

    // --- parse alg ---
    // An alg like "/-3,0/-1,2/4,-2/" splits on '/' into {"", "-3,0", "-1,2", "4,-2", ""}.
    // Each empty token represents a slice boundary; each non-empty token is a (u,d) move.
    // We track the shape state: slices change shape; layer turns don't.
    // The cubeshape flag for each move is checked BEFORE applying the move.

    std::vector<std::string> tokens = splitStr(replaceAll(algWCA, "\\", "/"), '/');

    // Strip a potential leading slash by noting that tokens[0] will be empty.
    // We'll reconstruct the output with '/' between every pair.

    std::string out;
    bool needSlash = false;

    // Emit a leading '/' if the alg starts with one (tokens[0] is empty).
    bool leadingSlash = !tokens.empty() && tokens[0].empty();
    if (leadingSlash) {
        out += '/';
        kcSlice(slotState);          // the leading slash is a real slice
    }

    // Walk remaining tokens: pairs of (move, slash) where the slash is the next empty token.
    // More precisely: tokens[0] is already handled above. We process tokens[1..n-1].
    for (size_t i = 1; i < tokens.size(); i++) {
        const std::string tok = trimStr(tokens[i]);

        if (tok.empty()) {
            // Another slice
            out += '/';
            kcSlice(slotState);
            continue;
        }

        // Determine cubeshape at the moment this turn executes.
        bool inCS = kcInCubeshape(slotState);
        const auto& table = inCS ? WCA_TO_KARN : WCA_TO_KARN_OCS;

        out += kcSubstituteToken(tok, table);

        // Advance shape state.
        kcApplyTurnToken(slotState, tok);
    }

    return out;
}
