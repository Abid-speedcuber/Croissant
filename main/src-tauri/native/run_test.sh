#!/bin/bash
# Compile the karnify test directly with g++

cd "$(dirname "$0")"

echo "Compiling test_karnify..."
g++ -std=c++17 -I./native -O2 ./test_karnify.cpp -o ./test_karnify_bin

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful!"
    echo ""
    echo "Usage: paste solver output below, end with 'Stop requested.' or Ctrl+D"
    echo ""
    ./test_karnify_bin
else
    echo "✗ Compilation failed"
    exit 1
fi
