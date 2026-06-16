# Nova Project — Step 25
## 题目：Self-hosted Build Driver + Codegen Parity Expansion

---

## 1. 当前起点

Step 24 已经完成：

```text
import behavior stabilized
./ and ../ import paths normalized
include-once behavior tested
cyclic / missing / malformed import diagnostics stable
runtime / builtin surface cleaned up
str_vec_* removed
buf_* kept as supported string builder utility
docs/standard_library.md added
C++ sema / Nova checker / Nova codegen builtin behavior aligned
```

Phase 2 现在进入真正的 self-hosting workflow 阶段。

之前 Steps 21–22 已经证明：

```text
C++ seed compiler can compile tools/nova_codegen.nv into stage0 nova_codegen
stage0 nova_codegen can compile tools/nova_codegen.nv into stage1 nova_codegen
stage-generated Nova tools can run on representative inputs
```

但目前 self-hosting 过程仍然比较手动：

```text
需要手动调用 nova_compile / nova_codegen
需要手动 cc generated C
需要手动比较 stage outputs
codegen parity 缺口只能通过零散测试发现
```

Step 25 的目标是把这些流程收束成一个可重复的 self-hosted build workflow，并在这个过程中补齐 Nova codegen 的剩余关键 parity 缺口。

---

## 2. 本次作业目标

Step 25 的目标是：

> **建立可重复的 self-hosted build driver，并扩展 Nova codegen parity，使 stage1 / stage2 workflow 可以稳定验证。**

核心成果包括：

```text
scripts/self_host.sh 或等价 driver
stage0 -> stage1 -> stage2 workflow
stage1 / stage2 能编译代表性 programs
stage1 / stage2 generated behavior 一致
Nova codegen 支持 self-host workflow 中暴露的剩余语言特性
bootstrap / self-host regression tests 可重复运行
```

---

## 3. 本阶段定位

Step 25 是 Phase 2 的核心 self-hosting step。

它应该解决：

```text
self-hosting 过程太手动
stage0/stage1/stage2 缺少统一 driver
stage-generated codegen 缺少系统回归
Nova codegen parity 缺口没有集中补齐
stage outputs 没有稳定比较方式
```

它不要求解决：

```text
完整优化器
native backend
incremental build cache
full module system
package manager
LSP / editor integration
complete standard library expansion
perfect C output formatting
```

---

## 4. 本阶段不做什么

Step 25 不做：

```text
新的语言大特性
module/export/private
separate compilation
incremental compiler cache
native code generation
LLVM backend
C optimizer
VS Code tooling
```

VS Code / developer tooling 留给 Step 26。

Phase 2 milestone 总结留给 Step 27。

---

# Part A：Self-hosted Build Driver

---

## 5. Driver 目标

新增一个可重复运行的 self-host driver。

推荐文件：

```text
scripts/self_host.sh
```

或者如果你想用 Nova 写一版 prototype，也可以后续新增：

```text
tools/nova_build.nv
```

但 Step 25 的最低要求是 shell driver，因为它最直接、最可靠。

---

## 6. Driver 要完成的 pipeline

推荐 pipeline：

```text
1. 使用 C++ seed compiler 编译 tools/nova_codegen.nv
   -> stage0 nova_codegen

2. 使用 stage0 nova_codegen 编译 tools/nova_codegen.nv
   -> stage1 nova_codegen

3. 使用 stage1 nova_codegen 编译 tools/nova_codegen.nv
   -> stage2 nova_codegen

4. 使用 stage1 / stage2 编译代表性 programs

5. 编译 generated C

6. 运行 executables

7. 比较 output
```

目标关系：

```text
C++ seed compiler
  -> stage0 nova_codegen
    -> stage1 nova_codegen
      -> stage2 nova_codegen
```

---

## 7. 推荐目录布局

Driver 可以使用临时目录：

```bash
WORK="${TMPDIR:-/tmp}/nova_self_host"
```

生成文件建议：

```text
$WORK/stage0/nova_codegen.c
$WORK/stage0/nova_codegen

$WORK/stage1/nova_codegen.c
$WORK/stage1/nova_codegen

$WORK/stage2/nova_codegen.c
$WORK/stage2/nova_codegen

$WORK/programs/<name>.stage1.c
$WORK/programs/<name>.stage1
$WORK/programs/<name>.stage1.out

$WORK/programs/<name>.stage2.c
$WORK/programs/<name>.stage2
$WORK/programs/<name>.stage2.out
```

Driver 默认可以清空工作目录：

```bash
rm -rf "$WORK"
mkdir -p "$WORK"
```

