# Nova Project — Step 20
## 题目：Bootstrap Milestone、项目整理与工具链收尾

---

## 1. 当前进度

到 Step 19 结束时，Nova 已经完成了 Phase 1 的主要技术目标：

```text
C++ compiler:
  lexer
  parser
  semantic analyzer
  C codegen
  runtime
  import/source_loader

Nova libraries:
  lib/token.nv
  lib/tokenizer.nv
  lib/parse_tree.nv
  lib/parser.nv
  lib/checker.nv
  lib/codegen_c.nv
  lib/source_loader.nv
  lib/diagnostics.nv

Nova tools:
  tools/nova_tokenizer.nv
  tools/nova_expr_parser.nv
  tools/nova_checker.nv
  tools/nova_frontend.nv
  tools/nova_codegen.nv

Testing:
  C++ compiler tests
  Nova tool tests
  import tests
  frontend smoke tests
  Nova-written codegen tests
```

现在项目已经可以展示：

```text
C++ nova_compile 编译 Nova-written tools
Nova frontend 检查 Nova source
Nova codegen 生成 C
C compiler 编译 generated C
generated executable 运行通过
```

Step 20 的目标不是继续扩语言功能，而是把 Phase 1 正式收口。

---

## 2. 本次作业目标

Step 20 的主题是：

> **整理项目、补齐文档、稳定测试、明确限制，并形成一个可以展示的 bootstrap milestone。**

本阶段主要包含：

```text
1. 全量测试收口
2. 项目结构整理
3. .gitignore 清理
4. README / docs 补齐
5. bootstrap demo
6. VS Code syntax extension 小更新
7. deprecated runtime 清理或记录
8. assignment 历史说明
9. Phase 1 milestone 标记
```

---

## 3. 本阶段不做什么

Step 20 不做新的大功能。

不做：

```text
完整 module system
source map diagnostics
完整 LSP
hover / go-to-definition
Nova codegen 完整替换 C++ codegen
完整 self-host compiler
大规模测试目录迁移
逐个重写 Step 1-19 assignment
```

这些留给 Phase 2。

---

## 4. specification.md 是否改名

当前：

```text
docs/specification.md
```

实际上是 Step 1 的早期语言规范，而不是当前完整规范。

建议在 Step 20 中改名，避免误导。

### 推荐方案 A

改成：

```text
docs/step1_language_spec.md
```

优点：

```text
简单
准确
不需要移动目录
```

### 推荐方案 B

移动到：

```text
docs/assignments/Nova_Step1_Language_Specification.md
```

优点：

```text
和 assignments 历史记录放在一起
```

缺点：

```text
如果 README 想引用语言规范，路径略长
```

### 推荐方案 C

保留 `docs/specification.md`，但新增顶部说明：

```markdown
# Nova Step 1 Language Specification

This document records the initial Step 1 language specification.
It is not the complete current language reference.
For current architecture and limitations, see:
- docs/architecture.md
- docs/limitations.md
```

最推荐：

```text
把 docs/specification.md 改名为 docs/step1_language_spec.md
```

然后未来如果要写真正完整规范，再新建：

```text
docs/language_reference.md
```

---

## 5. 项目目录整理

当前推荐保留的顶层结构：

```text
Nova/
  CMakeLists.txt
  cmake/
  docs/
  examples/
  include/
  lib/
  runtime/
  src/
  tests/
  tools/
  README.md
  .gitignore
```

这套结构已经比较清楚，不建议 Step 20 做大规模迁移。

---

## 6. docs 目录建议

新增或更新：

```text
README.md
docs/architecture.md
docs/bootstrap.md
docs/testing.md
docs/limitations.md
docs/assignments/README.md
docs/assignments/ERRATA.md
```

如果 `docs/specification.md` 改名，则新增：

```text
docs/step1_language_spec.md
```

未来完整语言规范可以叫：

```text
docs/language_reference.md
```

但 Step 20 不要求写完整 language reference。

---

## 7. README.md 内容建议

`README.md` 应该回答：

```text
Nova 是什么
当前 milestone 是什么
怎么 build
怎么 run tests
怎么运行 C++ compiler
怎么运行 Nova-written frontend
怎么运行 Nova-written codegen
当前限制在哪里
```

建议结构：

```markdown
# Nova

Nova is a small educational programming language and compiler project.

## Current milestone

Phase 1 bootstrap prototype:
- C++ compiler compiles Nova tools.
- Nova frontend tokenizes, parses, and checks Nova source.
- Nova codegen generates C for a useful subset.
- Import-based Nova libraries are supported.

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Test

```bash
ctest --test-dir build
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
```

---

## 8. docs/architecture.md 内容建议

说明当前架构：

```text
C++ compiler
  src/
  include/
  runtime/

Nova libraries
  lib/token.nv
  lib/tokenizer.nv
  lib/parse_tree.nv
  lib/parser.nv
  lib/checker.nv
  lib/codegen_c.nv
  lib/source_loader.nv
  lib/diagnostics.nv

