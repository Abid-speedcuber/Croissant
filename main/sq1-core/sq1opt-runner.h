#pragma once

#include <string>
#include <vector>

void sq1optSetTableDirectory(const std::string& dir);
void sq1optRequestStop();
int sq1optMain(int argc, char* argv[]);

// Direct position injection — bypasses string encoding/decoding.
// pos[24] uses FullPosition encoding: concrete 0-15, partial corners <0, partial edges >15.
// middle: 1=square, -1=kite, 0=ignore equator.
void sq1optSetPosition(const int pos[24], int middle);

// Valid preadf D rotations (doBot amounts) for 2-gen (twoGen==2) / pseudo-2-gen
// (twoGen==1) on the given position. Empty => the position can't be 2-genned in
// that mode. twoGen==0 returns {0}. Single source of truth shared by the solver
// (FullPosition::findPreadf) and the UI's Solve-button enable check.
// firstMatchOnly: stop and return after the first valid preadf (the UI only needs
// to know whether any exists; the solver needs the complete set).
std::vector<int> twoGenPreadf(const int pos[24], int twoGen, bool firstMatchOnly = false);

// Whether the 8 corners can be solved using only pseudo-2-gen moves (the corner
// permutation lies in the 2-gen corner group). Single source of truth shared by
// the solver's keep-cube-shape p2g guard and the UI's Solve-button enable check.
bool has2GenCorners(const int pos[24]);

// Partial-aware version of has2GenCorners: resolves partially-specified corners
// (U/V/W) by elimination, then defers to has2GenCorners. For fully-concrete
// positions it returns exactly has2GenCorners(pos).
// NOTE: like has2GenCorners it currently assumes the bottom-left corners are the
// solved G,H pair and is evaluated on the position as-is (not per preadf
// candidate) — see the logged refinement.
bool partialHas2GenCorners(const int pos[24]);