后续如果想调试，可以支持：

```bash
KEEP=1 scripts/self_host.sh
```

但不是必须。

---

## 8. Stage0 生成方式

stage0 由 C++ seed compiler 生成：

```bash
build/nova_compile tools/nova_codegen.nv "$WORK/stage0/nova_codegen.c"
cc "$WORK/stage0/nova_codegen.c" runtime/nova_runtime.c -I runtime -o "$WORK/stage0/nova_codegen"
```

如果你的 `nova_compile` 参数顺序不同，按实际项目命令调整。

---

## 9. Stage1 生成方式

stage1 由 stage0 生成：

```bash
"$WORK/stage0/nova_codegen" tools/nova_codegen.nv "$WORK/stage1/nova_codegen.c"
cc "$WORK/stage1/nova_codegen.c" runtime/nova_runtime.c -I runtime -o "$WORK/stage1/nova_codegen"
```

---

## 10. Stage2 生成方式

stage2 由 stage1 生成：

```bash
"$WORK/stage1/nova_codegen" tools/nova_codegen.nv "$WORK/stage2/nova_codegen.c"
cc "$WORK/stage2/nova_codegen.c" runtime/nova_runtime.c -I runtime -o "$WORK/stage2/nova_codegen"
```

---

## 11. Stage1 / Stage2 比较策略

不要一开始要求 generated C byte-for-byte 完全一致。

原因：

```text
generated C 可能包含顺序差异
空白差异无意义
helper emission order 可能受 traversal 影响
```

Step 25 推荐比较行为，而不是 C 文本。

最低要求：

```text
stage1-generated executable output == stage2-generated executable output
```

可以后续增加 normalized C comparison：

```text
去掉空白
去掉空行
比较稳定 helper order 后的文本
```

但这不是本 step 的必要条件。

---

## 12. Driver helper functions

`scripts/self_host.sh` 建议写 helper，避免重复：

```bash
compile_c() {
  local c_file="$1"
  local exe_file="$2"
  cc "$c_file" runtime/nova_runtime.c -I runtime -o "$exe_file"
}

run_codegen() {
  local codegen="$1"
  local input="$2"
  local output_c="$3"
  "$codegen" "$input" "$output_c"
}

run_and_capture() {
  local exe="$1"
  local output="$2"
  "$exe" > "$output"
}

compare_files() {
  local a="$1"
  local b="$2"
  diff -u "$a" "$b"
}
```

推荐脚本开启：

```bash
set -euo pipefail
```

并打印阶段信息：

```bash
echo "[self-host] building stage0"
echo "[self-host] building stage1"
echo "[self-host] building stage2"
```

---

# Part B：Representative Program Matrix

---

## 13. 为什么需要 program matrix

Stage1 / stage2 只编译 `tools/nova_codegen.nv` 还不够。

还需要确认 generated codegen 能编译各种代表性 Nova programs：

```text
基础函数调用
递归
if / else if / else
while
struct
enum
field access
vec<int>
vec<str>
vec<struct>
string operations
file/source-loader style code
parser/checker/codegen style code
```

这些程序可以来自已有 tests，也可以新增 dedicated self-host samples。

---

## 14. 推荐测试程序集合

Driver 中先选一组小而覆盖广的 programs。

推荐从已有 tests 选：

```text
tests/tools/codegen/hello.nv
tests/tools/codegen/recursion.nv
tests/tools/codegen/vec_basic.nv
tests/tools/codegen/vec_str_basic.nv
tests/tools/codegen/vec_struct_basic.nv
tests/tools/codegen/string_plus.nv
tests/tools/codegen/string_ends_with.nv
tests/tools/codegen/struct_basic.nv
tests/tools/codegen/enum_basic.nv
tests/tools/codegen/import_basic/main.nv
```

如果实际路径不同，按现有测试命名调整。

---

## 15. Stage comparison example

对每个 program 做：

```bash
"$WORK/stage1/nova_codegen" "$program" "$WORK/programs/$name.stage1.c"
cc "$WORK/programs/$name.stage1.c" runtime/nova_runtime.c -I runtime -o "$WORK/programs/$name.stage1"
"$WORK/programs/$name.stage1" > "$WORK/programs/$name.stage1.out"

"$WORK/stage2/nova_codegen" "$program" "$WORK/programs/$name.stage2.c"
cc "$WORK/programs/$name.stage2.c" runtime/nova_runtime.c -I runtime -o "$WORK/programs/$name.stage2"
"$WORK/programs/$name.stage2" > "$WORK/programs/$name.stage2.out"

diff -u "$WORK/programs/$name.stage1.out" "$WORK/programs/$name.stage2.out"
```

