# Nova

Nova is a small educational programming language and compiler project.

## Current milestone

Phase 2 self-hosting milestone:

- The C++ seed compiler builds Nova programs and Nova-written tools.
- Nova-written frontend tools can tokenize, parse, and check Nova source.
- Nova-written codegen can generate C for a useful subset of Nova.
- Stage0, stage1, and stage2 `nova_codegen` participate in a repeatable self-hosting workflow.
- Nova-written `nova_compile.nv` provides a compile-to-C driver.
- Nova-written `nova_build.nv` provides a prototype build driver.
- Import-based Nova libraries, source-aware diagnostics, and runtime/builtin APIs are documented.
- VS Code syntax tooling is available under `tools/vscode-nova-syntax`.

Nova remains an educational/prototype compiler. The C++ compiler is still the seed compiler.

## Build

From the repository root:

```bash
cmake -S . -B build
cmake --build build --parallel
```

## Test

```bash
ctest --test-dir build -j8 --output-on-failure
```

## Self-hosting workflow

```bash
scripts/self_host.sh
```

This builds stage0, stage1, and stage2 versions of `nova_codegen`, checks representative generated programs, and runs a `nova_compile_stage1` smoke test.

## Run the C++ seed compiler

```bash
./build/nova_compile examples/positive/helloworld.nv /tmp/hello.c
cc /tmp/hello.c runtime/nova_runtime.c -I runtime -o /tmp/hello
/tmp/hello
```

## Run the Nova-written frontend

```bash
./build/nova_compile tools/nova_frontend.nv /tmp/nova_frontend.c
cc /tmp/nova_frontend.c runtime/nova_runtime.c -I runtime -o /tmp/nova_frontend
/tmp/nova_frontend check tools/nova_checker.nv /tmp/check.out
cat /tmp/check.out
```

Expected output begins with:

```text
Check OK
```

## Run the Nova-written codegen tool

```bash
./build/nova_compile tools/nova_codegen.nv /tmp/nova_codegen.c
cc /tmp/nova_codegen.c runtime/nova_runtime.c -I runtime -o /tmp/nova_codegen
/tmp/nova_codegen tests/tools/codegen/positive/hello.nv /tmp/hello.c
cc /tmp/hello.c runtime/nova_runtime.c -I runtime -o /tmp/hello
/tmp/hello
```

Expected output:

```text
hello
```

## Run the Nova-written compile driver

```bash
./build/nova_compile tools/nova_compile.nv /tmp/nova_compile.c
cc /tmp/nova_compile.c runtime/nova_runtime.c -I runtime -o /tmp/nova_compile
/tmp/nova_compile tests/tools/compile/positive/hello.nv /tmp/hello.c
cc /tmp/hello.c runtime/nova_runtime.c -I runtime -o /tmp/hello
/tmp/hello
```

Expected output:

```text
hello
```

## Run the Nova build driver prototype

```bash
./build/nova_compile tools/nova_build.nv /tmp/nova_build.c
cc /tmp/nova_build.c runtime/nova_runtime.c -I runtime -o /tmp/nova_build
/tmp/nova_build compile tests/tools/compile/positive/hello.nv /tmp/hello_from_build.c
cc /tmp/hello_from_build.c runtime/nova_runtime.c -I runtime -o /tmp/hello_from_build
/tmp/hello_from_build
```

Expected output:

```text
hello
```

Optional full self-host command:

```bash
/tmp/nova_build self-host
```

## Documentation

- [Architecture](docs/architecture.md)
- [Bootstrap](docs/bootstrap.md)
- [Self-hosting](docs/self_hosting.md)
- [Tools](docs/tools.md)
- [Standard Library and Builtins](docs/standard_library.md)
- [Testing](docs/testing.md)
- [Limitations](docs/limitations.md)
- [Phase 2 Status](docs/phase2_status.md)
- [Initial language spec](docs/language_spec_v0.md)

## Status

Nova has a validated Phase 2 self-hosting codegen workflow, but it is not a production compiler. The C++ compiler remains the seed compiler, the import system is still textual include-style expansion, and the Nova build driver is a prototype.
