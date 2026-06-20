# Nova Limitations

This document records known limitations of the current Phase 2 self-hosting milestone.

Nova is a small educational language and compiler project. The current implementation is useful and well-tested, but it is still a prototype.

---

## Language

- No generic functions beyond builtin `vec<T>`.
- No interfaces, traits, typeclasses, or protocols.
- No closures or lambdas.
- No methods or associated functions.
- No pattern matching.
- No exceptions.
- No ownership, borrowing, or lifetime system.
- Control flow is intentionally small.

---

## Import System

- `import` is a textual include mechanism, not a real module system.
- Imported files are expanded into one flattened program.
- All declarations share one global namespace after expansion.
- There is no `export`, `private`, aliasing, or package search path.
- Include-once behavior prevents duplicate inclusion, but it does not provide module visibility or namespacing.
- There is no separate compilation or compiled module cache.

---

## Diagnostics

- Diagnostics are source-aware and generally report `file:line:column`.
- Imported source diagnostics can point back to the original imported file.
- There are no rich code frames yet.
- There are no multi-span diagnostics.
- There are no warnings or fix-it suggestions.
- Diagnostics are not yet integrated through a language server.

---

## Nova Codegen

- The Nova-written codegen is a prototype C backend.
- It is not a complete replacement for the C++ codegen backend.
- It supports the tested subset used by current Nova tools and representative programs.
- Generated C is intended to be readable and testable, not optimized.
- No optimizer or IR layer exists yet.
- Generated C is the execution path; there is no native backend.

---

## Self-hosting

- Nova has a repeatable self-hosted codegen workflow through `scripts/self_host.sh`.
- The workflow builds stage0, stage1, and stage2 versions of `nova_codegen`.
- Stage1/stage2 behavior is validated on representative programs.
- The C++ seed compiler is still required to produce stage0.
- This is not full production self-hosting.

---

## Nova Compile and Build Drivers

- `tools/nova_compile.nv` compiles Nova source to C.
- It does not invoke the system C compiler.
- `tools/nova_build.nv` is a prototype build driver.
- `nova_build` can delegate to shell commands and the self-host script.
- It is not a package manager, dependency graph engine, or incremental build system.
- It does not provide parallel build scheduling.

---

## Runtime

- The runtime is a small C runtime.
- Most allocations are intentionally simple and generally rely on process exit for cleanup.
- Generated vector helpers currently do not free allocated memory.
- Buffers are integer handles to runtime-managed string builders.
- `run_command` executes through the host shell and is intended only for trusted build tooling.
- `make_dir` is not recursive.
- `remove_file` removes files, not directories.
- Filesystem and process APIs assume a POSIX-like environment.

---

## Vectors

- Nested `vec` types are not supported.
- Vector helper functions are generated per element type.
- Generated vector memory is not freed explicitly.
- Vector helper C names are implementation details.

---

## Tooling

- The VS Code extension provides syntax highlighting, snippets, basic tasks, and problem matcher support.
- There is no language server yet.
- There is no semantic hover, go-to-definition, rename, references, formatting, or live diagnostics.
- Formatter and LSP work are Phase 3 candidates.

---

## Assignments

- Historical assignments record the development process.
- Some early goals were revised, merged, or replaced in later steps.
- The final implementation may differ from earlier step goals.
- The current project behavior is defined by the source tree, tests, and current documentation.