如果 program 需要 command-line args，先跳过或在 matrix 中记录 args。

---

## 16. Driver 输出

成功时建议输出：

```text
[self-host] stage0 ok
[self-host] stage1 ok
[self-host] stage2 ok
[self-host] checking program hello
[self-host] checking program vec_struct_basic
[self-host] all checks passed
```

失败时，让 shell / diff / compiler error 自然输出即可。

---

# Part C：Codegen Parity Expansion

---

## 17. Codegen parity 的目标

Step 25 的 parity 目标不是“支持完整 Nova 语言所有未来特性”，而是：

> **支持当前 Nova compiler tools 和 representative tests 所需的语言特性。**

也就是说，如果 self-host driver 暴露 codegen 缺口，本 step 要补齐。

---

## 18. 重点检查的 codegen areas

### 18.1 Function calls

确认：

```text
user function return type lookup 正确
builtin return type lookup 正确
no-return function 处理正确
argument expression codegen 正确
nested calls 正确
```

例子：

```nova
let s : str = parse_name(token_text(tokens, pos));
return format_node(make_node(...));
```

---

### 18.2 Field access

确认：

```text
struct field access works
nested field access works
field access type inference works
enum member access works
```

例子：

```nova
point.x
line.start.x
TokenKind.Identifier
ParseNodeKind.FunctionDecl
```

---

### 18.3 Struct literals

确认：

```text
struct literal codegen works
field order stable
field expression expected type works
nested struct literal works when needed
```

例子：

```nova
Token { kind: TokenKind.Identifier, lexeme: name, file: file, line: line, column: column }
```

---

### 18.4 Enums

确认：

```text
enum declarations emit C enum
enum members use stable C names
enum equality works
enum field/member access codegen works
```

---

### 18.5 Vectors

确认：

```text
vec_new expected type works
vec_push type checks and emits correct helper
vec_get return type inference works
vec_set works if language supports it
vec_len returns int
vec<struct> works
vec<enum> works if needed
vec<str> maps to const char** element storage correctly
```

Nested vectors are not required unless current compiler tools need them.

---

### 18.6 String operations

Confirm:

```text
str + str emits str_concat
str_eq direct call works
str_contains / starts_with / ends_with direct call works
int_to_str works
large output construction can still use buf_*
```

---

### 18.7 Control flow

Confirm:

```text
if / else works
else if shape works
nested if works
while works
return works
no-return call in non-void function works
```

---

### 18.8 Imports

Confirm generated codegen can compile imported source:

```text
program with import
nested import
diamond import
library split files
```

---

## 19. Known non-goals for parity

Do not add these unless they are already part of current language and blocking self-host:

```text
closures
interfaces
methods
modules
pattern matching
generics beyond typed vec<T>
nested vec<T> unless already needed
heap object destructors
```

---

# Part D：Tests and CMake Integration

---

## 20. Add a self-host test

Add CTest entry for the driver:

```cmake
add_test(
    NAME self_host_stage_codegen
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/self_host.sh
)

set_tests_properties(
    self_host_stage_codegen
    PROPERTIES LABELS "selfhost;codegen"
)
```

If runtime is long, you can first keep it out of default CTest and document manual command:

```bash
scripts/self_host.sh
```

But by the end of Step 25, it should ideally be part of CTest.

---

## 21. Add focused codegen tests

For every parity bug fixed, add a small focused test.

Examples:

```text
tests/tools/codegen/nested_field_access.nv
tests/tools/codegen/enum_member_expr.nv
tests/tools/codegen/vec_of_struct.nv
tests/tools/codegen/no_return_codegen.nv
tests/tools/codegen/import_diamond_codegen/main.nv
```

Each test should include:

```text
input .nv
expected .out
CTest entry
```

---

## 22. Avoid only testing via self-host

Self-host driver is an integration test.

When it finds a bug, also add a focused unit-style regression test.

Example:

```text
self_host.sh fails on field access inside vec_get result
```

Add focused test:

```nova
struct Point { x: int; y: int; }

fn main() : void {
    let xs : vec<Point> = vec_new();
    vec_push(xs, Point { x: 1, y: 2 });
    print_int(vec_get(xs, 0).x);
}
```

Expected:

```text
1
```

---

# Part E：Optional Nova Build Driver

---

## 23. Optional `tools/nova_build.nv`

If time permits, start a Nova-written build driver prototype:

```text
tools/nova_build.nv
```

Possible behavior:

```bash
nova_build compile input.nv output
nova_build self-host
```

