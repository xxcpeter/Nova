# Nova Project — Step 30
## 题目：Phase 2 Self-Hosting Milestone Closure

---

## 1. 当前起点

Step 29 已经完成或接近完成：

```text
runtime process/filesystem APIs 已加入
run_command 可执行 trusted shell command
file_exists / dir_exists / make_dir / remove_file 可用
C++ sema / Nova checker / Nova codegen 已同步 builtin
runtime API tests 已接入
tools/nova_build.nv prototype 已新增
nova_build bad-args test 已接入
ctest 通过
scripts/self_host.sh 通过
```

Phase 2 到这里已经完成了主要实作目标：

```text
Nova-written tokenizer
Nova-written parser prototype
Nova-written declaration checker
Nova-written expression parser/type checker
Nova-written frontend
Nova import/library split
Nova-written C codegen prototype
self-hosting codegen parity
source-aware diagnostics
import system v2
runtime/builtin/stdlib cleanup
VS Code tooling
golden/regression cleanup
Nova-written compile driver
CLI regression infrastructure
runtime process/filesystem APIs
Nova-written build driver prototype
```

Step 30 不再新增大功能。  
它是 Phase 2 的 milestone closure。

---

## 2. 本次作业目标

Step 30 的目标是：

> **冻结 Phase 2 状态，完成最终验证、文档整理、milestone report，并为 Phase 3 建立清晰入口。**

核心成果：

```text
1. 明确 Phase 2 achieved / not achieved
2. 更新 README
3. 更新 docs/bootstrap.md
4. 更新 docs/self_hosting.md
5. 更新 docs/tools.md
6. 更新 docs/standard_library.md
7. 更新 docs/limitations.md
8. 新增或更新 docs/phase2_status.md
9. 新增或更新 docs/phase3_plan.md
10. 清理 stale references / old tool names / outdated paths
11. 运行完整 release-style validation
12. 给出 Phase 2 completion commit
```

---

## 3. 本阶段定位

Step 30 是 documentation + validation + milestone closure step。

它应该解决：

```text
Phase 2 到底完成了什么
哪些能力已经稳定
哪些仍然是 prototype
用户如何 build / test / self-host
开发者如何运行 tools
Phase 3 从哪里开始
旧文档里的 stale 内容
旧路径 / 旧工具名 / 旧 usage
```

它不应该引入：

```text
新语法
新 runtime API
新 compiler feature
新 test runner
大规模重构
新的 codegen parity work
新的 module system
LSP / formatter
package manager
```

Step 30 的原则是：

```text
No new major feature.
Stabilize, document, validate, close.
```

---

## 4. 本阶段不做什么

Step 30 不做：

```text
module/export/private system
formatter
LSP
warnings/fix-its
code frames
package manager
parallel build
native backend
new type system features
new stdlib families
```

这些应该留给 Phase 3。

如果在 Step 30 发现 bug：

```text
可以修小 bug
可以修 docs / tests / stale path
可以修 broken smoke
```

但不要把 Step 30 变成新 feature step。

---

# Part A：Phase 2 Status Report

---

## 5. 新增 docs/phase2_status.md

新增：

```text
docs/phase2_status.md
```

这是 Phase 2 的 milestone report。

它应该回答：

```text
Phase 2 的目标是什么？
现在完成了什么？
怎么验证？
哪些能力是 stable？
哪些能力还是 prototype？
下一阶段是什么？
```

---

## 6. phase2_status.md 推荐结构

```markdown
# Nova Phase 2 Status

## Summary

Phase 2 establishes Nova's self-hosting frontend/codegen path and supporting developer tooling.

## Completed

- Nova-written tokenizer
- Nova-written parser prototype
- Nova-written declaration checker
- Nova-written expression/type checker
- Nova-written frontend
- Nova-written C codegen prototype
- Import and library split
- Source-aware diagnostics
- Runtime/builtin/stdlib cleanup
- Self-hosting codegen parity workflow
- Golden/regression test cleanup
- VS Code syntax tooling
- Nova-written compile driver
- CLI regression tests
- Runtime process/filesystem APIs
- Nova build driver prototype

## Validation

- `ctest --test-dir build -j8 --output-on-failure`
- `scripts/self_host.sh`
- generated `nova_compile_stage1` smoke
- `nova_build` smoke, if run manually

## Stable enough for Phase 2

- C++ seed compiler
- Nova-written tokenizer/parser/checker/frontend/codegen toolchain
- compile-to-C workflow
- import flattening workflow
- source-aware diagnostics
- regression test suite

## Prototype / limitations

- build driver delegates to shell commands
- `run_command` is shell-based and trusted-input only
- import system is file-flattening, not a full module namespace
- codegen is C backend only
- diagnostics do not yet have rich code frames
- no package manager
- no LSP

## Next phase

Phase 3 focuses on language ergonomics, module semantics, diagnostics v2, formatter/LSP, and build/tooling maturation.
```

