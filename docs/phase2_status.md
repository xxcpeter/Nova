# Nova Phase 2 Status

## Summary

Phase 2 establishes Nova's self-hosting frontend/codegen path and supporting developer tooling.

Nova now has:

```text
C++ seed compiler
Nova-written frontend libraries
Nova-written C codegen prototype
Nova-written compile driver
Nova build driver prototype
source-aware diagnostics
stable import/include behavior
runtime/builtin documentation
self-hosting codegen workflow
```

The project remains educational/prototype, but Phase 2 is a validated self-hosting milestone.

---

## Completed

- Nova-written tokenizer.
- Nova-written parser prototype.
- Nova-written declaration checker.
- Nova-written expression/type checker.
- Nova-written frontend driver.
- Nova-written C codegen prototype.
- Import-based library split.
- Source-aware diagnostics.
- Import system v2 cleanup and tests.
- Runtime/builtin/stdlib cleanup.
- `str_vec_*` legacy removal.
- `str + str` support.
- `str_ends_with`.
- Runtime process/filesystem APIs.
- Self-hosting codegen parity workflow.
- Stage0/stage1/stage2 `nova_codegen`.
- Nova-written `nova_compile.nv`.
- Nova-written `nova_build.nv` prototype.
- CLI regression tests.
- Golden/regression test cleanup.
- VS Code syntax tooling.
- Standard library documentation.
- Tools documentation.

---

## Validation

Primary validation commands:

```bash
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build -j8 --output-on-failure
scripts/self_host.sh
```

Additional smoke commands:

```bash
./build/nova_compile tools/nova_compile.nv /tmp/nova_compile.c
cc /tmp/nova_compile.c runtime/nova_runtime.c -I runtime -o /tmp/nova_compile
/tmp/nova_compile tests/tools/compile/positive/hello.nv /tmp/hello.c
cc /tmp/hello.c runtime/nova_runtime.c -I runtime -o /tmp/hello
/tmp/hello
```

```bash
./build/nova_compile tools/nova_build.nv /tmp/nova_build.c
cc /tmp/nova_build.c runtime/nova_runtime.c -I runtime -o /tmp/nova_build
/tmp/nova_build compile tests/tools/compile/positive/hello.nv /tmp/hello_build.c
cc /tmp/hello_build.c runtime/nova_runtime.c -I runtime -o /tmp/hello_build
/tmp/hello_build
```

---

## Stable Enough for Phase 2

These components are stable enough for continued Phase 2/Phase 3 development:

```text
C++ seed compiler
Nova-written tokenizer/parser/checker/frontend/codegen toolchain
compile-to-C workflow
source-level import flattening
source-aware diagnostics v1
runtime builtin surface
CTest regression suite
self-host script
VS Code syntax extension
```

---

## Prototype / Limitations

These areas remain prototype or limited:

```text
Nova build driver delegates to shell commands.
run_command is shell-based and trusted-input only.
Import system is source flattening, not a full module namespace.
Nova codegen is a C backend prototype.
Diagnostics do not have rich code frames.
No package manager.
No LSP.
No formatter.
No optimizer or IR layer.
C++ compiler remains the seed compiler.
```

---

## What Phase 2 Proves

Phase 2 proves:

```text
1. C++ seed compiler can build Nova-written compiler tools.
2. Nova-written codegen can regenerate itself through stage0/stage1/stage2.
3. Generated tools can compile representative Nova programs.
4. Nova-written compile driver can compile Nova source to C.
5. Source-aware diagnostics and import expansion are reliable enough for multi-file tools.
6. Runtime/builtin APIs are documented and aligned across C++ sema, Nova checker, and Nova codegen.
```

---

## What Phase 2 Does Not Prove

Phase 2 does not prove:

```text
complete self-hosting without C++ seed compiler
native code generation
full module system
package management
production diagnostics
complete language server support
complete standard library
```