But this is optional.

The shell driver remains the source of truth for Step 25.

---

## 24. Why optional

A Nova build driver needs more runtime/process support if it wants to invoke compilers and C compiler directly.

Current runtime may not support:

```text
spawn process
check exit code
make directory
remove directory
```

So do not block Step 25 on a fully Nova-written build driver.

---

# Part F：Documentation

---

## 25. Update bootstrap docs

Update:

```text
docs/bootstrap.md
```

Add section:

```text
Self-hosted codegen workflow
```

Include:

```bash
scripts/self_host.sh
```

Explain stages:

```text
stage0 generated by C++ seed compiler
stage1 generated by stage0
stage2 generated by stage1
stage1/stage2 behavior compared on representative programs
```

---

## 26. Update limitations docs

Update:

```text
docs/limitations.md
```

Clarify:

```text
Nova is not fully self-hosting yet if C++ seed compiler is still needed for stage0
Nova codegen supports a tested subset sufficient for current tools
Generated C is intended for correctness/readability, not optimization
```

---

## 27. Update README

Add a short section:

```markdown
## Self-hosting smoke test

```bash
scripts/self_host.sh
```

This builds stage0, stage1, and stage2 Nova codegen and checks representative generated programs.
```

---

# Part G：Recommended Implementation Order

---

## 28. Step A：Write the shell driver skeleton

Create:

```text
scripts/self_host.sh
```

Implement:

```text
WORK setup
stage0 build
stage1 build
stage2 build
basic hello program check
```

Run manually first.

---

## 29. Step B：Add representative program matrix

Add programs one by one:

```text
hello
recursion
string_plus
string_ends_with
struct_basic
enum_basic
vec_basic
vec_str_basic
vec_struct_basic
import_basic
```

When one fails, fix codegen and add focused regression.

---

## 30. Step C：Add CTest integration

Once stable:

```text
add self_host_stage_codegen test
label as selfhost;codegen
```

If it is slow, keep label so you can run:

```bash
ctest --test-dir build -L selfhost --output-on-failure
```

---

## 31. Step D：Expand parity based on failures

Fix only real blockers.

Common likely blockers:

```text
CallExpr type inference
FieldAccessExpr type inference
StructLiteral expected type
vec_get expression base
enum member codegen
else-if branch shape
imported source codegen
```

---

## 32. Step E：Docs update

Update:

```text
docs/bootstrap.md
docs/limitations.md
README.md
```

---

## 33. Step F：Full regression

Run:

```bash
cmake --build build --parallel
ctest --test-dir build -j8 --output-on-failure
scripts/self_host.sh
```

If `scripts/self_host.sh` is included in CTest, then the separate manual command can still be run as a final smoke check.

---

# Part H：验收标准

---

## 34. 合格

```text
scripts/self_host.sh exists
stage0 nova_codegen builds successfully
stage1 nova_codegen builds successfully
stage2 nova_codegen builds successfully
stage1/stage2 compile at least hello + vec + struct representative programs
stage1/stage2 generated executables produce matching output
full CTest passes
```

---

## 35. 良好

```text
self_host.sh covers a representative program matrix
CTest includes self-host smoke test
focused regression tests added for every codegen parity bug fixed
imported source can be compiled by stage1/stage2 codegen
bootstrap docs updated
bootstrap_stage1.sh either still works or is superseded clearly by self_host.sh
```

---

## 36. 优秀

```text
stage1 and stage2 can both compile tools/nova_frontend.nv
stage1 and stage2 can both compile tools/nova_checker.nv
stage1 and stage2 can both compile tools/nova_codegen.nv
stage-generated frontend/checker/codegen run representative tests
self-host workflow is documented and reproducible from a clean build
```

---

## 37. 完成标志

Step 25 完成时，应该可以说：

```text
Nova has a repeatable self-hosted codegen workflow, and its Nova-written codegen has enough parity to build and validate stage1/stage2 tools on representative programs.
```

至少这些成立：

```text
C++ seed -> stage0 nova_codegen
stage0 -> stage1 nova_codegen
stage1 -> stage2 nova_codegen
stage1/stage2 compile representative Nova programs
stage1/stage2 program outputs match
all normal tests pass
self-host workflow is documented
```

---

## 38. 后续关联

Step 26 将转向：

```text
VS Code Developer Tooling
```

包括：

```text
syntax highlighting polish
snippets
tasks/problem matcher if useful
editor setup docs
```

Step 27 将作为 Phase 2 milestone：

```text
self-hosting status summary
demo commands
final docs cleanup
known limitations
release-style validation
```