---

## 7. Be honest about prototype status

Do not overstate Phase 2.

Use wording like:

```text
self-hosting codegen path
Nova-written compile driver
build driver prototype
source-aware diagnostics v1
import system v2
```

Avoid wording like:

```text
production compiler
full self-hosted toolchain
complete build system
complete module system
```

Phase 2 is a strong milestone, but still educational/prototype.

---

# Part B：README Cleanup

---

## 8. Update README.md

README should become the top-level entry point.

It should explain:

```text
what Nova is
how to build
how to run tests
how to run self-host workflow
what tools exist
where docs live
what Phase 2 status is
```

---

## 9. README recommended structure

```markdown
# Nova

Nova is a small educational programming language and compiler project.

## Build

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

## C++ seed compiler

```bash
./build/nova_compile input.nv output.c
cc output.c runtime/nova_runtime.c -I runtime -o program
./program
```

## Nova-written compile driver

```bash
./build/nova_compile tools/nova_compile.nv /tmp/nova_compile.c
cc /tmp/nova_compile.c runtime/nova_runtime.c -I runtime -o /tmp/nova_compile
/tmp/nova_compile input.nv output.c
```

## Nova build driver prototype

```bash
./build/nova_compile tools/nova_build.nv /tmp/nova_build.c
cc /tmp/nova_build.c runtime/nova_runtime.c -I runtime -o /tmp/nova_build
/tmp/nova_build self-host
```

## Documentation

- `docs/bootstrap.md`
- `docs/self_hosting.md`
- `docs/tools.md`
- `docs/standard_library.md`
- `docs/limitations.md`
- `docs/phase2_status.md`
- `docs/phase3_plan.md`
```

---

## 10. README should not be too long

README should not duplicate every doc.

Good README style:

```text
short command examples
clear pointers to docs
honest status summary
```

Detailed explanations belong in docs.

---

# Part C：Self-hosting Docs

---

## 11. Add or update docs/self_hosting.md

Create or update:

```text
docs/self_hosting.md
```

This should be the authoritative self-hosting doc.

---

## 12. self_hosting.md recommended structure

```markdown
# Nova Self-Hosting

## Overview

Nova currently supports a self-hosting codegen workflow.

## Stages

### Stage 0

The C++ seed compiler builds `tools/nova_codegen.nv`.

### Stage 1

The generated `nova_codegen_stage0` builds `tools/nova_codegen.nv` again.

### Stage 2

The generated `nova_codegen_stage1` builds `tools/nova_codegen.nv` again.

## Validation

Stage 1 and Stage 2 generated tools compile representative programs.
Their outputs are compared.

## Nova compile driver smoke

The self-host script also builds `tools/nova_compile.nv` into `nova_compile_stage1`
and uses it to compile a representative program.

## Command

```bash
scripts/self_host.sh
```

## What this proves

- Nova-written codegen can regenerate itself
- generated tools can compile representative Nova programs
- Nova-written compile driver can compile Nova source to C

## What this does not prove

- full native self-hosting without C backend
- package management
- full module system
- full build system
```

---

## 13. Document actual file names

Use actual script names and stage names:

```text
nova_codegen_stage0
nova_codegen_stage1
nova_codegen_stage2
nova_compile_stage1
hello_from_nova_compile_stage1
```

If your script uses different exact names, document those exact names.

---

# Part D：Bootstrap Docs

---

## 14. Update docs/bootstrap.md

`docs/bootstrap.md` should explain the bootstrapping story.

Include:

```text
C++ seed compiler
Nova-written tools
stage0/stage1/stage2 codegen
Nova-written compile driver
Nova build driver prototype
```

