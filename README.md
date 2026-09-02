# Nova

[![CI](https://github.com/xxcpeter/Nova/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/xxcpeter/Nova/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

Nova is a small statically typed programming language with a C backend and a reproducible self-hosting compiler workflow. It is designed to make compiler bootstrapping inspectable end to end: a C++ seed compiler builds compiler components written in Nova, and the Nova-written code generator then regenerates itself through multiple stages.

> [!NOTE]
> Nova has reached its validated Phase 2 self-hosting milestone. The C++ compiler is still required to produce stage0; Nova is an educational compiler project, not yet a production toolchain.

## Highlights

- A C++20 seed compiler with lexer, parser, semantic analysis, and C code generation.
- Nova-written tokenizer, parser, checker, frontend, C codegen, and compile/build drivers.
- Repeatable stage0 → stage1 → stage2 codegen regeneration and behavioral validation.
- Source-aware diagnostics and relative, cycle-checked source imports.
- Regression coverage across both the seed compiler and Nova-written tools.
- A small C runtime plus lightweight VS Code syntax tooling.

## Current status

Phase 2 validates the following bootstrap path:

```text
C++ seed compiler
  └─> Nova codegen (stage0)
        └─> Nova codegen (stage1)
              └─> Nova codegen (stage2)
```

Stage1 and stage2 compile representative Nova programs, and the workflow compares their observable behavior. A stage-generated Nova compile driver is also exercised as a smoke test. See [Phase 2 status](docs/phase2_status.md) for the complete scope and [known limitations](docs/limitations.md) for the current boundaries.

## Quick start

### Requirements

- CMake 3.16 or newer
- A C++20 compiler
- A C compiler available as `cc`
- Bash and a POSIX-like environment

Build the seed compiler and run the full regression suite:

```bash
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --parallel 2 --output-on-failure
```

Compile and run a Nova program:

```bash
./build/nova_compile examples/positive/helloworld.nv /tmp/hello.c
cc /tmp/hello.c runtime/nova_runtime.c -I runtime -o /tmp/hello
/tmp/hello
```

Expected output:

```text
hello world
```

## Language example

```nova
fn main() : void {
    let message : str = "hello from Nova";
    print_str(message);
    return;
}
```

Nova currently includes functions, explicit types, lexical scopes, conditionals, loops, structs, enums, strings, and typed vectors. Imports use relative source inclusion rather than a full module system.

## Architecture

| Layer | Implementation | Role |
| --- | --- | --- |
| Seed compiler | C++20 | Imports, lexing, parsing, semantic analysis, and C codegen |
| Self-hosted toolchain | Nova | Frontend libraries, checker, C codegen, and compile/build drivers |
| Runtime | C | Strings, buffers, file/process helpers, and generated program support |
| Validation | CTest and Bash | Golden tests, compiled-program tests, CLI tests, and staged regeneration |

The C++ seed compiler follows:

```text
source → import expansion → lexer → parser → semantic analysis → C codegen
```

The Nova-written pipeline follows:

```text
source → import expansion → tokenizer → parser → checker → C codegen
```

See [Architecture](docs/architecture.md) for a component-level tour.

## Self-hosting

Run the canonical workflow from the repository root:

```bash
scripts/self_host.sh
```

The script builds stage0, stage1, and stage2 versions of `nova_codegen`, compares stage1/stage2 behavior on representative programs, and runs a stage-generated `nova_compile` smoke test. A successful run ends with:

```text
[self-host] all checks passed
```

The workflow demonstrates codegen self-regeneration; it does not yet eliminate the C++ seed compiler. The exact guarantees are documented in [Self-hosting](docs/self_hosting.md).

## Project layout

```text
include/   C++ compiler headers
src/       C++ seed compiler and command-line tools
lib/       Reusable compiler libraries written in Nova
tools/     Nova-written tools and VS Code support
runtime/   C runtime linked with generated programs
tests/     Seed compiler and Nova tool regression tests
cmake/     CTest helper scripts
scripts/   Regression and self-hosting workflows
docs/      Architecture, testing, status, and design documentation
examples/  Small Nova programs
```

## Testing

Nova uses golden-file tests and compiled-program tests for:

- lexer, parser, and semantic analysis behavior;
- C++ and Nova-written C code generation;
- imports and source-aware diagnostics;
- Nova-written frontend, compile driver, and CLI behavior;
- the staged self-hosting workflow.

For targeted commands and guidance on adding regression cases, see the [Testing guide](docs/testing.md).

## Roadmap

- Expand the language subset handled by the Nova-written compiler path.
- Harden import/path handling, generated-code escaping, and runtime boundaries.
- Improve diagnostics with code frames and richer source context.
- Evolve textual imports toward a real module system.
- Improve runtime memory management and reduce bootstrap dependencies.
- Explore formatter and language-server support.

## Documentation

- [Architecture](docs/architecture.md)
- [Bootstrap](docs/bootstrap.md)
- [Self-hosting](docs/self_hosting.md)
- [Phase 2 status](docs/phase2_status.md)
- [Testing guide](docs/testing.md)
- [Tools](docs/tools.md)
- [Standard library and builtins](docs/standard_library.md)
- [Known limitations](docs/limitations.md)
- [Historical development assignments](docs/assignments/README.md)

## Contributing

Contributions are welcome. Read [CONTRIBUTING.md](CONTRIBUTING.md) for the build, test, and golden-file workflow. Please use [SECURITY.md](SECURITY.md) for vulnerability reports rather than opening a public issue with sensitive details.

## License

Nova is available under the [MIT License](LICENSE).
