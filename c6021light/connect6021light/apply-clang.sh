#!/bin/sh

CLANG_BINARY="/c/Program Files/LLVM/bin/clang-format.exe"

for filename in `find src -iname "*.cpp" -o -iname "*.h"`
do
    echo "Formatting $filename"
    "$CLANG_BINARY" -i "$filename"
done