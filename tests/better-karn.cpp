/*
 * better-karn.cpp
 * Standalone CLI tool for testing cubeshape-aware karnotation.
 *
 * Usage:
 *   better-karn "<alg>"              — uses solved state as starting position
 *   better-karn "<alg>" "<posHex>"   — uses given position string (e.g. A1B2C3D45E6F7G8H-)
 *   better-karn "<alg>" "<posHex>" g — generator mode (g flag)
 *
 * Examples:
 *   better-karn "/-3,0/-1,2/4,-2/"
 *   better-karn "3,0/0,-3/1,-2/-4,2/" "A1B2C3D45E6F7G8H-"
 *   better-karn "3,0/0,-3/" "A1B2C3D45E6F7G8H-" g
 *
 * Compile:
 *   g++ -std=c++17 -o better-karn better-karn.cpp
 *
 * karnotation.h must be in the same directory or on the include path.
 */

#if __has_include("karnotation.h")
#include "karnotation.h"
#elif __has_include("../main/sq1-core/karnotation.h")
#include "../main/sq1-core/karnotation.h"
#else
#error "Fatal: karnotation.h not found in ./ or ../main/sq1-core/"
#endif

#include <iostream>
#include <string>

static void printUsage() {
    std::cout << "Usage:\n"
              << "  better-karn \"<alg>\"\n"
              << "  better-karn \"<alg>\" \"<posHex>\"\n"
              << "  better-karn \"<alg>\" \"<posHex>\" g\n\n"
              << "posHex example: A1B2C3D45E6F7G8H-  (solved state)\n"
              << "g flag: generator mode\n\n"
              << "Outputs:\n"
              << "  plain karn   : result of karnify() — cubeshape-unaware\n"
              << "  smart karn   : result of karnifycs() — cubeshape-aware\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string alg      = argv[1];
    std::string posHex   = argc >= 3 ? argv[2] : "A1B2C3D45E6F7G8H-";
    bool generatorMode   = argc >= 4 && std::string(argv[3]) == "g";

    std::cout << "Input      : " << alg        << "\n";
    std::cout << "Position   : " << posHex     << "\n";
    std::cout << "Mode       : " << (generatorMode ? "generator" : "solver") << "\n";
    std::cout << "\n";

    // Strip parens and normalize whitespace before karnifying
    std::string cleaned = alg;
    cleaned = replaceAll(cleaned, "(", "");
    cleaned = replaceAll(cleaned, ")", "");
    // collapse spaces around slashes
    {
        std::string prev;
        do {
            prev = cleaned;
            cleaned = replaceAll(cleaned, " /", "/");
            cleaned = replaceAll(cleaned, "/ ", "/");
            cleaned = replaceAll(cleaned, "  ", " ");
        } while (cleaned != prev);
    }
    cleaned = trimStr(cleaned);

    std::cout << "Cleaned    : " << cleaned << "\n\n";

    // Debug: trace cubeshape state after every slice
    {
        int dbgState[24];
        if (!karnifycs_detail::kcParseState(posHex, dbgState)) {
            const int solved[24] = {0,0,1,0,0,1,0,0,1,0,0,1, 1,0,0,1,0,0,1,0,0,1,0,0};
            for (int i = 0; i < 24; i++) dbgState[i] = solved[i];
        }

        auto printState = [&]() {
            for (int i = 0; i < 24; i++) std::cout << dbgState[i];
        };

        auto tokens = splitStr(replaceAll(cleaned, "\\", "/"), '/');
        int sliceNum = 0;
        std::cout << "CS trace:\n";
        std::cout << "  start          : ";
        printState();
        std::cout << "  " << (karnifycs_detail::kcInCubeshape(dbgState) ? "IN" : "OUT") << "\n";

        for (size_t i = 0; i < tokens.size(); i++) {
            std::string tok = trimStr(tokens[i]);
            if (!tok.empty()) {
                karnifycs_detail::kcApplyTurnToken(dbgState, tok);
            }
            // every separator between tokens is a slice; also trailing slash
            bool isLastToken = (i + 1 == tokens.size());
            bool hasSeparatorAfter = !isLastToken; // there's always a '/' after each non-last token
            bool isTrailingSlash  = isLastToken && tok.empty();
            if (hasSeparatorAfter || isTrailingSlash) {
                karnifycs_detail::kcSlice(dbgState);
                sliceNum++;
                std::cout << "  after slice " << sliceNum << " (after " << (tok.empty() ? "bare/" : tok) << ") : ";
                printState();
                std::cout << "  " << (karnifycs_detail::kcInCubeshape(dbgState) ? "IN" : "OUT") << "\n";
            }
        }
        std::cout << "\n";
    }

    std::string plain = karnify(cleaned);
    std::string smart = karnifycs(cleaned, posHex, generatorMode);

    std::cout << "plain karn : " << plain << "\n";
    std::cout << "smart karn : " << smart << "\n";

    return 0;
}