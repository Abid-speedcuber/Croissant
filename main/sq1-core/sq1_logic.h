#pragma once

#include <QString>
#include <QStringList>
#include <string>
#include <vector>
#include <utility>
#include <map>

// ============================================================
// Ergonomics Rating — pure C++ translation of alg_rater.html
// ============================================================

struct AlgRating
{
    double FINAL;
    std::string sliceStart;
};

// ─────────────────────────────────────────────────────────
// Alg Rating Functions
// ─────────────────────────────────────────────────────────

/// Rates an algorithm for ergonomics based on move values and slice patterns.
/// algRaw must be in WCA numeric slash format (or karnotation — unkarnify is called
/// internally if alpha characters are detected).
AlgRating rateAlg(const std::string &algRaw, bool initial_top_A,
                  double W1, double W2, double W3, double W4, double W5);

/// Rates and sorts multiple solutions by ergonomic score (highest first).
/// solutionLines may contain karn or numeric algs; rating always uses numeric.
std::vector<std::pair<QString, double>>
rateAndSort(const QStringList &solutionLines, const QString &posHex, bool useKarnotation);

// ─────────────────────────────────────────────────────────
// Helper Functions (internal use, but exposed for testing)
// ─────────────────────────────────────────────────────────

std::pair<int, int> getOverwork(const std::vector<std::string> &moves);
int getMoveValue(bool startA, bool upslice, const std::string &move);
