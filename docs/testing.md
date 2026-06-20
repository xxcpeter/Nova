# Nova Testing Guide

Nova uses CTest to run both C++ compiler tests and Nova-written tool tests.

This document explains the test layout, common commands, and how to add or update tests.

---

## 1. Running Tests

Build first:

```bash
cmake -S . -B build
cmake --build build --parallel
```

Run all tests:

```bash
ctest --test-dir build -j8 --output-on-failure
```

Run tests matching a name:

```bash
ctest --test-dir build -R nova_tool_codegen -j8 --output-on-failure
```

Run tests with a label:

```bash
ctest --test-dir build -L nova_tool -j8 --output-on-failure
```

Run self-host tests:

```bash
ctest --test-dir build -L selfhost --output-on-failure
```

Run the self-host script directly:

```bash
scripts/self_host.sh
```

---

## 2. Test Categories

Nova has two broad groups of tests:

```text
C++ compiler tests
Nova-written tool tests
```

The C++ compiler tests validate the seed compiler implementation in `src/`.

The Nova-written tool tests validate tools under `tools/` and libraries under `lib/`.

---

## 3. C++ Compiler Tests

### `tests/lexer/`

```text
tests/lexer/positive/
tests/lexer/negative/
```

### `tests/parser/`

```text
tests/parser/positive/
tests/parser/negative/
```

### `tests/sema/`

```text
tests/sema/positive/
tests/sema/negative/
```

### `tests/codegen/`

```text
tests/codegen/positive/
```

These compile Nova programs to C, compile the generated C, run the executable, and compare stdout.

### `tests/import/`

```text
tests/import/positive/
tests/import/negative/
```

These validate source loader import behavior, including missing import, cyclic import, duplicate import, and path normalization.

---

## 4. Nova Tool Tests

Nova-written tool tests live under:

```text
tests/tools/
```

The current layout is:

```text
tests/tools/<tool>/positive/
tests/tools/<tool>/negative/
tests/tools/frontend/positive/<mode>/
tests/tools/frontend/negative/<mode>/
tests/tools/frontend_import/positive/<mode>/<case>/
tests/tools/frontend_import/negative/<mode>/<case>/
tests/tools/compile/positive/
tests/tools/cli/negative/
```

---

## 5. Tool Test Groups

### Tokenizer

```text
tests/tools/tokenizer/positive/
```

Tests:

```text
tools/nova_tokenizer.nv
lib/tokenizer.nv
```

### Parser

```text
tests/tools/parser/positive/
tests/tools/parser/negative/
```

Tests:

```text
tools/nova_parser.nv
lib/parser.nv
lib/parse_tree.nv
```

### Checker

```text
tests/tools/checker/positive/
tests/tools/checker/negative/
```

Early declaration and structural checks.

### Typecheck

```text
tests/tools/typecheck/positive/
tests/tools/typecheck/negative/
```

Expression-level checker tests.

### Frontend

```text
tests/tools/frontend/positive/tokens/
tests/tools/frontend/positive/parse/
tests/tools/frontend/positive/check/
tests/tools/frontend/negative/parse/
tests/tools/frontend/negative/check/
tests/tools/frontend/smoke/
```

### Frontend import

```text
tests/tools/frontend_import/positive/check/<case>/
tests/tools/frontend_import/negative/check/<case>/
```

### Nova codegen

```text
tests/tools/codegen/positive/
```

These compile `tools/nova_codegen.nv`, use it to generate C, compile the generated C, run the executable, and compare stdout.

### Nova compile driver

```text
tests/tools/compile/positive/
```

These test `tools/nova_compile.nv`.

### CLI tests

```text
tests/tools/cli/negative/
```

These compile Nova-written tools and check bad-args usage diagnostics.

---

## 6. Test Drivers

CTest helper scripts live in:

```text
cmake/
```

Important drivers:

```text
RunTextCompareTest.cmake
RunCodegenTest.cmake
RunNovaToolTest.cmake
RunNovaCodegenToolTest.cmake
RunNovaToolCliTest.cmake
```

### `RunNovaToolTest.cmake`

Used for positive and negative Nova tool tests.

Typical flow:

```text
1. nova_compile tool.nv -> tool.c
2. cc tool.c runtime/nova_runtime.c -> tool executable
3. run tool executable
4. compare output file or stderr with EXPECT
```

Negative tests set:

```text
EXPECT_FAILURE=ON
STRIP_RUNTIME_PREFIX=ON
```

### `RunNovaCodegenToolTest.cmake`

Used for Nova-written codegen/compile style tests.

