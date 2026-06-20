# Nova Bootstrap

This document explains Nova's current bootstrap and self-hosting workflow.

Nova has a C++ seed compiler and several Nova-written compiler tools. Phase 2 demonstrates that the Nova-written codegen can regenerate itself through stage0/stage1/stage2 and that generated tools can compile representative Nova programs.

---

## 1. Build the Project

From the repository root:

```bash
cmake -S . -B build
cmake --build build --parallel
```

Run the full test suite:

```bash
ctest --test-dir build -j8 --output-on-failure
```

---

## 2. C++ Seed Compiler Demo

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

This demonstrates:

```text
Nova source
  -> C++ seed compiler
  -> generated C
  -> native executable
```

---

## 3. Compile the Nova Frontend Tool

Compile the Nova-written frontend with the C++ seed compiler:

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

---

## 4. Nova Frontend Modes

Token mode:

```bash
/tmp/nova_frontend tokens tests/tools/frontend/positive/tokens/basic.nv /tmp/basic.tok
cat /tmp/basic.tok
```

Parse mode:

```bash
/tmp/nova_frontend parse tests/tools/frontend/positive/parse/expression.nv /tmp/expression.out
cat /tmp/expression.out
```

Check mode:

```bash
/tmp/nova_frontend check tests/tools/frontend/positive/check/let_return_int.nv /tmp/let_return_int.check
cat /tmp/let_return_int.check
```

The frontend pipeline is:

```text
source
  -> load_source_with_imports
  -> tokenize_loaded
  -> parse_program_tree
  -> check_program
```

---

## 5. Nova-written Codegen Tool

Compile the Nova-written codegen tool:

```bash
./build/nova_compile tools/nova_codegen.nv /tmp/nova_codegen.c
```

Compile the generated codegen executable:

```bash
cc /tmp/nova_codegen.c runtime/nova_runtime.c -I runtime -o /tmp/nova_codegen
```

Use it to generate C:

```bash
/tmp/nova_codegen tests/tools/codegen/positive/hello.nv /tmp/generated_hello.c
```

Compile and run the generated program:

```bash
cc /tmp/generated_hello.c runtime/nova_runtime.c -I runtime -o /tmp/generated_hello
/tmp/generated_hello
```

Expected output:

```text
hello
```

---

## 6. Nova-written Compile Driver

Compile the Nova-written compile-to-C driver:

```bash
./build/nova_compile tools/nova_compile.nv /tmp/nova_compile.c
```

Compile the generated driver:

```bash
cc /tmp/nova_compile.c runtime/nova_runtime.c -I runtime -o /tmp/nova_compile
```

Use it to compile a Nova program:

```bash
/tmp/nova_compile tests/tools/compile/positive/hello.nv /tmp/hello_by_nova_compile.c
```

Compile and run the generated C:

```bash
cc /tmp/hello_by_nova_compile.c runtime/nova_runtime.c -I runtime -o /tmp/hello_by_nova_compile
/tmp/hello_by_nova_compile
```

Expected output:

```text
hello
```

This demonstrates:

```text
C++ seed compiler
  -> compiles Nova-written compile driver

Nova-written compile driver
  -> compiles Nova source to C
```

---

## 7. Self-hosted Codegen Workflow

Run:

```bash
scripts/self_host.sh
```

This script builds:

```text
C++ seed compiler
  -> nova_codegen_stage0

nova_codegen_stage0
  -> nova_codegen_stage1

nova_codegen_stage1
  -> nova_codegen_stage2
```

Then it uses stage1 and stage2 codegen to compile representative Nova programs and compares their outputs.

The script also builds:

```text
nova_compile_stage1
```

and uses it to compile a representative Nova program.

---

## 8. Bootstrap Diagram

```text
C++ seed compiler
  └── builds tools/nova_codegen.nv
        └── nova_codegen_stage0
              └── builds tools/nova_codegen.nv
                    └── nova_codegen_stage1
                          └── builds tools/nova_codegen.nv
                                └── nova_codegen_stage2

C++ seed compiler or stage0 codegen
  └── builds tools/nova_compile.nv
        └── nova_compile_stage1
              └── compiles Nova source to C
```

---

## 9. Nova Build Driver Prototype

Compile the Nova build driver prototype:

```bash
./build/nova_compile tools/nova_build.nv /tmp/nova_build.c
cc /tmp/nova_build.c runtime/nova_runtime.c -I runtime -o /tmp/nova_build
```

Compile a Nova program through the build driver:

```bash
/tmp/nova_build compile tests/tools/compile/positive/hello.nv /tmp/hello_by_nova_build.c
cc /tmp/hello_by_nova_build.c runtime/nova_runtime.c -I runtime -o /tmp/hello_by_nova_build
/tmp/hello_by_nova_build
```

Expected output:

```text
hello
```

Optional full workflow:

```bash
/tmp/nova_build self-host
```

---

## 10. What Phase 2 Proves

Phase 2 proves:

```text
1. The C++ seed compiler can compile import-based Nova tools.
2. Nova-written libraries implement tokenizer, parser, checker, and codegen pieces.
3. The Nova frontend can check Nova source code.
4. The Nova codegen prototype can generate C.
5. The Nova codegen can regenerate itself across stage0/stage1/stage2.
6. The Nova-written compile driver can compile Nova source to C.
7. Runtime/builtin APIs and import behavior are documented and tested.
8. The full system is covered by CTest and self-host validation.
```

This is not full production self-hosting, but it is a validated educational self-hosting milestone.
