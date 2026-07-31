#pragma once

#include <string>
#include <vector>

void sq1optSetTableDirectory(const std::string& dir);
void sq1optRequestStop();
int sq1optMain(int argc, char* argv[]);

// Direct position injection — bypasses string encoding/decoding.
// pos[24] uses FullPosition encoding: concrete 0-15, partial corners <0, partial edges >15.
// middle: 1=-, -1=+, 0=ignore.
void sq1optSetPosition(const int pos[24], int middle);

// valid preadfs when 2g or p2g is enabled
// specificAngleBot: passed from UI
// firstMatchOnly: stop and return after the first valid preadf
std::vector<int> twoGenPreadf(const int pos[24], int twoGen, bool specificAngleBot = false, bool firstMatchOnly = false);

bool has2GenCorners(const int pos[24]);

bool partialHas2GenCorners(const int pos[24]);

// Whether the corners are 2g, checked once per valid preadf candidate
// (twoGen: 2 = 2g, 1 = p2g, 0 = no restriction → always true)
// specificAngleBot: from UI. restrict preadf candidates
// This is the entry point the solver guards and the Solve-button gate should use.
bool cornersAre2GenSolvable(const int pos[24], int twoGen, bool specificAngleBot = false);
