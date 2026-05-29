# Nova Phase 2 Self-Hosting Plan

## 0. 文档目的

本文档用于记录 Nova 项目 Phase 2 的最终规划。

Phase 1 已经完成了一个可工作的 bootstrap prototype：

```text
C++ compiler compiles Nova tools.
Nova frontend checks Nova source.
Nova codegen generates C.
Generated C compiles and runs.
Import-based Nova libraries work.
CTest passes.
```

Phase 2 的目标不是继续随意增加语言功能，而是围绕一个核心目标推进：

> **让 Nova self-hosting 变成可重复、可测试、可展示、可继续开发的工具链能力。**

如果后续对话中上下文丢失，可以把本文档重新提供给 assistant，作为 Phase 2 的规划基准。

---

## 1. Phase 1 当前基线

截至 Phase 1 结束，Nova 已具备以下能力。

### 1.1 C++ compiler

C++ 实现的主编译器已经支持：

```text
lexer
parser
semantic analyzer
C codegen
runtime integration
import/source_loader
```

主入口：

```text
nova_lex
nova_parse
nova_sema
nova_compile
```

其中 `nova_compile` 可以把 Nova 源码编译成 C。

---

### 1.2 Nova libraries

Nova 已经有 import-based library layout：

```text
lib/token.nv
lib/tokenizer.nv
lib/parse_tree.nv
lib/parser.nv
lib/checker.nv
lib/codegen_c.nv
lib/source_loader.nv
lib/diagnostics.nv
```

这些库组成了 Nova-written frontend 和 prototype backend。

---

### 1.3 Nova tools

当前主要 Nova-written tools：

```text
tools/nova_tokenizer.nv
tools/nova_expr_parser.nv
tools/nova_checker.nv
tools/nova_frontend.nv
tools/nova_codegen.nv
```

其中：

```text
nova_frontend:
  tokens / parse / check

nova_codegen:
  Nova source -> C source
```

---

### 1.4 Testing

当前全量测试已经通过：

```text
C++ compiler tests
Nova tool tests
import tests
frontend smoke tests
Nova-written codegen tests
```

Phase 1 完成时，全量 CTest 状态为：

```text
213/213 passed
```

---

## 2. Phase 2 总目标

Phase 2 的目标是：

> **Verified Self-Hosting Toolchain**

也就是让 Nova 不只是“可以生成 C”，而是能够完成可重复的 stage-based self-hosting。

目标链路：

```text
C++ seed compiler
  -> builds stage0 nova_codegen

stage0 nova_codegen
  -> builds stage1 nova_codegen

stage1 nova_codegen
  -> builds stage2 nova_codegen

stage1 / stage2
  -> compile representative Nova programs
  -> behavior matches
```

进一步目标：

```text
stage-generated nova_frontend can check Nova tools
stage-generated nova_checker can check representative programs
stage-generated nova_codegen can compile representative programs
```

---

## 3. Phase 2 非目标

Phase 2 不追求一次性完成所有语言和工具链能力。

不做：

```text
完整 industrial module system
完整 C++ backend retirement
完整 source-map diagnostics v2
完整 LSP
formatter
package manager
optimizer / IR
ownership / lifetime model
full standard library expansion
```

这些属于 Phase 3 或之后。

Phase 2 关注：

```text
self-hosting
codegen parity for compiler source
source-aware diagnostics v1
import robustness
runtime/builtin cleanup
developer tooling baseline
```

---

## 4. Self-hosting 等级定义

### Level 1：Nova codegen 编译普通 Nova 程序

```text
nova_codegen input.nv output.c
cc output.c runtime/nova_runtime.c -I runtime -o program
```

Phase 1 已完成。

---

### Level 2：Nova codegen 编译 nova_codegen 自己

```text
C++ seed compiler
  -> nova_codegen_stage0

nova_codegen_stage0
  -> nova_codegen_stage1

nova_codegen_stage1
  -> nova_codegen_stage2
```

Phase 2 的核心目标是把 Level 2 变成稳定 milestone。

---

### Level 3：日常开发使用 Nova-generated toolchain

```text
C++ compiler only acts as seed.
Nova-generated stage tools are used for regular frontend/codegen checks.
```

Phase 2 会接近 Level 3，但不要求完全替代 C++ compiler。

---

## 5. Phase 2 Step Overview

最终 Phase 2 规划为 9 个 step：

```text
Step 21  Self-hosting codegen parity I
Step 22  Self-hosting codegen parity II
Step 23  Source-aware diagnostics v1
Step 24  Import system v2
Step 25  Runtime / builtin / stdlib cleanup
Step 26  Self-hosted build driver
Step 27  Nova codegen parity expansion
Step 28  VS Code developer tooling
Step 29  Phase 2 self-hosting milestone
```

