#include "karnotation.h"
#include "sq1-logic.h"

#include <cstring>
#include <sstream>
#include <string>
#include <vector>

void sq1optSetExtendedOutput(bool val);

std::vector<int> twoGenPreadf(const int pos[24], int two_gen, bool first_match_only);
bool cornersAre2GenSolvable(const int pos[24], int two_gen);

// Static initializer — enables extended output for the WASM bridge before main()
struct EnableExtended { EnableExtended() { sq1optSetExtendedOutput(true); } };
static EnableExtended s_enableExtended;

namespace {
char *copy_allocated(const std::string &value) {
    auto *result = new char[value.size() + 1];
    std::memcpy(result, value.data(), value.size());
    result[value.size()] = '\0';
    return result;
}

std::string json_bool(bool value) {
    return value ? "true" : "false";
}
}

extern "C" char *sq1_web_unkarnify_alloc(const char *input) {
    try {
        return copy_allocated(replaceShorthands(unkarnifyHelp(input ? input : "")));
    } catch (...) {
        return copy_allocated("");
    }
}

extern "C" char *sq1_web_karnify_alloc(const char *input, const char *position, bool generator) {
    try {
        if (position && position[0] != '\0') {
            return copy_allocated(karnifycs(input ? input : "", position, generator));
        }
        return copy_allocated(karnify(input ? input : ""));
    } catch (...) {
        return copy_allocated("");
    }
}

extern "C" char *sq1_web_rate_algorithm_json_alloc(const char *algorithm, bool initial_top_a) {
    try {
        const AlgRating rating = rateAlg(algorithm ? algorithm : "", initial_top_a, 34, 100, 38, 10);
        std::ostringstream out;
        out << "{\"finalScore\":" << rating.FINAL
            << ",\"phase1\":" << rating.PHASE1
            << ",\"phase2\":" << rating.PHASE2
            << ",\"phase3\":" << rating.PHASE3
            << ",\"phase4\":" << rating.PHASE4
            << ",\"ergoUp\":" << rating.ergo_up
            << ",\"ergoDown\":" << rating.ergo_down
            << ",\"sliceCount\":" << rating.sliceCount
            << ",\"movement\":" << rating.movement
            << ",\"bonus\":" << rating.bonus
            << ",\"valid\":" << json_bool(rating.valid)
            << ",\"sliceStart\":" << (rating.sliceStart.empty() ? 0 : static_cast<int>(rating.sliceStart.front()))
            << "}";
        return copy_allocated(out.str());
    } catch (...) {
        return copy_allocated("{\"finalScore\":0,\"phase1\":0,\"phase2\":0,\"phase3\":0,\"phase4\":0,\"ergoUp\":0,\"ergoDown\":0,\"sliceCount\":0,\"movement\":0,\"bonus\":0,\"valid\":false,\"sliceStart\":0}");
    }
}

extern "C" char *sq1_web_two_gen_status_json_alloc(const int *position) {
    if (!position) {
        return copy_allocated("{\"compatibility\":0,\"cornersTwo\":false,\"cornersPseudo\":false}");
    }
    const bool corners_two = cornersAre2GenSolvable(position, 2);
    const bool corners_pseudo = cornersAre2GenSolvable(position, 1);
    const int compatibility = !twoGenPreadf(position, 2, true).empty() ? 2
        : !twoGenPreadf(position, 1, true).empty() ? 1
        : 0;
    std::ostringstream out;
    out << "{\"compatibility\":" << compatibility
        << ",\"cornersTwo\":" << json_bool(corners_two)
        << ",\"cornersPseudo\":" << json_bool(corners_pseudo)
        << "}";
    return copy_allocated(out.str());
}

extern "C" void sq1_web_free_string(char *value) {
    delete[] value;
}
