#include "karnotation.h"
#include <cstring>
#include <string>

namespace {
int copy_result(const std::string &value, char *output, std::size_t capacity) {
    if (!output || capacity == 0) return static_cast<int>(value.size() + 1);
    const std::size_t count = value.size() < capacity - 1 ? value.size() : capacity - 1;
    std::memcpy(output, value.data(), count);
    output[count] = '\0';
    return static_cast<int>(value.size() + 1);
}
}

extern "C" int sq1_unkarnify(const char *input, char *output, std::size_t capacity) {
    try { return copy_result(replaceShorthands(unkarnifyHelp(input ? input : "")), output, capacity); }
    catch (...) { return -1; }
}

extern "C" int sq1_karnify(const char *input, char *output, std::size_t capacity) {
    try { return copy_result(karnify(input ? input : ""), output, capacity); }
    catch (...) { return -1; }
}

extern "C" int sq1_karnify_smart(const char *input, const char *position, bool generator,
                                  char *output, std::size_t capacity) {
    try {
        return copy_result(karnifycs(input ? input : "", position ? position : "", generator),
                           output, capacity);
    } catch (...) { return -1; }
}