---

## 15. Avoid stale bootstrap claims

Remove or update old claims like:

```text
only C++ tools exist
Nova parser/checker/codegen not available
old tool names like nova_expr_parser
old tests/tools/codegen paths without positive/
```

---

## 16. Bootstrap diagram

Add a small diagram:

```text
C++ seed compiler
  └── builds tools/nova_codegen.nv
        └── nova_codegen_stage0
              └── builds tools/nova_codegen.nv
                    └── nova_codegen_stage1
                          └── builds tools/nova_codegen.nv
                                └── nova_codegen_stage2

C++ seed compiler or stage0 codegen
  └── builds tools/nova_compile.nv
        └── nova_compile_stage1
              └── compiles Nova source to C
```

---

# Part E：Tools Docs

---

## 17. Update docs/tools.md

Make sure `docs/tools.md` documents:

```text
C++ seed tools:
  build/nova_lex
  build/nova_parse
  build/nova_sema
  build/nova_compile

Nova-written tools:
  tools/nova_tokenizer.nv
  tools/nova_parser.nv
  tools/nova_checker.nv
  tools/nova_frontend.nv
  tools/nova_codegen.nv
  tools/nova_compile.nv
  tools/nova_build.nv
```

---

## 18. Usage strings must match tests

Docs usage strings should match CLI tests exactly:

```text
Usage: nova_tokenizer <input.nv> <output.tok>
Usage: nova_parser <input.nv> <output.out>
Usage: nova_checker <input.nv> <output.check>
Usage: nova_frontend <tokens|parse|check> <input.nv> <output>
Usage: nova_codegen <input.nv> <output.c>
Usage: nova_compile <input.nv> <output.c>
Usage: nova_build <self-host|compile|test> [args...]
```

If the actual usage differs, update docs or tool, but keep them consistent.

---

## 19. Clarify nova_codegen vs nova_compile

Docs should say:

```text
nova_codegen.nv is a backend/codegen regression tool.
nova_compile.nv is the official Nova-written compile-to-C driver.
```

Do not imply `nova_codegen.nv` is obsolete unless you actually removed it.

---

# Part F：Standard Library Docs

---

## 20. Update docs/standard_library.md

Make sure docs include all current runtime builtins:

```text
print_int
print_str
str_eq
str_concat
str_len
str_get
str_slice
str_starts_with
str_ends_with
str_contains
int_to_str
read_file
write_file
buf_new
buf_push_str
buf_push_int
buf_to_str
arg_count
arg_get
nova_runtime_error
run_command
file_exists
dir_exists
make_dir
remove_file
```

---

## 21. Process/filesystem warnings

Document:

```text
run_command executes through host shell
run_command is for trusted build tooling
paths with spaces may not work in nova_build prototype
make_dir is not recursive
remove_file removes files, not directories
remove_file missing file behavior
```

---

## 22. Builtin consistency audit

Search for all builtins in:

```text
runtime/nova_runtime.h
runtime/nova_runtime.c
src/sema.cpp
lib/checker.nv
lib/codegen_c.nv
docs/standard_library.md
VS Code syntax grammar
```

Make sure:

```text
same names
same arg count
same return type
same documented behavior
```

---

# Part G：Limitations Docs

---

## 23. Update docs/limitations.md

This file should clearly define current limits.

Suggested sections:

```markdown
# Nova Limitations

## Language

- no generics except builtin vec<T>
- no interfaces/traits
- no closures
- limited module semantics

## Imports

- import system flattens source files
- no public/private exports
- no namespace isolation

## Diagnostics

- source-aware diagnostics exist
- no rich code frames yet
- no warnings/fix-its yet

## Runtime

- C runtime only
- process API uses shell commands
- filesystem APIs are minimal

## Build system

- nova_build is a prototype
- no package manager
- no dependency graph cache
- no parallel build scheduler

## Platform

- POSIX-like assumptions in some tests/scripts
```

---

## 24. Avoid discouraging wording

Use honest but positive phrasing:

```text
Current limitations
Prototype status
Planned for Phase 3
```

Avoid making it sound broken.

---

# Part H：Testing Docs

---

## 25. Update docs/testing.md

Make sure it documents current test categories:

```text
C++ lexer/parser/sema tests
C++ codegen execution tests
import tests
Nova tokenizer/parser/checker/frontend tests
Nova codegen tests
Nova compile driver tests
CLI bad-args tests
runtime process/filesystem tests
self-host test
```

---

## 26. Current test layout

Document current layout:

```text
tests/<component>/positive
tests/<component>/negative

tests/tools/<tool>/positive
tests/tools/<tool>/negative

tests/tools/frontend/positive/<mode>
tests/tools/frontend/negative/<mode>

tests/tools/frontend_import/positive/<mode>/<case>
tests/tools/frontend_import/negative/<mode>/<case>

tests/tools/compile/positive
tests/tools/cli/negative
```

Adjust to actual repository layout.

---

## 27. How to run subsets

Document examples:

```bash
ctest --test-dir build -L nova_tool --output-on-failure
ctest --test-dir build -L codegen --output-on-failure
ctest --test-dir build -L selfhost --output-on-failure
ctest --test-dir build -L cli --output-on-failure
```

If label names differ, use actual labels.

---

# Part I：Phase 3 Plan

---

## 28. Add or update docs/phase3_plan.md

Create:

```text
docs/phase3_plan.md
```

This should be short and directional, not a giant spec.

---

## 29. Phase 3 recommended themes

Suggested Phase 3 tracks:

```text
1. Module semantics
2. Diagnostics v2
3. Language ergonomics
4. Standard library expansion
5. Tooling: formatter + LSP
6. Build system maturation
7. Codegen parity and cleanup
```

---

## 30. Phase 3 possible steps

Suggested outline:

```text
Step 31 — Module Semantics v1
Step 32 — Diagnostics v2: Code Frames
Step 33 — Formatter Prototype
Step 34 — LSP Prototype
Step 35 — Language Ergonomics I
Step 36 — Standard Library Expansion I
Step 37 — Build System Maturation
Step 38 — Codegen Cleanup and IR Boundary
Step 39 — Docs and Examples Expansion
Step 40 — Phase 3 Milestone
```

Do not overcommit.  
Frame this as provisional.

---

# Part J：Stale Reference Cleanup

---

## 31. Search for old names

Search docs/tests/CMake for stale references:

```text
nova_expr_parser
tests/tools/codegen/hello.nv
tests/tools/parser old path
tests/tools/lexer old path
str_vec
old import test paths
old usage strings
old Step numbers
```

---

## 32. Search commands

Suggested:

```bash
grep -R "nova_expr_parser" .
grep -R "tests/tools/codegen/hello.nv" .
grep -R "tests/tools/lexer" .
grep -R "tests/tools/parser" .
grep -R "str_vec" .
grep -R "Usage:" tools docs tests cmake README.md
```

Adjust to avoid generated/build/tmp directories if needed:

```bash
grep -R --exclude-dir=build --exclude-dir=tmp "nova_expr_parser" .
```

---

## 33. What to fix

Fix stale references in:

```text
README.md
docs/*.md
CMakeLists.txt
cmake/*.cmake
scripts/*.sh
tests expected files
VS Code package docs, if any
```

Do not fix generated build artifacts.

---

# Part K：Final Validation

---

## 34. Clean build validation

Run from clean-ish state:

```bash
rm -rf build tmp
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build -j8 --output-on-failure
scripts/self_host.sh
```

---

## 35. Manual tool smoke

Run at least:

```bash
./build/nova_compile tests/tools/compile/positive/hello.nv /tmp/nova_hello.c
cc /tmp/nova_hello.c runtime/nova_runtime.c -I runtime -o /tmp/nova_hello
/tmp/nova_hello
```

Expected:

```text
hello
```

---

## 36. Nova-written compile driver smoke

```bash
./build/nova_compile tools/nova_compile.nv /tmp/nova_compile.c
cc /tmp/nova_compile.c runtime/nova_runtime.c -I runtime -o /tmp/nova_compile
/tmp/nova_compile tests/tools/compile/positive/hello.nv /tmp/hello_by_nova_compile.c
cc /tmp/hello_by_nova_compile.c runtime/nova_runtime.c -I runtime -o /tmp/hello_by_nova_compile
/tmp/hello_by_nova_compile
```

Expected:

```text
hello
```

---

## 37. Nova build driver smoke

```bash
./build/nova_compile tools/nova_build.nv /tmp/nova_build.c
cc /tmp/nova_build.c runtime/nova_runtime.c -I runtime -o /tmp/nova_build
/tmp/nova_build compile tests/tools/compile/positive/hello.nv /tmp/hello_by_nova_build.c
cc /tmp/hello_by_nova_build.c runtime/nova_runtime.c -I runtime -o /tmp/hello_by_nova_build
/tmp/hello_by_nova_build
```

Expected:

```text
hello
```

Optional:

```bash
/tmp/nova_build self-host
```

---

## 38. CLI negative smoke

Run:

```bash
ctest --test-dir build -L cli --output-on-failure
```

Expected:

```text
all CLI bad-args tests pass
```

---

# Part L：Version / Tagging Notes

---

## 39. Optional tag

If using Git tags, optional:

```text
phase2-self-hosting-milestone
```

or:

```text
v0.2-phase2
```

This is optional.

---

## 40. Commit message

Recommended commit:

```text
Step 30 close Phase 2 self-hosting milestone
```

Long form:

```text
Step 30 close Phase 2 self-hosting milestone

- Document Phase 2 status and validation workflow
- Update self-hosting, tools, testing, and standard library docs
- Clean stale tool names and paths
- Add Phase 3 planning notes
- Run final ctest and self-host validation
```

---

# Part M：Recommended Implementation Order

---

## 41. Step A：Status doc

Add:

```text
docs/phase2_status.md
```

---

## 42. Step B：Core docs update

Update:

```text
README.md
docs/bootstrap.md
docs/self_hosting.md
docs/tools.md
docs/standard_library.md
docs/limitations.md
docs/testing.md
```

---

## 43. Step C：Phase 3 plan

Add:

```text
docs/phase3_plan.md
```

---

## 44. Step D：Stale reference cleanup

Search and fix old names/paths.

---

## 45. Step E：Validation

Run:

```bash
rm -rf build tmp
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build -j8 --output-on-failure
scripts/self_host.sh
```

---

## 46. Step F：Manual smoke

Run:

```text
C++ seed compiler smoke
Nova-written compile driver smoke
Nova build driver compile smoke
optional nova_build self-host smoke
```

---

## 47. Step G：Final docs pass

Make sure docs agree with actual commands.

---

# Part N：验收标准

---

## 48. 合格

```text
docs/phase2_status.md exists
README updated
docs/self_hosting.md updated or added
docs/tools.md updated
docs/limitations.md updated
docs/testing.md updated
docs/phase3_plan.md exists
stale old tool names removed
ctest passes
scripts/self_host.sh passes
```

---

## 49. 良好

```text
clean build from rm -rf build tmp passes
Nova-written compile driver smoke documented and works
Nova build driver smoke documented and works
standard library docs match runtime/checker/codegen builtin tables
test layout docs match actual repository
Phase 3 plan is clear and realistic
```

---

## 50. 优秀

```text
Phase 2 status doc clearly separates stable vs prototype
self-hosting docs explain stage0/stage1/stage2 workflow accurately
README is concise and newcomer-friendly
all stale paths/tool names are gone
release-style validation commands are reproducible
```

---

## 51. 完成标志

Step 30 完成时，应该可以说：

```text
Nova Phase 2 is closed as a documented, validated self-hosting milestone.
```

至少这些成立：

```text
Phase 2 status is documented
self-hosting workflow is documented
tools are documented
limitations are documented
Phase 3 entry plan exists
ctest passes
scripts/self_host.sh passes
manual compile/build driver smoke works
```

---

## 52. Final Phase 2 statement

At the end of Step 30, the project should be able to honestly state:

```text
Nova has a working C++ seed compiler, a Nova-written frontend/codegen toolchain,
a Nova-written compile driver, source-aware diagnostics, a regression test suite,
and a validated self-hosting codegen workflow.
```

And also honestly state:

```text
Nova is still an educational/prototype compiler.
The build driver and process APIs are minimal.
The import system is not a full module system.
Diagnostics are source-aware but not yet rich code-frame diagnostics.
Phase 3 will focus on ergonomics, modules, diagnostics, and tooling maturity.
```
