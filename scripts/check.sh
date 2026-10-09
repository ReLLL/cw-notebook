#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
cmake -S . -B build-core -DBUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-core --parallel 4
ctest --test-dir build-core --output-on-failure
python3 -m unittest discover -s tests -p 'test_*.py' -v
