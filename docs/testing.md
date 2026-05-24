# Nova Testing Guide

Nova uses CTest to run both C++ compiler tests and Nova-written tool tests.

This document explains the test layout, common commands, and how to add new tests.

---

## 1. Running Tests

Build first:

```bash
cmake -S . -B build
cmake --build build
```

Run all tests:

```bash
ctest --test-dir build
```

Run with verbose output:

```bash
ctest --test-dir build --output-on-failure
```

Run tests matching a name:

```bash
ctest --test-dir build -R nova_tool_codegen
```

Run tests with a label:

```bash
ctest --test-dir build -L nova_tool
```

---

## 2. Test Categories

Nova has two broad groups of tests:

```text
C++ compiler tests
Nova-written tool tests
```

The C++ compiler tests validate the main compiler implementation in `src/`.

The Nova-written tool tests validate tools implemented in Nova under `tools/` and libraries under `lib/`.

---

## 3. C++ Compiler Tests

### `tests/lexer/`

Tests the C++ lexer.

```text
tests/lexer/positive/
tests/lexer/negative/
```

Positive tests compare token output.

Negative tests compare lexer error output.

---

### `tests/parser/`

Tests the C++ parser.

```text
tests/parser/positive/
tests/parser/negative/
```

Positive tests compare parse / AST dump output.

Negative tests compare parser diagnostics.

---

### `tests/sema/`

Tests the C++ semantic analyzer.

```text
tests/sema/positive/
tests/sema/negative/
```

These tests cover:

```text
type checking
function calls
structs
enums
vectors
returns
assignments
conditions
scope rules
```

---

### `tests/codegen/`

Tests the C++ compiler code generator.

```text
tests/codegen/positive/
```

These tests compile Nova programs to C, compile the generated C, run the executable, and compare stdout.

---

### `tests/import/`

Tests the C++ source loader and import/include system.

```text
tests/import/positive/
tests/import/negative/
```

Important cases:

```text
simple import
diamond import / include once
missing import
cyclic import
```

---

## 4. Nova Tool Tests

Nova-written tool tests live under:

```text
tests/tools/
```

These tests compile a Nova tool with the C++ compiler, compile the generated C tool, run it, and compare its output.

---

### `tests/tools/tokenizer/`

Tests:

```text
tools/nova_tokenizer.nv
lib/tokenizer.nv
```

The tool reads a Nova source file and outputs tokens.

Typical files:

```text
tests/tools/tokenizer/basic.nv
tests/tools/tokenizer/basic.tok
```

---

### `tests/tools/expr_parser/`

Tests:

```text
tools/nova_expr_parser.nv
lib/parser.nv
lib/parse_tree.nv
```

These tests validate expression-aware parse trees.

Typical files:

```text
tests/tools/expr_parser/expression_precedence.nv
tests/tools/expr_parser/expression_precedence.out
```

Negative parser tests are under:

```text
tests/tools/expr_parser/negative/
```

---

### `tests/tools/checker/`

Tests early Nova-written checker behavior.

```text
tests/tools/checker/positive/
tests/tools/checker/negative/
```

These focus mostly on declaration-level and structural checks.

---

### `tests/tools/typecheck/`

Tests expression-level type checking in the Nova-written checker.

```text
tests/tools/typecheck/positive/
tests/tools/typecheck/negative/
```

These cover:

```text
let initializer type checking
return type checking
if / while conditions
undefined variables
function call arguments
field access
enum member access
vec intrinsic type checking
```

---

### `tests/tools/frontend/`

Tests:

```text
tools/nova_frontend.nv
```

Frontend modes:

```text
tokens
parse
check
```

Layout:

```text
tests/tools/frontend/tokens/
tests/tools/frontend/parse/
tests/tools/frontend/check/
tests/tools/frontend/negative/
tests/tools/frontend/smoke/
```

Smoke tests check that the Nova frontend can check Nova tools.
Smoke expected files usually only require output beginning with:

```text
Check OK
```

---

### `tests/tools/frontend_import/`

Tests frontend behavior on source files that use Nova imports.

```text
tests/tools/frontend_import/check/
tests/tools/frontend_import/negative/
```

These validate Nova-side import expansion through `lib/source_loader.nv`.

---

### `tests/tools/codegen/`

Tests:

```text
tools/nova_codegen.nv
lib/codegen_c.nv
```

These tests compile the Nova-written codegen tool, use it to generate C from a Nova test program, compile that generated C, run it, and compare stdout.

Typical files:

```text
tests/tools/codegen/hello.nv
tests/tools/codegen/hello.out
```

Covered features include:

```text
functions
recursion
if / while
bool logic
strings
runtime calls
structs
enums
vectors
vec<int>
vec<str>
vec<struct>
```

---

## 5. Common CTest Commands

Run all tests:

```bash
ctest --test-dir build
```

Run all Nova tool tests:

```bash
ctest --test-dir build -L nova_tool
```

Run Nova codegen tool tests:

```bash
ctest --test-dir build -R nova_tool_codegen
```

Run import tests:

```bash
ctest --test-dir build -R import
```

Run frontend tests:

```bash
ctest --test-dir build -R nova_tool_frontend
```

Run checker/typecheck tests:

```bash
ctest --test-dir build -R "nova_tool_(checker|typecheck)"
```

Run with failure details:

```bash
ctest --test-dir build --output-on-failure
```

---

## 6. Test Drivers

