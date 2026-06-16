#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

cmake -S . -B build
cmake --build build --parallel

WORK="./tmp/nova_self_host"
rm -rf "$WORK"
mkdir -p "$WORK"
mkdir -p "$WORK/stage0"
mkdir -p "$WORK/stage1"
mkdir -p "$WORK/stage2"
mkdir -p "$WORK/programs"

compile_c() {
  local c_file="$1"
  local exe_file="$2"
  cc "$c_file" runtime/nova_runtime.c -I runtime -o "$exe_file"
}

run_codegen() {
  local codegen="$1"
  local input="$2"
  local output_c="$3"
  "$codegen" "$input" "$output_c"
}

run_and_capture() {
  local exe="$1"
  local output="$2"
  "$exe" > "$output"
}

compare_files() {
  local a="$1"
  local b="$2"
  diff -u "$a" "$b"
}

PROGRAMS=(
  "tests/tools/codegen/hello.nv"
  "tests/tools/codegen/recursion.nv"
  "tests/tools/codegen/vec_basic.nv"
  "tests/tools/codegen/vec_str_basic.nv"
  "tests/tools/codegen/vec_struct_basic.nv"
  "tests/tools/codegen/string_plus.nv"
  "tests/tools/codegen/string_ends_with.nv"
  "tests/tools/codegen/struct_basic.nv"
  "tests/tools/codegen/enum_basic.nv"
  "tests/tools/codegen/import_basic/main.nv"
)

echo "[self-host] building stage0"
run_codegen ./build/nova_compile tools/nova_codegen.nv "$WORK/stage0/nova_codegen.c"
compile_c "$WORK/stage0/nova_codegen.c" "$WORK/stage0/nova_codegen"

echo "[self-host] building stage1"
run_codegen "$WORK/stage0/nova_codegen" tools/nova_codegen.nv "$WORK/stage1/nova_codegen.c"
compile_c "$WORK/stage1/nova_codegen.c" "$WORK/stage1/nova_codegen"

echo "[self-host] building stage2"
run_codegen "$WORK/stage1/nova_codegen" tools/nova_codegen.nv "$WORK/stage2/nova_codegen.c"
compile_c "$WORK/stage2/nova_codegen.c" "$WORK/stage2/nova_codegen"

for program in "${PROGRAMS[@]}"; do
  if [[ -f "$program" ]]; then
    name=$(basename "$program")
    echo "[self-host] checking program $name"
    run_codegen "$WORK/stage1/nova_codegen" "$program" "$WORK/programs/$name.stage1.c"
    compile_c "$WORK/programs/$name.stage1.c" "$WORK/programs/$name.stage1"
    run_and_capture "$WORK/programs/$name.stage1" "$WORK/programs/$name.stage1.out"

    run_codegen "$WORK/stage2/nova_codegen" "$program" "$WORK/programs/$name.stage2.c"
    compile_c "$WORK/programs/$name.stage2.c" "$WORK/programs/$name.stage2"
    run_and_capture "$WORK/programs/$name.stage2" "$WORK/programs/$name.stage2.out"

    compare_files "$WORK/programs/$name.stage1.out" "$WORK/programs/$name.stage2.out"
  fi
done
echo "[self-host] all checks passed"
