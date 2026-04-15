#pragma once

#include <QString>
#include <QStringList>
#include <string>
#include <vector>
#include <utility>
#include <map>
#include <limits>

// ============================================================
// Ergonomics Rating — pure C++ translation of alg_rater.html
// ============================================================

struct AlgRating
{
    double FINAL;
    std::string sliceStart;
    bool valid{true}; // false when the alg could not be parsed / rated
};

// ─────────────────────────────────────────────────────────
// Alg Rating Functions
// ─────────────────────────────────────────────────────────

/// Rates an algorithm for ergonomics based on move values and slice patterns.
/// algRaw must be in WCA numeric slash format (or karnotation — unkarnify is called
/// internally if alpha characters are detected).
/// W1=ergo-per-slice, W2=slice-count penalty, W3=overwork-per-slice, W4=bonus weight.
/// No constant term is added; call rateAndSort to get median-normalised scores.
AlgRating rateAlg(const std::string &algRaw, bool initial_top_A,
                  double W1, double W2, double W3, double W4);

/// Rates and sorts multiple solutions by ergonomic score (highest first).
/// solutionLines may contain karn or numeric algs; rating always uses numeric.
std::vector<std::pair<QString, double>>
rateAndSort(const QStringList &solutionLines, const QString &posHex, bool useKarnotation);

// ─────────────────────────────────────────────────────────
// Helper Functions (internal use, but exposed for testing)
// ─────────────────────────────────────────────────────────

std::pair<int, int> getOverwork(const std::vector<std::string> &moves);
int getMoveValue(bool startA, bool upslice, const std::string &move);
