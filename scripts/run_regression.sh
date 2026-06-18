#!/usr/bin/env bash
set -euo pipefail

cmake --build build --parallel
ctest --test-dir build -j8 --output-on-failure
scripts/self_host.sh