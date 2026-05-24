# Nova Limitations

This document records known limitations of the current Phase 1 bootstrap prototype.

---

## Import System

- `import` is a textual include mechanism, not a real module system.
- All imported declarations share one global namespace.
- There is no `export`, `private`, aliasing, or package search path.
- Include-once behavior prevents duplicate inclusion, but it does not provide module visibility or namespacing.

---

## Diagnostics

- Diagnostics for imported source may use flattened source locations.
- There is no source map yet.
- Diagnostics currently report line and column, but not source file identity for imported code.

---

## Nova Codegen

- The Nova-written codegen is a prototype backend.
- It is not a replacement for the C++ codegen backend.
- It supports a useful subset of Nova, but does not guarantee complete parity with the C++ compiler.
- Generated C is intended to be readable and testable, not optimized.

---

## Vectors

- Nested `vec` types are not supported.
- Generated vector helpers currently do not free allocated memory.
- Vector helper functions are generated per element type.

---

## Memory Management

- Runtime allocation is checked in generated vector helpers.
- There is no ownership, lifetime, or destructor model.
- Most generated programs rely on process exit for cleanup.

---

## Tooling

- The VS Code extension provides syntax highlighting only.
- There is no hover, go-to-definition, reference search, formatting, or diagnostics.
- There is no LSP server yet.

---

## Assignments

- Historical assignments record the development process.
- Some early goals were revised, merged, or replaced in later steps.
- The final implementation may differ from earlier step goals.
- The current project behavior is defined by the source tree, tests, and current documentation.