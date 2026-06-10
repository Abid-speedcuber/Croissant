#pragma once

#include <string>

void sq1optSetTableDirectory(const std::string& dir);
void sq1optRequestStop();
int sq1optMain(int argc, char* argv[]);

// Direct position injection — bypasses string encoding/decoding.
// pos[24] uses FullPosition encoding: concrete 0-15, partial corners <0, partial edges >15.
// middle: 1=square, -1=kite, 0=ignore equator.
void sq1optSetPosition(const int pos[24], int middle);