CTest helpers live in:

```text
cmake/
```

Important drivers:

```text
RunCodegenTest.cmake
RunNovaToolTest.cmake
RunNovaToolNegativeTest.cmake
RunNovaCodegenToolTest.cmake
RunTextCompareTest.cmake
```

---

### `RunNovaToolTest.cmake`

Used for positive Nova tool tests.

Typical flow:

```text
1. nova_compile tool.nv -> tool.c
2. cc tool.c runtime/nova_runtime.c -> tool executable
3. run tool executable on INPUT
4. compare output file with EXPECT
```

Supports optional:

```text
TOOL_ARGS
EXPECT_PREFIX
```

`TOOL_ARGS` is used for tools like:

```bash
nova_frontend check input.nv output.check
```

`EXPECT_PREFIX` is used for smoke tests where only the beginning of output matters.

---

### `RunNovaToolNegativeTest.cmake`

Used for negative Nova tool tests.

Typical flow:

```text
1. compile Nova tool
2. run it on invalid input
3. expect failure
4. compare stderr with .err file
```

Often uses:

```text
STRIP_RUNTIME_PREFIX=ON
```

to remove the outer `Nova runtime error:` prefix from expected diagnostics.

---

### `RunNovaCodegenToolTest.cmake`

Used for `tools/nova_codegen.nv`.

Flow:

```text
1. compile tools/nova_codegen.nv -> nova_codegen.c
2. compile nova_codegen.c -> nova_codegen executable
3. run nova_codegen input.nv generated.c
4. compile generated.c -> generated executable
5. run generated executable
6. compare stdout with expected .out
```

This validates the full Nova-written codegen pipeline.

---

## 7. Adding a C++ Codegen Test

Add files under:

```text
tests/codegen/positive/
```

Example:

```text
tests/codegen/positive/example.nv
tests/codegen/positive/example.out
```

Then add the test in `CMakeLists.txt` using the existing codegen helper.

The `.out` file should contain the exact expected stdout.

---

## 8. Adding a Nova Tool Codegen Test

Add files under:

```text
tests/tools/codegen/
```

Example:

```text
tests/tools/codegen/example.nv
tests/tools/codegen/example.out
```

Then add:

```cmake
add_nova_codegen_tool_test(example)
```

to `CMakeLists.txt`.

The test will:

```text
compile tools/nova_codegen.nv
run it on example.nv
compile generated C
run generated executable
compare stdout with example.out
```

---

## 9. Adding a Frontend Test

For token mode:

```text
tests/tools/frontend/tokens/name.nv
tests/tools/frontend/tokens/name.tok
```

Add:

```cmake
add_nova_frontend_test(tokens name tok)
```

For parse mode:

```text
tests/tools/frontend/parse/name.nv
tests/tools/frontend/parse/name.out
```

Add:

```cmake
add_nova_frontend_test(parse name out)
```

For check mode:

```text
tests/tools/frontend/check/name.nv
tests/tools/frontend/check/name.check
```

Add:

```cmake
add_nova_frontend_test(check name check)
```

For negative tests:

```text
tests/tools/frontend/negative/<mode>/name.nv
tests/tools/frontend/negative/<mode>/name.err
```

Add:

```cmake
add_nova_frontend_negative_test(<mode> name)
```

---

## 10. Adding an Import Test

Positive import tests live under:

```text
tests/import/positive/<name>/
```

A typical layout:

```text
main.nv
main.out
helper.nv
```

Negative import tests live under:

```text
tests/import/negative/<name>/
```

A typical layout:

```text
main.nv
main.err
```

Important import cases:

```text
missing imported file
cyclic import
diamond import
duplicate include
relative path resolution
```

---

## 11. Golden Files

Many tests compare against golden output files:

```text
*.out
*.err
*.tok
*.check
```

These files are source-controlled and should not be ignored by `.gitignore`.

Do not add these patterns to `.gitignore`:

```text
*.out
*.err
*.tok
*.check
```

They are part of the test suite.

---

## 12. Debugging Failed Tests

Run a failing test with verbose output:

```bash
ctest --test-dir build -R test_name --output-on-failure
```

Inspect the generated work directory. Nova tool tests usually write temporary outputs under:

```text
build/nova_tool_tests/
```

For codegen tests, inspect generated C:

```text
build/nova_tool_tests/codegen_<name>/
```

Common files include:

```text
nova_codegen.c
nova_codegen_exe
<test>.generated.c
<test>_generated_exe
```

If generated C fails to compile, the test driver prints the generated C source.

---

## 13. Expected Failure Style

Negative tests should prefer stable diagnostics.

Examples:

```text
ParseError at 2:19: expected expression
CheckerError at 2:15: undefined variable 'x'
CodegenError: unsupported type in tiny codegen: Point
ImportError: cannot open import 'missing.nv'
```

When running through generated Nova tools, runtime errors may be wrapped as:

```text
Nova runtime error: ...
```

Most negative test drivers can strip this with:

```cmake
-DSTRIP_RUNTIME_PREFIX=ON
```

---

## 14. Recommended Final Sweep

Before marking a milestone complete, run:

```bash
ctest --test-dir build
```

Then run targeted groups:

```bash
ctest --test-dir build -L nova_tool
ctest --test-dir build -R nova_tool_codegen
ctest --test-dir build -R import
```

All should pass before the Phase 1 bootstrap milestone is considered complete.
