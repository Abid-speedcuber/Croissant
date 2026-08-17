#include "sq1-logic.h"
#include <cstring>

struct Sq1RatingResult {
    double final_score;
    double phase1, phase2, phase3, phase4;
    double ergo_up, ergo_down;
    int slice_count, movement, bonus;
    bool valid;
    char slice_start;
};

extern "C" bool sq1_rate_algorithm(const char *algorithm, bool initial_top_a,
                                    Sq1RatingResult *output) {
    if (!output) return false;
    try {
        const RatingWeights w = getRatingWeights();
        const AlgRating rating = rateAlg(algorithm ? algorithm : "", initial_top_a, w.w1, w.w2, w.w3, w.w4);
        output->final_score = rating.FINAL;
        output->phase1 = rating.PHASE1; output->phase2 = rating.PHASE2;
        output->phase3 = rating.PHASE3; output->phase4 = rating.PHASE4;
        output->ergo_up = rating.ergo_up; output->ergo_down = rating.ergo_down;
        output->slice_count = rating.sliceCount; output->movement = rating.movement;
        output->bonus = rating.bonus; output->valid = rating.valid;
        output->slice_start = rating.sliceStart.empty() ? '\0' : rating.sliceStart.front();
        return rating.valid;
    } catch (...) {
        std::memset(output, 0, sizeof(*output));
        return false;
    }
}

// Runtime configuration for the ergonomics rater — see sq1-logic.h.
extern "C" void sq1_set_rating_weights(double w1, double w2, double w3, double w4) {
    setRatingWeights(w1, w2, w3, w4);
}

extern "C" bool sq1_set_move_value(const char *key, int value) {
    return key ? setMoveValueOverride(key, value) : false;
}

extern "C" void sq1_reset_rating_config() {
    resetRatingConfig();
}