每个 step 都必须有明确功能贡献。  
测试是每个 step 的验收标准，不作为单独 step 存在。

---

# Step 21 — Self-hosting codegen parity I

## 目标

让：

```text
stage0 nova_codegen
  -> compile tools/nova_codegen.nv
  -> produce nova_codegen_stage1
```

当前已知 blocker：

```text
CodegenError: unsupported statement type in codegen: Condition
```

## 贡献

补齐 Nova codegen 对当前 compiler source 必需的 parse-tree shape 支持。

重点包括：

```text
Condition
ThenBranch
ElseBranch
IfStmt layout
WhileStmt layout
ReturnStmt complex expression
nested BlockStmt
```

当前 ParseNode 结构大致为：

```text
IfStmt
  Condition
    <expr>
  Then
    Block
  Else
    Block

WhileStmt
  Condition
    <expr>
  Block
```

codegen 不应该把 `Condition`、`ThenBranch`、`ElseBranch` 当普通 statement 处理。

## 验收标准

```text
stage0 nova_codegen 能成功编译 tools/nova_codegen.nv
生成 nova_codegen_stage1.c
stage1 executable 能成功编译
```

---

# Step 22 — Self-hosting codegen parity II

## 目标

让 stage-generated tools 不只是能编译出来，还能实际运行。

重点目标：

```text
tools/nova_codegen.nv
tools/nova_frontend.nv
tools/nova_checker.nv
tools/nova_expr_parser.nv
```

## 贡献

补齐当前 Nova compiler libraries 实际需要、但 Nova codegen 还不稳定的能力：

```text
struct literal field expected type
vec_new expected type in nested contexts
vec expression whose first argument is not a simple identifier
function-call expression type inference
return struct / return vec / return function call
scope lookup for generated code
compiler library 中实际出现的表达式形态
```

## 验收标准

stage0 能生成并编译：

```text
nova_codegen_stage1
nova_frontend_stage1
nova_checker_stage1
nova_expr_parser_stage1
```

这些 generated tools 能跑代表性输入：

```text
nova_frontend_stage1 check tools/nova_checker.nv
nova_checker_stage1 tests/tools/typecheck/positive/vec_basic.nv
nova_codegen_stage1 tests/tools/codegen/hello.nv
```

---

# Step 23 — Source-aware diagnostics v1

## 目标

解决 import 后错误位置不准的问题。

当前 Phase 1 限制：

```text
diagnostics may use flattened source locations
```

Phase 2 应该提供第一版 source-aware diagnostics。

## 贡献

把位置从：

```text
line:column
```

升级为：

```text
file:line:column
```

可能涉及：

```text
Token 增加 source file 信息
ParseNode 增加 source file 信息
C++ source_loader 维护 import source info
Nova source_loader 尽量保留 file 信息
parse_error / checker_error / codegen_error 支持 file 输出
```

不要求完整 source map v2，但 imported file 内的错误应能指回 imported file。

## 验收标准

例如：

```nova
// main.nv
import "bad.nv";
```

```nova
// bad.nv
fn main() : void {
    let x : int = "hello";
}
```

应报类似：

```text
bad.nv:2:19: CheckerError: type error in variable declaration: expected 'int', got 'str'
```

---

# Step 24 — Import system v2

## 目标

把 Step 18 的 textual include 做得更可靠。

## 贡献

加强 import system，但仍不做完整 module system。

支持改进：

```text
path lexical normalization
./ 和 ../ 处理
重复路径指向同一文件时 include-once
更好的 cyclic import error
import root / project root 规则
C++ source_loader 和 Nova source_loader 行为对齐
```

仍然不做：

```text
namespace
export/private
alias
package manager
separate compilation
```

## 验收标准

这些场景稳定：

```text
import "./a.nv"
import "dir/../a.nv"
diamond import
cyclic import
missing import
Nova frontend check imported source
Nova codegen compile imported source
```

---

# Step 25 — Runtime / builtin / stdlib cleanup

## 目标

统一 runtime、checker builtin 表、codegen builtin 行为。

当前已知状态：

```text
runtime 中仍保留 str_vec_* legacy
checker 不把 str_vec_* 当 builtin
codegen 对 runtime call 基本 direct call
vec_* 有 special-case
```

## 贡献

整理：

```text
runtime/nova_runtime.h
runtime/nova_runtime.c
C++ sema builtin table
Nova checker builtin table
Nova codegen runtime call behavior
docs/standard_library.md
```

明确：

```text
哪些 runtime API 是 user-facing
哪些是 compiler-internal
哪些是 legacy
哪些应该删除
```

如果 `str_vec_*` 不再被主线使用，可以删除。  
如果 legacy tests 仍依赖，则保留并记录。

## 验收标准

```text
runtime builtin list 一致
C++ sema / Nova checker / Nova codegen 行为一致
deprecated API 状态明确
ctest 全过
docs/standard_library.md 存在
```

