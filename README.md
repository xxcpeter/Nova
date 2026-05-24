# Nova

Nova is a small educational programming language and compiler project.

## Current milestone

Phase 1 bootstrap prototype:

- C++ compiler compiles Nova tools.
- Nova frontend tokenizes, parses, and checks Nova source.
- Nova codegen generates C for a useful subset.
- Import-based Nova libraries are supported.

## Build

From the repository root:

```bash
cmake -S . -B build
cmake --build build --parallel
```

## Test

```bash
ctest --test-dir build --output-on-failure
```

## Run the C++ compiler

```bash
./build/nova_compile examples/positive/helloworld.nv /tmp/hello.c
cc /tmp/hello.c runtime/nova_runtime.c -I runtime -o /tmp/hello
/tmp/hello
```

## Run the Nova frontend

```bash
./build/nova_compile tools/nova_frontend.nv /tmp/nova_frontend.c
cc /tmp/nova_frontend.c runtime/nova_runtime.c -I runtime -o /tmp/nova_frontend
/tmp/nova_frontend check tools/nova_checker.nv /tmp/check.out
cat /tmp/check.out
```

## Run the Nova codegen prototype

```bash
./build/nova_compile tools/nova_codegen.nv /tmp/nova_codegen.c
cc /tmp/nova_codegen.c runtime/nova_runtime.c -I runtime -o /tmp/nova_codegen
/tmp/nova_codegen tests/tools/codegen/hello.nv /tmp/hello.c
cc /tmp/hello.c runtime/nova_runtime.c -I runtime -o /tmp/hello
/tmp/hello
```

## Documentation

* [Architecture](docs/architecture.md)
* [Bootstrap demo](docs/bootstrap.md)
* [Testing guide](docs/testing.md)
* [Limitations](docs/limitations.md)
* [Initial language spec](docs/language_spec_v0.md)

## Status

Nova is not yet fully self-hosting. The C++ compiler remains the primary compiler, while the Nova-written frontend and codegen prototype demonstrate the Phase 1 bootstrap foundation.
