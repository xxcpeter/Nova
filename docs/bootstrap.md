# Nova Bootstrap Demo

This document shows the current Phase 1 bootstrap milestone.

The goal is to demonstrate that:

```text
C++ compiler compiles Nova-written tools.
Nova frontend checks Nova source.
Nova codegen generates C.
Generated C compiles and runs.
```

---

## 1. Build the Project

From the repository root:

```bash
cmake -S . -B build
cmake --build build
```

Optional full test sweep:

```bash
ctest --test-dir build
```

---

## 2. Demo 1: Compile and Run a Nova Program with the C++ Compiler

Compile a simple Nova program:

```bash
./build/nova_compile examples/positive/helloworld.nv /tmp/nova_hello.c
```

Compile the generated C:

```bash
cc /tmp/nova_hello.c runtime/nova_runtime.c -I runtime -o /tmp/nova_hello
```

Run it:

```bash
/tmp/nova_hello
```

Expected result: the program prints the hello-world output from `examples/positive/helloworld.nv`.

This demonstrates:

```text
Nova source
  -> C++ compiler
  -> generated C
  -> native executable
```

---

## 3. Demo 2: Compile the Nova Frontend Tool

Compile the Nova-written frontend using the C++ compiler:

```bash
./build/nova_compile tools/nova_frontend.nv /tmp/nova_frontend.c
```

Compile the generated C frontend:

```bash
cc /tmp/nova_frontend.c runtime/nova_runtime.c -I runtime -o /tmp/nova_frontend
```

Run frontend check mode on a Nova tool:

```bash
/tmp/nova_frontend check tools/nova_checker.nv /tmp/nova_checker.check
cat /tmp/nova_checker.check
```

Expected output begins with:

```text
Check OK
```

This demonstrates:

```text
C++ compiler compiles a Nova-written compiler tool.
The generated Nova frontend can check Nova source code.
```

---

## 4. Demo 3: Use the Nova Frontend Modes

After compiling `/tmp/nova_frontend`, run token mode:

```bash
/tmp/nova_frontend tokens tests/tools/frontend/tokens/basic.nv /tmp/basic.tok
cat /tmp/basic.tok
```

Run parse mode:

```bash
/tmp/nova_frontend parse tests/tools/frontend/parse/expression.nv /tmp/expression.out
cat /tmp/expression.out
```

Run check mode:

```bash
/tmp/nova_frontend check tests/tools/frontend/check/let_return_int.nv /tmp/let_return_int.check
cat /tmp/let_return_int.check
```

This demonstrates the Nova-written frontend pipeline:

```text
source
  -> load_source_with_imports
  -> tokenize
  -> parse_program_tree
  -> check_program
```

---

## 5. Demo 4: Compile and Run the Nova Codegen Tool

Compile the Nova-written codegen tool using the C++ compiler:

```bash
./build/nova_compile tools/nova_codegen.nv /tmp/nova_codegen.c
```

Compile the generated C codegen tool:

```bash
cc /tmp/nova_codegen.c runtime/nova_runtime.c -I runtime -o /tmp/nova_codegen
```

Use the Nova-written codegen tool to generate C:

```bash
/tmp/nova_codegen tests/tools/codegen/hello.nv /tmp/generated_hello.c
```

Compile the generated C program:

```bash
cc /tmp/generated_hello.c runtime/nova_runtime.c -I runtime -o /tmp/generated_hello
```

Run it:

```bash
/tmp/generated_hello
```

Expected output:

```text
hello
```

This demonstrates:

```text
C++ compiler
  -> compiles Nova-written codegen tool

Nova-written codegen tool
  -> compiles Nova source into C

C compiler
  -> compiles generated C into executable
```

---

## 6. Demo 5: Nova Codegen with a Larger Example

Generate C from a codegen test with structs, vectors, or recursion:

```bash
/tmp/nova_codegen tests/tools/codegen/vec_struct_basic.nv /tmp/vec_struct_basic.c
```

Compile it:

```bash
cc /tmp/vec_struct_basic.c runtime/nova_runtime.c -I runtime -o /tmp/vec_struct_basic
```

Run it:

```bash
/tmp/vec_struct_basic
```

Expected output should match:

```text
tests/tools/codegen/vec_struct_basic.out
```

You can compare manually:

```bash
/tmp/vec_struct_basic > /tmp/vec_struct_basic.actual
diff -u tests/tools/codegen/vec_struct_basic.out /tmp/vec_struct_basic.actual
```

---

## 7. Import-Based Library Demo

Nova tools use import-based libraries.

For example, `tools/nova_frontend.nv` imports reusable libraries from `lib/`:

```nova
import "../lib/source_loader.nv";
import "../lib/tokenizer.nv";
import "../lib/parser.nv";
import "../lib/checker.nv";
```

The C++ compiler expands these imports before lexing and parsing:

```bash
./build/nova_compile tools/nova_frontend.nv /tmp/nova_frontend.c
```

This demonstrates the current source-level import/include system.

---

## 8. One-Command Style Demo

A compact Phase 1 demo:

```bash
cmake -S . -B build
cmake --build build

./build/nova_compile tools/nova_frontend.nv /tmp/nova_frontend.c
cc /tmp/nova_frontend.c runtime/nova_runtime.c -I runtime -o /tmp/nova_frontend
/tmp/nova_frontend check tools/nova_checker.nv /tmp/nova_checker.check
cat /tmp/nova_checker.check

./build/nova_compile tools/nova_codegen.nv /tmp/nova_codegen.c
cc /tmp/nova_codegen.c runtime/nova_runtime.c -I runtime -o /tmp/nova_codegen
/tmp/nova_codegen tests/tools/codegen/hello.nv /tmp/generated_hello.c
cc /tmp/generated_hello.c runtime/nova_runtime.c -I runtime -o /tmp/generated_hello
/tmp/generated_hello
```

Expected highlights:

```text
Check OK
hello
```

---

## 9. What This Milestone Proves

The Phase 1 bootstrap prototype proves:

```text
1. The C++ compiler can compile import-based Nova tools.
2. Nova-written libraries can implement tokenizer, parser, checker, and codegen pieces.
3. The Nova frontend can check Nova source code.
4. The Nova codegen prototype can generate C.
5. Generated C can be compiled and executed.
6. The full system is covered by CTest.
```

This is not yet full self-hosting, but it is a working bootstrap foundation.