---

# Step 26 — Self-hosted build driver

## 目标

新增正式 self-host build workflow。

这不是“测试 step”，而是一个实际工具能力。

可以是：

```text
scripts/build_self_host.sh
```

或：

```text
cmake target: nova_self_host
```

## 贡献

该 driver 负责：

```text
1. C++ seed compiler builds stage0
2. stage0 builds stage1
3. stage1 builds stage2
4. stage1/stage2 compile representative programs
5. compare behavior
```

## 验收标准

用户可以运行：

```bash
scripts/build_self_host.sh
```

得到类似：

```text
stage0 OK
stage1 OK
stage2 OK
stage1/stage2 behavior match
self-host verification OK
```

并且可以接入 CTest：

```bash
ctest --test-dir build -L bootstrap
```

---

# Step 27 — Nova codegen parity expansion

## 目标

让 Nova codegen 更接近 C++ codegen，减少 prototype 限制。

## 贡献

选择一批 C++ compiler codegen tests，迁移或镜像到 Nova codegen tests。

重点补：

```text
file I/O generated programs
buffer-heavy programs
argument runtime functions
more string runtime functions
more struct/enum/vec combinations
examples/positive programs
```

如果某些特性不支持：

```text
补支持
或明确写入 limitations
```

## 验收标准

更多 C++ codegen 正例也能被 Nova codegen 通过：

```text
tests/codegen/positive/*
```

对应或镜像到：

```text
tests/tools/codegen/*
```

并且：

```text
examples/positive 大部分能被 Nova codegen 编译运行
```

---

# Step 28 — VS Code developer tooling

## 目标

把 VS Code extension 从 syntax-only 升级到 lightweight language support。

仍不做完整 LSP。

## 贡献

可实现：

```text
DocumentSymbolProvider
basic HoverProvider
basic DefinitionProvider
```

先只支持单文件。

支持：

```text
Outline 显示 fn / struct / enum
hover function signature
hover struct fields
go to definition for same-file fn / struct / enum
```

不做：

```text
跨 import 查找
semantic diagnostics
rename
references
formatter
完整 LSP
```

## 验收标准

在 VS Code 中打开 `.nv` 文件：

```text
Outline 能看到 functions / structs / enums
Hover 能显示简单信息
Go to definition 能跳同文件定义
Syntax highlighting 继续正常
```

---

# Step 29 — Phase 2 self-hosting milestone

## 目标

Phase 2 收尾，不加大功能。

## 贡献

把 self-hosting 状态文档化和固定化。

新增：

```text
docs/self_hosting.md
docs/self_hosting_status.md
```

记录：

```text
如何从 C++ seed 构建 stage0
如何从 stage0 构建 stage1
如何从 stage1 构建 stage2
stage1/stage2 验证范围
还有哪些东西依赖 C++
哪些是 Phase 3 目标
```

## 最终验收标准

```text
1. C++ seed compiler builds stage0 nova_codegen.
2. stage0 builds stage1 nova_codegen.
3. stage1 builds stage2 nova_codegen.
4. stage1/stage2 behavior matches on representative tests.
5. Nova-generated frontend/checker/codegen run smoke tests.
6. import/source diagnostics are usable.
7. runtime/builtin cleanup complete or documented.
8. docs explain the self-host workflow.
```

---

## 6. Phase 2 不再拆分成纯测试步骤

Phase 2 的原则：

```text
每个 step 必须交付新能力。
测试是验收标准，不单独作为 step。
```

因此：

```text
stage1/stage2 comparison
frontend smoke tests
CTest integration
```

都作为功能 step 的验收，而不是单独 step。

---

## 7. Phase 3 保留内容

Phase 2 完成后，Phase 3 可以关注更大的语言/工具链能力：

```text
true module system with namespaces
complete Nova codegen parity / retiring C++ backend
source-mapped diagnostics v2
full LSP
formatter
package/build system
ownership / memory model
optimizer or IR
standard library expansion
```

Phase 2 负责：

```text
self-hosting 可重复、可测试、可用
```

Phase 3 负责：

```text
把 Nova 从 educational compiler 推向更完整的语言工具链
```

---

## 8. 当前 Phase 2 起点

根据 Phase 1 结束时的 readiness report：

```text
Build: OK
Full CTest: 213/213 passed
Docs: OK
Import system: OK, Nova-side canonicalization weak
Nova frontend: OK
Nova codegen: stage0 works for normal programs
Self-host: stage0 -> stage1 currently blocked
Main blocker: CodegenError: unsupported statement type in codegen: Condition
```

因此 Phase 2 应该从：

```text
Step 21: Self-hosting codegen parity I
```

开始，第一目标是修复 `Condition` blocker，让 stage0 成功生成 stage1。
