# Nova Architecture

Nova is a small educational programming language and compiler project. The current project state is a Phase 1 bootstrap prototype: the main compiler is implemented in C++, while several compiler tools are now implemented in Nova itself.

---

## Overview

Nova currently has two implementation layers:

```text
C++ compiler
  source -> import expansion -> lexer -> parser -> sema -> C codegen

Nova tools
  source -> import expansion -> tokenizer -> parser -> checker -> optional C codegen
```

The C++ compiler is still the primary compiler. The Nova-written tools demonstrate that Nova can implement meaningful parts of its own frontend and a prototype backend.

---

## Directory Layout

```text
include/   C++ compiler headers
src/       C++ compiler implementation and command-line tools
runtime/   C runtime used by generated programs
lib/       Nova-written reusable compiler libraries
tools/     Nova-written command-line tools and editor support
tests/     C++ compiler tests and Nova tool tests
cmake/     CTest helper scripts
docs/      project documentation and historical assignments
examples/  small Nova example programs
```

---

## C++ Compiler

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

The generated C is compiled together with the runtime:

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

All modern C++ entry points use the shared source loader, so they support `import "path.nv";`.

---

## Import System

Nova currently supports a minimal source-level include system:

```nova
import "relative/path.nv";
```

Imports are resolved relative to the importing file, expanded before lexing, included once, and checked for cycles.

This is not a full module system. There are no namespaces, exports, aliases, package search paths, or separate compilation. After expansion, all declarations share one global namespace.

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
lib/diagnostics.nv    ParseError / CheckerError helpers
```

These files are imported by Nova tools, for example:

```nova
import "../lib/tokenizer.nv";
import "../lib/parser.nv";
import "../lib/checker.nv";
```

---

## Nova Frontend

The main Nova-written frontend driver is:

```text
tools/nova_frontend.nv
```

It supports three modes:

```bash
nova_frontend tokens input.nv output.tok
nova_frontend parse  input.nv output.out
nova_frontend check  input.nv output.check
```

The frontend pipeline is:

```text
source
  -> load_source_with_imports
  -> tokenize
  -> parse_program_tree
  -> check_program
```

---

## Nova Codegen Prototype

The Nova-written codegen tool is:

```text
tools/nova_codegen.nv
```

It uses:

```text
lib/codegen_c.nv
```

The pipeline is:

```text
source
  -> load_source_with_imports
  -> tokenize
  -> parse_program_tree
  -> check_program
  -> gen_c_program
  -> output.c
```

The prototype supports a useful subset of Nova, including functions, structs, enums, vectors, control flow, runtime calls, and basic expressions. It is not yet a complete replacement for the C++ codegen backend.

---

## Runtime

Generated C programs link against:

```text
runtime/nova_runtime.c
runtime/nova_runtime.h
```

The runtime provides printing, strings, buffers, file I/O, command-line arguments, and runtime error handling.

Generated C includes:

```c
#include "nova_runtime.h"
```

---

## Testing

Nova uses CTest. Tests are split broadly into:

```text
tests/lexer/      C++ lexer tests
tests/parser/     C++ parser tests
tests/sema/       C++ semantic tests
tests/codegen/    C++ codegen tests
tests/import/     import/source loader tests
tests/tools/      Nova-written tool tests
```

Detailed test instructions live in `docs/testing.md`.

---

## Current Milestone

The current Phase 1 milestone demonstrates:

```text
C++ compiler compiles Nova-written tools.
Nova frontend checks Nova source.
Nova codegen generates C.
Generated C compiles and runs.
Import-based Nova libraries work.
CTest passes.
```

Detailed bootstrap commands live in `docs/bootstrap.md`.

---

## Known Boundaries

Important limitations are documented in `docs/limitations.md`. The short version:

```text
import is textual include, not a true module system
diagnostics do not have source maps yet
Nova codegen is a prototype backend
nested vec is not supported
generated vector memory is not freed
VS Code support is syntax highlighting only
```
