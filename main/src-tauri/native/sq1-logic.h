#pragma once

#ifndef SQ1OPT_NO_QT
#include <QString>
#include <QStringList>
#endif
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
    // Breakdown (only meaningful when valid==true)
    double PHASE1{0}, PHASE2{0}, PHASE3{0}, PHASE4{0};
    int sliceCount{0};
    double ergo_up{0}, ergo_down{0};
    int movement{0}, bonus{0};
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
#ifndef SQ1OPT_NO_QT
std::vector<std::pair<QString, double>>
rateAndSort(const QStringList &solutionLines, const QString &posHex, bool useKarnotation);
#endif

// ─────────────────────────────────────────────────────────
// Runtime-configurable rating weights & move values
// ─────────────────────────────────────────────────────────
// W1..W4 and the per-move ergonomics table (MOVE_VALUES) default to the
// hardcoded values below, but can be overridden at runtime by the UI (see
// the "Configure ergonomics rater" settings). All rateAlg call sites should
// read the current weights via getRatingWeights() instead of hardcoding
// 34, 100, 38, 10 directly, so overrides apply immediately everywhere.

struct RatingWeights
{
    double w1{34}, w2{100}, w3{38}, w4{10};
};

/// Current W1..W4 weights (defaults unless overridden via setRatingWeights).
RatingWeights getRatingWeights();

/// Overrides the current W1..W4 weights.
void setRatingWeights(double w1, double w2, double w3, double w4);

/// Overrides a single MOVE_VALUES entry. Returns false (no-op) if `key` isn't
/// a recognised move-value key, so callers can reject bad input.
bool setMoveValueOverride(const std::string &key, int value);

/// Reverts weights and all move-value overrides back to their hardcoded
/// defaults.
void resetRatingConfig();

// ─────────────────────────────────────────────────────────
// Helper Functions (internal use, but exposed for testing)
// ─────────────────────────────────────────────────────────

std::pair<int, int> getOverwork(const std::vector<std::string> &moves);
int getMoveValue(bool startA, bool upslice, const std::string &move);
