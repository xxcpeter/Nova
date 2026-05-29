#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

cmake -S . -B build
cmake --build build --parallel

WORK="${TMPDIR:-/tmp}/nova_bootstrap_stage1"
rm -rf "$WORK"
mkdir -p "$WORK"

echo "[1/4] Build stage0 nova_codegen with C++ seed compiler"
./build/nova_compile tools/nova_codegen.nv "$WORK/nova_codegen_stage0.c"

echo "[2/4] Compile stage0"
cc "$WORK/nova_codegen_stage0.c" runtime/nova_runtime.c -I runtime -o "$WORK/nova_codegen_stage0"

echo "[3/4] Use stage0 to generate stage1 C"
"$WORK/nova_codegen_stage0" tools/nova_codegen.nv "$WORK/nova_codegen_stage1.c"

echo "[4/4] Compile stage1"
cc "$WORK/nova_codegen_stage1.c" runtime/nova_runtime.c -I runtime -o "$WORK/nova_codegen_stage1"

echo "stage1 bootstrap OK"
echo "work dir: $WORK"