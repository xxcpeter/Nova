# Nova Architecture

Nova is a small educational programming language and compiler project. The current state is a Phase 2 self-hosting milestone: the main seed compiler is still implemented in C++, while a growing frontend/codegen toolchain is implemented in Nova itself.

---

## Overview

Nova currently has two implementation layers:

```text
C++ seed compiler
  source -> import expansion -> lexer -> parser -> sema -> C codegen

Nova-written toolchain
  source -> import expansion -> tokenizer -> parser -> checker -> C codegen
```

The C++ compiler remains the seed compiler. The Nova-written tools demonstrate that Nova can implement meaningful parts of its own compiler pipeline and participate in a repeatable self-hosting workflow.

---

## Directory Layout

```text
include/   C++ compiler headers
src/       C++ compiler implementation and command-line tools
runtime/   C runtime used by generated programs
lib/       Nova-written reusable compiler libraries
tools/     Nova-written command-line tools and VS Code extension
tests/     C++ compiler tests and Nova tool tests
cmake/     CTest helper scripts
scripts/   build, regression, and self-host scripts
docs/      project documentation and historical assignments
examples/  small Nova example programs
```

---

## C++ Seed Compiler

The C++ compiler pipeline is:

```text
input.nv
  -> load_source_with_imports
  -> Lexer
  -> Parser
  -> SemanticAnalyzer
  -> CCodegen
  -> output.c
```

The main command is:

```bash
./build/nova_compile input.nv output.c
```

The generated C is compiled with the runtime:

```bash
cc output.c runtime/nova_runtime.c -I runtime -o program
```

Additional C++ tools expose individual phases:

```text
nova_lex
nova_parse
nova_sema
nova_compile
```

All modern C++ entry points use the shared source loader and support `import "path.nv";`.

---

## Import System

Nova currently supports a source-level include system:

```nova
import "relative/path.nv";
```

Imports are resolved relative to the importing file, path-normalized, expanded before lexing, included once, and checked for cycles.

This is not a full module system. There are no namespaces, exports, aliases, package search paths, or separate compilation. After import expansion, declarations share one global namespace.

---

## Source-aware Diagnostics

Tokens and parse nodes now carry source file information. Diagnostics generally use:

```text
file:line:column: ErrorKind: message
```

This applies across imported source as well as root files. The system does not yet provide rich code frames, multi-span diagnostics, warnings, or fix-it suggestions.

---

## Nova Libraries

Reusable Nova compiler code lives in `lib/`:

```text
lib/token.nv          TokenKind and Token
lib/tokenizer.nv      tokenizer and token dumping
lib/parse_tree.nv     ParseNodeKind and ParseNode
lib/parser.nv         parse_program_tree
lib/checker.nv        declaration and expression checker
lib/codegen_c.nv      prototype C codegen
lib/source_loader.nv  Nova-side import expansion
lib/diagnostics.nv    diagnostic helpers
```

These files are imported by Nova tools, for example:

```nova
import "../lib/source_loader.nv";
import "../lib/tokenizer.nv";
import "../lib/parser.nv";
import "../lib/checker.nv";
```

---

## Nova-written Tools

Main Nova-written tools:

```text
tools/nova_tokenizer.nv
tools/nova_parser.nv
tools/nova_checker.nv
tools/nova_frontend.nv
tools/nova_codegen.nv
tools/nova_compile.nv
tools/nova_build.nv
```

### Frontend

`tools/nova_frontend.nv` supports:

```bash
nova_frontend tokens input.nv output.tok
nova_frontend parse  input.nv output.out
nova_frontend check  input.nv output.check
```

### Codegen

`tools/nova_codegen.nv` is a backend/codegen-oriented regression tool. It reads Nova source and writes C.

### Compile driver

`tools/nova_compile.nv` is the Nova-written compile-to-C driver. It represents the user-facing Nova-written compiler pipeline:

```text
source -> imports -> tokenize -> parse -> check -> gen C -> write output.c
```

### Build driver prototype

`tools/nova_build.nv` is a prototype build driver. It uses runtime process/filesystem APIs and can delegate to the self-host workflow.

---

## Self-hosting Workflow

The canonical self-hosting script is:

```bash
scripts/self_host.sh
```

It builds:

```text
C++ seed compiler -> nova_codegen_stage0
nova_codegen_stage0 -> nova_codegen_stage1
nova_codegen_stage1 -> nova_codegen_stage2
```

It then uses stage1 and stage2 codegen to compile representative programs and compare behavior. It also builds a `nova_compile_stage1` smoke driver.

---

## Runtime

Generated C programs link against:

```text
runtime/nova_runtime.c
runtime/nova_runtime.h
```

The runtime provides:

```text
printing
strings
file I/O
buffers
command-line arguments
runtime errors
process command execution
basic filesystem helpers
```

Typed vector operations are compiler-known builtins and are generated as per-type C helpers.

---

## VS Code Tooling

A lightweight VS Code extension lives in:

```text
tools/vscode-nova-syntax/
```

It provides `.nv` file association, TextMate syntax highlighting, snippets, tasks/problem matcher examples, and editor configuration. It is not a full language server.

---

## Testing

Nova uses CTest. Test groups include:

```text
tests/lexer/              C++ lexer tests
tests/parser/             C++ parser tests
tests/sema/               C++ semantic tests
tests/codegen/            C++ codegen tests
tests/import/             import/source loader tests
tests/tools/              Nova-written tool tests
scripts/self_host.sh      self-hosting workflow
```

Detailed test instructions live in `docs/testing.md`.

---

## Current Milestone

The Phase 2 milestone demonstrates:

```text
C++ seed compiler compiles Nova-written tools.
Nova-written frontend checks Nova source.
Nova-written codegen generates C.
Nova-written compile driver compiles Nova source to C.
Stage0/stage1/stage2 codegen workflow is repeatable.
Runtime/builtin surface is documented.
CTest and self-host validation pass.
```

---

## Known Boundaries

Important limitations are documented in `docs/limitations.md`. The short version:

```text
import is textual include, not a true module system
diagnostics are source-aware but not rich code-frame diagnostics
Nova codegen is a prototype C backend
nested vec is not supported
generated vector memory is not freed
run_command is shell-based and intended for trusted tooling
VS Code support is lightweight, not LSP
```
