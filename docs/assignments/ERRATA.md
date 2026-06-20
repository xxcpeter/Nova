# Nova Assignment Errata

This file records places where historical assignment text differs from the final implementation.

## Step 1

The original language specification represented the initial language design. It is now preserved as:

```text
Nova_Phase1_Prototype_Plan.md
```

## Step 15

Parser output was extended to include expression trees. Earlier parser trace assumptions may not represent the final parser structure.

## Step 18

`import` was implemented as a source-level include system, not a full module system.

## Step 19

Nova-written codegen is a C codegen prototype. It is not a complete replacement for the C++ codegen backend.

## Step 21–22

Self-hosting work introduced stage0/stage1 codegen and then stage-generated tool usability. Some earlier dummy returns were removed after no-return control-flow handling improved.

## Step 23

Diagnostics became source-aware. Historical examples that show only `line:column` may be outdated.

## Step 24

Import v2 and runtime/builtin cleanup were combined. `str_vec_*` legacy APIs were removed from the supported language surface.

## Step 27

Test layout was normalized around `positive/negative` directories for Nova tool tests. Older references to `tests/tools/lexer`, `tests/tools/expr_parser`, or flat `tests/tools/codegen/*.nv` paths may be stale.

## Step 28

`tools/nova_compile.nv` was added as the Nova-written compile-to-C driver. `tools/nova_codegen.nv` remains as a backend/codegen regression tool.

## Step 29

Runtime process/filesystem APIs and `tools/nova_build.nv` were added as a prototype build driver. This is not a full package manager or build system.