Flow:

```text
1. compile Nova tool
2. run tool on input.nv -> generated C
3. compile generated C
4. run final executable
5. compare stdout
```

### `RunNovaToolCliTest.cmake`

Used for CLI bad-args tests.

Flow:

```text
1. compile Nova tool
2. run tool with CLI_ARGS
3. expect failure
4. compare stderr
```

---

## 7. Golden Files

Tests compare against source-controlled golden files:

```text
*.out
*.err
*.tok
*.check
```

Do not add these patterns to `.gitignore`.

---

## 8. Golden Update Policy

Only update golden files when behavior intentionally changes:

```text
diagnostic format changes
source location correction
parse/tree output changes
runtime/builtin behavior changes
expected stdout changes
```

Recommended workflow:

```text
1. Run the failing test.
2. Confirm the actual output is the correct new behavior.
3. Manually update the matching golden file.
4. Rerun the targeted test.
5. Rerun the relevant label group.
6. Run full CTest.
```

Do not update golden files to match temporary debug output.

---

## 9. Common CTest Commands

Run all:

```bash
ctest --test-dir build -j8 --output-on-failure
```

Nova tool tests:

```bash
ctest --test-dir build -L nova_tool -j8 --output-on-failure
```

Codegen tests:

```bash
ctest --test-dir build -L codegen -j8 --output-on-failure
```

Import tests:

```bash
ctest --test-dir build -L import -j8 --output-on-failure
```

Self-host tests:

```bash
ctest --test-dir build -L selfhost --output-on-failure
```

CLI tests:

```bash
ctest --test-dir build -L cli --output-on-failure
```

---

## 10. Adding a C++ Codegen Test

Add:

```text
tests/codegen/positive/name.nv
tests/codegen/positive/name.out
```

Then register:

```cmake
add_nova_cpp_codegen_positive_test(name)
```

---

## 11. Adding a Nova Codegen Tool Test

Add:

```text
tests/tools/codegen/positive/name.nv
tests/tools/codegen/positive/name.out
```

Then register:

```cmake
add_nova_codegen_tool_test(name)
```

---

## 12. Adding a Nova Compile Tool Test

Add:

```text
tests/tools/compile/positive/name.nv
tests/tools/compile/positive/name.out
```

Then register:

```cmake
add_nova_compile_tool_test(name)
```

---

## 13. Adding a Frontend Test

Positive token mode:

```text
tests/tools/frontend/positive/tokens/name.nv
tests/tools/frontend/positive/tokens/name.tok
```

Register:

```cmake
add_nova_frontend_positive_test(tokens name tok)
```

Positive parse mode:

```text
tests/tools/frontend/positive/parse/name.nv
tests/tools/frontend/positive/parse/name.out
```

Register:

```cmake
add_nova_frontend_positive_test(parse name out)
```

Positive check mode:

```text
tests/tools/frontend/positive/check/name.nv
tests/tools/frontend/positive/check/name.check
```

Register:

```cmake
add_nova_frontend_positive_test(check name check)
```

Negative test:

```text
tests/tools/frontend/negative/<mode>/name.nv
tests/tools/frontend/negative/<mode>/name.err
```

Register:

```cmake
add_nova_frontend_negative_test(<mode> name)
```

---

## 14. Adding an Import Test

Positive import tests:

```text
tests/import/positive/<name>/
  main.nv
  main.out
  helper files...
```

Register:

```cmake
add_nova_import_codegen_positive_test(name)
```

Negative parse import tests:

```text
tests/import/negative/<name>/
  main.nv
  main.err
```

Register:

```cmake
add_nova_import_parse_negative_test(name)
```

Negative sema import tests:

```cmake
add_nova_import_sema_negative_test(name)
```

---

## 15. Adding a CLI Test

Add:

```text
tests/tools/cli/negative/name.err
```

Register:

```cmake
add_nova_tool_cli_negative_test(group tool name)
```

Expected files usually omit the runtime prefix if the test runner strips it:

```text
Usage: nova_compile <input.nv> <output.c>
```

---

## 16. Debugging Failed Tests

Run a failing test with output:

```bash
ctest --test-dir build -R test_name --output-on-failure
```

Nova tool tests write work files under:

```text
build/nova_tool_tests/
```

Common files include:

```text
tool.c
tool executable
generated C
generated executable
actual output
```

If generated C fails to compile, inspect the work directory and the test driver output.

---

## 17. Final Milestone Sweep

Before marking a milestone complete, run:

```bash
rm -rf build tmp
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build -j8 --output-on-failure
scripts/self_host.sh
```
