#pragma once

#include <QString>
#include <QStringList>
#include <string>
#include <vector>
#include <utility>
#include <map>

// ============================================================
// Ergonomics Rating — pure C++ translation of alg_rater.html
// Uses KARNOTATION from karnotation.h for unkarnify.
// ============================================================

struct AlgRating
{
    double FINAL;
    std::string sliceStart;
};

// ─────────────────────────────────────────────────────────
// Parsing & Conversion Functions
// ─────────────────────────────────────────────────────────

/// Converts Karnotation (e.g., "bjj", "fv") to WCA numeric notation (e.g., "1,0")
std::string unkarnify(const std::string &algIn);

/// Rates an algorithm for ergonomics based on move values and slice patterns
AlgRating rateAlg(const std::string &algRaw, bool initial_top_A,
                  double W1, double W2, double W3, double W4, double W5);

/// Rates and sorts multiple solutions by ergonomic score (highest first)
std::vector<std::pair<QString, double>>
rateAndSort(const QStringList &solutionLines, const QString &posHex, bool useKarnotation);

// ─────────────────────────────────────────────────────────
// Helper Functions (internal use, but exposed for testing)
// ─────────────────────────────────────────────────────────

std::vector<std::string> splitStr(const std::string &s, char delim);
std::string addCommasToMove(const std::string &move);
std::string getAlignment(bool topA, bool bottomA);
std::string unkarnifyHelp(const std::string &scramble);
std::string replaceShorthands(std::string scramble);
std::pair<int, int> getOverwork(const std::vector<std::string> &moves);
int getMoveValue(bool startA, bool upslice, const std::string &move);