Nova tools
  tools/nova_frontend.nv
  tools/nova_codegen.nv
  tools/nova_checker.nv
  tools/nova_expr_parser.nv
  tools/nova_tokenizer.nv

Import/include system
  import "path.nv";
  textual include
  include-once
  cycle detection
```

建议写明 frontend pipeline：

```text
source
  -> load_source_with_imports
  -> tokenize
  -> parse_program_tree
  -> check_program
```

以及 codegen pipeline：

```text
source
  -> load_source_with_imports
  -> tokenize
  -> parse_program_tree
  -> check_program
  -> gen_c_program
  -> C compiler
  -> executable
```

---

## 9. docs/bootstrap.md 内容建议

这个文件放完整 demo。

### Demo 1: C++ compiler compiles a Nova program

```bash
cmake -S . -B build
cmake --build build

./build/nova_compile examples/positive/helloworld.nv /tmp/hello.c
cc /tmp/hello.c runtime/nova_runtime.c -I runtime -o /tmp/hello
/tmp/hello
```

### Demo 2: C++ compiler compiles Nova frontend

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

### Demo 3: Nova codegen generates C

```bash
./build/nova_compile tools/nova_codegen.nv /tmp/nova_codegen.c
cc /tmp/nova_codegen.c runtime/nova_runtime.c -I runtime -o /tmp/nova_codegen

/tmp/nova_codegen tests/tools/codegen/hello.nv /tmp/generated_hello.c
cc /tmp/generated_hello.c runtime/nova_runtime.c -I runtime -o /tmp/generated_hello
/tmp/generated_hello
```

Expected output:

```text
hello
```

---

## 10. docs/testing.md 内容建议

解释测试分类。

```text
tests/lexer/
  C++ lexer tests

tests/parser/
  C++ parser tests

tests/sema/
  C++ semantic analyzer tests

tests/codegen/
  C++ compiler codegen tests

tests/import/
  C++ import/source_loader tests

tests/tools/tokenizer/
  Nova tokenizer tool tests

tests/tools/expr_parser/
  Nova expression parser tool tests

tests/tools/checker/
  Nova declaration checker tests

tests/tools/typecheck/
  Nova expression type checker tests

tests/tools/frontend/
  Nova frontend driver tests

tests/tools/frontend_import/
  Nova frontend import tests

tests/tools/codegen/
  Nova-written codegen tool tests
```

推荐写常用命令：

```bash
ctest --test-dir build
ctest --test-dir build -L nova_tool
ctest --test-dir build -R nova_tool_codegen
ctest --test-dir build -R import
```

---

## 11. docs/limitations.md 内容建议

记录当前限制：

```text
Import system
  import is textual include, not a real module system.
  all declarations share one global namespace.
  no export/private/alias/package search path.

Diagnostics
  imported source diagnostics may use flattened source locations.
  no source map yet.

Nova codegen
  prototype backend, not a replacement for C++ codegen.
  supports useful subset only.
  no complete parity guarantee.

Vectors
  nested vec is not supported.
  generated vectors currently do not free allocated memory.

Memory management
  runtime allocation is checked in generated vec helpers,
  but no ownership/lifetime model exists.

Tooling
  VS Code extension is syntax highlighting only.
  no hover, go-to-definition, or diagnostics.

Assignments
  historical assignments record the development process.
  final implementation may differ from earlier step goals.
```

---

## 12. docs/assignments/README.md

建议内容：

```markdown
# Nova Assignments

This directory contains the step-by-step assignment documents used during the development of Nova.

These documents are historical development records. Some early goals were revised, merged, or replaced in later steps.

The final project state is defined by:
- README.md
- docs/architecture.md
- docs/testing.md
- docs/limitations.md
- the current test suite

For known assignment-level changes, see ERRATA.md.
```

---

## 13. docs/assignments/ERRATA.md

不要逐个重写 Step 1-19，只记录重要变化。

建议内容：

```markdown
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
```

---

## 14. .gitignore

建议更新为：

```gitignore
# Build directories
/build/
/build-*/
/cmake-build-*/
/out/

# CMake / CTest generated files
/Testing/
CMakeFiles/
CMakeCache.txt
cmake_install.cmake
CTestTestfile.cmake
DartConfiguration.tcl
install_manifest.txt

# Compilation database
/compile_commands.json

# Generated binaries and object files
*.o
*.obj
*.a
*.so
*.dylib
*.dll
*.exe
a.out

# Temporary generated C files
*.generated.c
*.tmp.c
/tmp_*.c

# Local run outputs
tokens.txt
*.tmp
*.log

# Editor / OS files
/.vscode/
.idea/
.DS_Store
Thumbs.db

# Python/cache helper files
__pycache__/
*.pyc

# Local scratch
/scratch/
/tmp/
```

注意：

```text
不要 ignore *.out
不要 ignore *.err
不要 ignore *.tok
不要 ignore *.check
```

这些是测试 golden 文件。

也不要 ignore：

```text
tools/vscode-nova-syntax/
```

这是项目内容，不是本地编辑器配置。

---

## 15. VS Code syntax extension 更新

Step 20 可以更新：

```text
tools/vscode-nova-syntax/
  package.json
  language-configuration.json
  syntaxes/nova.tmLanguage.json
