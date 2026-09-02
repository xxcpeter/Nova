# Contributing to Nova

Thanks for helping improve Nova. The project is an educational compiler with two implementations that must remain behaviorally aligned: the C++ seed compiler and the compiler components written in Nova.

## Development setup

You need:

- CMake 3.16 or newer;
- a C++20 compiler;
- a C compiler available as `cc`;
- Bash and a POSIX-like environment.

Configure and build from the repository root:

```bash
cmake -S . -B build
cmake --build build --parallel
```

## Before submitting a change

Run the full CTest suite:

```bash
ctest --test-dir build --parallel 2 --output-on-failure
```

Run the canonical self-hosting workflow explicitly when changing `lib/`, `tools/`, code generation, the runtime, or bootstrap behavior:

```bash
scripts/self_host.sh
```

Validate the VS Code extension files when changing editor support:

```bash
(cd tools/vscode-nova-syntax && scripts/validate.sh)
```

## Choosing regression tests

Add the smallest test that demonstrates the intended behavior:

| Change | Primary test area |
| --- | --- |
| Tokenization | `tests/lexer/` and/or `tests/tools/tokenizer/` |
| Parsing | `tests/parser/` and/or `tests/tools/parser/` |
| Type or semantic checks | `tests/sema/`, `tests/tools/checker/`, or `tests/tools/typecheck/` |
| C code generation/runtime | `tests/codegen/` and/or `tests/tools/codegen/` |
| Import behavior | `tests/import/` and/or `tests/tools/frontend_import/` |
| Compile driver/CLI | `tests/tools/compile/` or `tests/tools/cli/` |
| Bootstrap behavior | `scripts/self_host.sh` and its representative programs |

The detailed test layout and targeted commands are documented in [docs/testing.md](docs/testing.md).

## Golden files

Positive tests generally pair a `.nv` input with an `.out` file containing expected stdout. Negative tests pair the input with an `.err` file containing expected diagnostics.

- Keep each fixture focused on one behavior.
- Update a golden file only when the behavior change is intentional.
- Explain diagnostic or output changes in the pull request.
- Preserve source locations in negative import and diagnostic tests.
- Where both implementations cover a feature, add or update tests for both paths.

## Pull requests

Keep pull requests focused and describe:

- what behavior changed and why;
- which implementation layers are affected;
- which regression tests were added or updated;
- whether bootstrap output or documented limitations changed.

Update relevant files in `docs/` when a language feature, compiler boundary, builtin, or milestone guarantee changes. Historical assignments under `docs/assignments/` record the development process and normally should not be rewritten to describe current behavior.

## Security

Do not include undisclosed vulnerability details in a public issue or pull request. Follow the private reporting process in [SECURITY.md](SECURITY.md).
