# Nova Assignment Errata

## Step 1

The original `docs/specification.md` represented the initial language specification.
It has been renamed to `docs/step1_language_spec.md`.

## Step 15

Parser output was extended to include expression trees.
Earlier parser trace assumptions may not represent the final parser structure.

## Step 18

`import` was implemented as a source-level include system, not a full module system.

## Step 19

Nova-written codegen is a tiny C codegen prototype.
It is not a complete replacement for the C++ codegen backend.