# Nova Self-Hosting

## Overview

Nova currently supports a self-hosting codegen workflow.

The workflow does not remove the C++ seed compiler. Instead, it proves that:

```text
C++ seed compiler can build stage0 Nova codegen.
stage0 Nova codegen can build stage1 Nova codegen.
stage1 Nova codegen can build stage2 Nova codegen.
stage1/stage2 generated tools can compile representative programs with matching behavior.
```

---

## Command

From the repository root:

```bash
scripts/self_host.sh
```

---

## Stages

### Stage 0

The C++ seed compiler builds `tools/nova_codegen.nv`.

```text
build/nova_compile tools/nova_codegen.nv
  -> nova_codegen_stage0.c
  -> nova_codegen_stage0
```

### Stage 1

`nova_codegen_stage0` builds `tools/nova_codegen.nv` again.

```text
nova_codegen_stage0 tools/nova_codegen.nv
  -> nova_codegen_stage1.c
  -> nova_codegen_stage1
```

### Stage 2

`nova_codegen_stage1` builds `tools/nova_codegen.nv` again.

```text
nova_codegen_stage1 tools/nova_codegen.nv
  -> nova_codegen_stage2.c
  -> nova_codegen_stage2
```

---

## Representative Program Validation

The script uses stage1 and stage2 codegen to compile representative programs.

It then:

```text
compiles stage1-generated C
runs stage1-generated executable
captures output

compiles stage2-generated C
runs stage2-generated executable
captures output

compares outputs
```

The comparison is behavior-based. The generated C does not have to be byte-for-byte identical.

---

## Nova Compile Driver Smoke

The self-host script also builds:

```text
tools/nova_compile.nv
  -> nova_compile_stage1.c
  -> nova_compile_stage1
```

Then it uses `nova_compile_stage1` to compile a representative Nova program.

This proves that a stage-generated Nova-written compile driver can compile Nova source to C.

---

## Nova Build Driver Prototype

Nova also has a build driver prototype:

```text
tools/nova_build.nv
```

It can delegate to the self-host workflow:

```bash
./build/nova_compile tools/nova_build.nv /tmp/nova_build.c
cc /tmp/nova_build.c runtime/nova_runtime.c -I runtime -o /tmp/nova_build
/tmp/nova_build self-host
```

This is a prototype build driver, not a full package manager or incremental build system.

---

## What This Proves

The self-host workflow proves:

```text
Nova-written codegen can regenerate itself.
Generated Nova codegen tools can compile representative Nova programs.
Nova-written compile driver can compile Nova source to C.
Import expansion, diagnostics, checker, codegen, runtime, and vector support are sufficient for the tested workflow.
```

---

## What This Does Not Prove

The workflow does not prove:

```text
native code generation
full replacement of the C++ seed compiler
complete module system
complete package/build system
byte-for-byte identical generated C
full optimizer
complete standard library
production-grade compiler maturity
```

---

## Expected Success Output

A successful run ends with:

```text
[self-host] all checks passed
```

The exact intermediate messages may vary as the script evolves.
