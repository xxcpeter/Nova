# Nova Tools

This document describes the current Nova toolchain.

Nova has two layers of tools:

```text
C++ seed tools
Nova-written tools
```

The C++ tools are built directly by CMake. The Nova-written tools are compiled by the C++ seed compiler or by stage-generated Nova codegen during self-hosting.

---

## 1. C++ Seed Tools

### `build/nova_lex`

Lexes a Nova source file and prints tokens.

```bash
./build/nova_lex input.nv
```

### `build/nova_parse`

Parses a Nova source file and prints the parse tree / AST dump.

```bash
./build/nova_parse input.nv
```

### `build/nova_sema`

Runs semantic analysis on a Nova source file.

```bash
./build/nova_sema input.nv
```

### `build/nova_compile`

Compiles Nova source to C.

```bash
./build/nova_compile input.nv output.c
```

Then compile the generated C:

```bash
cc output.c runtime/nova_runtime.c -I runtime -o program
```

---

## 2. Nova-written Tools

The examples below assume each tool has been compiled to an executable.

### `tools/nova_tokenizer.nv`

Tokenizes Nova source.

Usage:

```text
Usage: nova_tokenizer <input.nv> <output.tok>
```

### `tools/nova_parser.nv`

Parses Nova source and writes parse output.

Usage:

```text
Usage: nova_parser <input.nv> <output.out>
```

### `tools/nova_checker.nv`

Checks Nova source and writes checker output.

Usage:

```text
Usage: nova_checker <input.nv> <output.check>
```

### `tools/nova_frontend.nv`

Integrated frontend driver.

Usage:

```text
Usage: nova_frontend <tokens|parse|check> <input.nv> <output>
```

Modes:

```text
tokens
parse
check
```

### `tools/nova_codegen.nv`

Backend/codegen regression tool. It compiles Nova source to C.

Usage:

```text
Usage: nova_codegen <input.nv> <output.c>
```

This tool remains useful for codegen-focused tests.

### `tools/nova_compile.nv`

Official Nova-written compile-to-C driver.

Usage:

```text
Usage: nova_compile <input.nv> <output.c>
```

This is the Nova-written compiler pipeline entry point:

```text
source -> imports -> tokenize -> parse -> check -> gen C
```

### `tools/nova_build.nv`

Prototype Nova-written build driver.

Usage:

```text
Usage: nova_build <self-host|compile|test> [args...]
```

Current modes:

```text
self-host
compile <input.nv> <output.c>
test
```

`nova_build` is a prototype. It can delegate to shell commands through `run_command`, but it is not a package manager or full build system.

---

## 3. Seed vs Nova-written Compile Driver

There are two compile drivers:

```text
build/nova_compile
  C++ seed compiler

tools/nova_compile.nv
  Nova-written compile-to-C driver
```

When both are built locally, use names like:

```text
nova_compile_nova
nova_compile_stage1
```

for generated Nova-written compiler executables to avoid confusion with `build/nova_compile`.

---

## 4. Scripts

### `scripts/self_host.sh`

Runs the self-hosting codegen workflow.

```bash
scripts/self_host.sh
```

It builds stage0, stage1, and stage2 `nova_codegen`, compares representative generated program behavior, and runs a `nova_compile_stage1` smoke test.

---

## 5. Common Development Commands

Build:

```bash
cmake -S . -B build
cmake --build build --parallel
```

Test:

```bash
ctest --test-dir build -j8 --output-on-failure
```

Self-host:

```bash
scripts/self_host.sh
```

Compile the Nova-written compile driver:

```bash
./build/nova_compile tools/nova_compile.nv /tmp/nova_compile.c
cc /tmp/nova_compile.c runtime/nova_runtime.c -I runtime -o /tmp/nova_compile
```

Compile with the Nova-written compile driver:

```bash
/tmp/nova_compile tests/tools/compile/positive/hello.nv /tmp/hello.c
cc /tmp/hello.c runtime/nova_runtime.c -I runtime -o /tmp/hello
/tmp/hello
```