```

只做 syntax highlighting。

### 需要确认高亮的关键字

```text
import
fn
let
return
if
else
while
struct
enum
true
false
void
int
bool
str
vec
```

### runtime / intrinsic names

```text
print_int
print_str
str_eq
str_concat
str_len
str_get
str_slice
str_starts_with
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
vec_new
vec_push
vec_get
vec_set
vec_len
```

### syntax examples

```nova
import "../lib/tokenizer.nv";

enum Color {
    Red;
}

struct Point {
    x: int;
    y: int;
}

fn main() : void {
    let c : Color = Color.Red;
    let p : Point = Point { x: 1, y: 2 };
    let xs : vec<int> = vec_new();
    vec_push(xs, p.x);
    return;
}
```

不做：

```text
hover
go-to-definition
semantic diagnostics
formatting
LSP
```

---

## 16. Deprecated runtime cleanup

Step 20 可以处理，但要保守。

推荐流程：

```bash
grep -R "deprecated_function_name" .
```

如果没有主线使用：

```text
删除 runtime deprecated API
删除或迁移对应旧测试
ctest 全量跑一遍
```

如果还有 legacy tool 使用：

```text
先保留
在 docs/limitations.md 中说明
不要加 warning
```

不要加 runtime warning，因为会污染 stderr，影响测试。

---

## 17. Bootstrap demo script 可选项

可以新增：

```text
scripts/bootstrap_demo.sh
```

如果你不想新增 scripts 目录，也可以保留在 `docs/bootstrap.md` 里。

建议脚本内容：

```bash
#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build
cmake --build build

./build/nova_compile tools/nova_frontend.nv /tmp/nova_frontend.c
cc /tmp/nova_frontend.c runtime/nova_runtime.c -I runtime -o /tmp/nova_frontend
/tmp/nova_frontend check tools/nova_checker.nv /tmp/nova_checker.check
cat /tmp/nova_checker.check

./build/nova_compile tools/nova_codegen.nv /tmp/nova_codegen.c
cc /tmp/nova_codegen.c runtime/nova_runtime.c -I runtime -o /tmp/nova_codegen
/tmp/nova_codegen tests/tools/codegen/hello.nv /tmp/hello.generated.c
cc /tmp/hello.generated.c runtime/nova_runtime.c -I runtime -o /tmp/hello.generated
/tmp/hello.generated
```

---

## 18. Full test sweep

Step 20 完成前必须跑：

```bash
ctest --test-dir build
```

建议额外跑：

```bash
ctest --test-dir build -L nova_tool
ctest --test-dir build -R nova_tool_codegen
ctest --test-dir build -R import
```

如果更新了 VS Code syntax extension，不需要 CTest，但建议手动在 VS Code 中打开 `.nv` 文件确认：

```text
import
enum
struct
vec
runtime builtins
```

都有高亮。

---

## 19. 推荐实现顺序

### Step A: Rename early specification

```text
docs/specification.md -> docs/step1_language_spec.md
```

并更新引用。

### Step B: Update .gitignore

确保 build artifacts 被忽略，golden files 不被忽略。

### Step C: Add docs

新增：

```text
README.md
docs/architecture.md
docs/bootstrap.md
docs/testing.md
docs/limitations.md
docs/assignments/README.md
docs/assignments/ERRATA.md
```

### Step D: Update VS Code syntax extension

补：

```text
import
enum
struct
vec
runtime builtins
```

### Step E: Deprecated runtime decision

删除或记录，不加 warning。

### Step F: Full test sweep

跑：

```bash
ctest --test-dir build
```

### Step G: Final milestone tag

在 README 里写：

```text
Phase 1 bootstrap prototype complete.
```

如果使用 git，可以考虑：

```bash
git tag phase1-bootstrap
```

---

## 20. 验收标准

### 合格

```text
ctest 全量通过
README.md 存在
docs/bootstrap.md 存在
docs/testing.md 存在
.gitignore 更新
```

### 良好

```text
docs/architecture.md 存在
docs/limitations.md 存在
VS Code syntax 支持 import / enum / struct / vec / runtime builtins
docs/specification.md 已改名或说明清楚
assignments README / ERRATA 存在
```

### 优秀

```text
bootstrap demo 可直接复制运行
deprecated runtime 状态明确
项目目录结构干净
Phase 1 milestone 清楚可展示
```

---

## 21. 完成标志

Step 20 完成时，项目应该能清楚展示：

```text
C++ compiler compiles Nova tools.
Nova frontend checks Nova tools.
Nova codegen generates and runs C programs.
Import-based Nova libraries work.
CTest passes.
Docs explain how to build, test, and demo the project.
```

这标志着：

```text
Nova Phase 1 Bootstrap Prototype Complete
```

之后可以进入 Phase 2，例如：

```text
true module system
source-mapped diagnostics
Nova codegen parity
self-host pipeline experiment
VS Code LSP
memory/runtime cleanup
standard library design
```
