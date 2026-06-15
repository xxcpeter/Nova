# Nova Project — Step 24
## 题目：Import v2 Finalization + Runtime / Builtin / Stdlib Cleanup

---

## 1. 当前起点

Step 23 已经完成：

```text
C++ compiler diagnostics source-aware
Nova-written frontend diagnostics source-aware
import expansion 保留原始 file:line:column
Token / ParseNode / diagnostics 携带 source file
missing / cyclic / malformed import 有 source-aware diagnostics
C++ source_loader 和 Nova source_loader 已经有 LoadedSource / line map
str_concat 基本迁移为 str + str
```

因此，原计划中 Step 24 的大部分 import source-location 工作已经在 Step 23 中完成。

本 Step 24 合并原来的：

```text
Step 24 — Import System v2
Step 25 — Runtime / builtin / stdlib cleanup
```

新的 Step 24 定位为一次 stabilization step：

```text
Part A:
  Import v2 finalization

Part B:
  Runtime / builtin / stdlib cleanup
```

---

## 2. 本次作业目标

Step 24 的目标是：

> **收尾 import system，并统一 runtime / builtin / stdlib 的语言表面。**

具体目标：

```text
Import side:
  path lexical normalization
  ./ 和 ../ 等价路径处理
  include-once 行为稳定
  diamond import 不重复展开
  cyclic / missing / malformed import diagnostics 稳定
  C++ source_loader 和 Nova source_loader 行为对齐

Runtime / builtin side:
  明确 runtime API 清单
  明确哪些 builtin 是 user-facing
  明确哪些 runtime function 是 compiler-internal
  明确哪些 API 是 legacy
  对齐 C++ sema builtin table
  对齐 Nova checker builtin table
  对齐 Nova codegen runtime call behavior
  增加 docs/standard_library.md
```

完成后，Nova 的基础设施应该更稳定，后续 self-hosted build driver 可以建立在更清晰的 import/runtime 边界上。

---

## 3. 本阶段定位

Step 24 是 cleanup + stabilization，不是新语言特性扩张。

它应该解决：

```text
import path 行为不够系统化
C++ / Nova source_loader 行为可能不完全一致
runtime 中存在历史遗留 API
checker / sema / codegen 对 builtin 的认识可能不一致
缺少 standard library / runtime API 文档
```

它不要求解决：

```text
真正 module system
namespace
export/private
import alias
package manager
separate compilation
incremental compilation
ownership / lifetime / destructor
完整 runtime memory management
```

---

## 4. 本阶段不做什么

Step 24 不做：

```text
module Foo;
export fn ...
private fn ...
import "x" as y
qualified names like math.add
package search path
compiled module cache
LSP support
runtime garbage collection
ownership checker
```

Nova 仍然使用 textual include-style import。

---

# Part A：Import v2 Finalization

---

## 5. Import 当前语义

当前 import 仍然是 textual include-style import：

```nova
import "relative/path.nv";
```

语义是：

```text
imported file 的内容被展开到 import 位置
同一个文件应该只展开一次
所有 declarations 仍进入同一个 global namespace
```

这不是 module system。

---

## 6. 三种路径概念必须保持清楚

source_loader 中应该继续区分：

```text
import_spec:
  源码里写的字符串
  例如 "a.nv"、"./a.nv"、"dir/../a.nv"
  用于 import error message

canonical_key:
  normalize 后的唯一 key
  用于 visited / stack / include-once / cycle detection

display_path:
  用于 source diagnostics
  例如 tests/import/positive/nested_relative/lib/a.nv
```

不要把这三者混用。

---

## 7. Import side 要确认的行为

Step 24 不一定需要大改实现，但必须用测试确认这些行为：

```text
import "a.nv" 和 import "./a.nv" 指向同一个文件
import "dir/a.nv" 和 import "dir/../dir/a.nv" 指向同一个文件
diamond import 不重复展开
nested relative import 基于当前文件目录解析
import in middle of file 不破坏 LoadedSource.text / line map 顺序
missing import 报在 import statement
cyclic import 报在触发 cycle 的 import statement
malformed import 报在 malformed import statement
imported file 内部 parse/check/codegen error 仍指向 imported file 本地位置
```

---

## 8. C++ source_loader 要求

C++ side 应该满足：

```text
visited 使用 canonical key
stack 使用 canonical key
display path 稳定为相对 project root
import error message 保留 import_spec
LoadedSource.content 和 line_mapping 顺序一致
```

如果已有这些 helper，检查并补测试即可：

```cpp
canonical_key_for(path)
make_display_path(path, project_root)
find_project_root()
```

如果没有，建议整理出来。

---

## 9. Nova source_loader 要求

Nova side 应该满足：

```text
path_join 后进行 lexical normalization
visited 使用 normalize_path(path)
stack 使用 normalize_path(path)
display_path_for 使用 normalized path
ImportContext 中区分 import_spec / import_file / import_line / import_column
递归 import 时创建 child_context，而不是覆盖 parent context 的 import-site 字段
LoadedSource.text 和 LoadedSource.lines 同步 append
```

推荐 helper：

```nova
fn normalize_path(path: str) : str
fn path_join(base: str, child: str) : str
fn path_dirname(path: str) : str
fn display_path_for(path: str, project_root: str) : str
fn infer_project_root(path: str) : str
```

---

## 10. Import tests

### 10.1 Duplicate same file

目录：

```text
tests/import/positive/duplicate_same_file/
  main.nv
  a.nv
  main.out
```

`main.nv`：

```nova
import "a.nv";
import "./a.nv";

fn main() : void {
    print_str(message());
}
```

`a.nv`：

```nova
fn message() : str {
    return "ok";
}
```

期望：

```text
ok
```

目的：

```text
确认 a.nv 和 ./a.nv 不会重复展开
```

---

### 10.2 Duplicate parent path

目录：

```text
tests/import/positive/duplicate_parent_path/
  main.nv
  dir/a.nv
  main.out
```

`main.nv`：

```nova
import "dir/a.nv";
import "dir/../dir/a.nv";

fn main() : void {
    print_str(message());
}
```

`dir/a.nv`：

```nova
fn message() : str {
    return "ok";
}
```

期望：

```text
ok
```

---

### 10.3 Diamond import

目录：

```text
tests/import/positive/diamond/
  main.nv
  a.nv
  b.nv
  shared.nv
  main.out
```

`main.nv`：

```nova
import "a.nv";
import "b.nv";

fn main() : void {
    print_str(a() + ":" + b());
}
```

`a.nv`：

```nova
import "shared.nv";

fn a() : str {
    return shared();
}
```

`b.nv`：

```nova
import "./shared.nv";

fn b() : str {
    return shared();
}
```

`shared.nv`：

```nova
fn shared() : str {
    return "shared";
}
```

期望：

```text
shared:shared
```

目的：

```text
确认 diamond import 不会重复展开 shared.nv 导致 duplicate function
```

---

### 10.4 Nested relative import

目录：

```text
tests/import/positive/nested_relative/
  main.nv
  lib/a.nv
  lib/b.nv
  main.out
```

`main.nv`：

```nova
import "lib/a.nv";

fn main() : void {
    print_str(a());
}
```

`lib/a.nv`：

```nova
import "b.nv";

fn a() : str {
    return b();
}
```

`lib/b.nv`：

```nova
fn b() : str {
    return "nested";
}
```

期望：

```text
nested
```

---

### 10.5 Import in middle

目录：

```text
tests/import/positive/import_in_middle/
  main.nv
  helper.nv
  main.out
```

`main.nv`：

```nova
fn before() : str {
    return "before";
}

import "helper.nv";

fn main() : void {
    print_str(before() + ":" + helper());
}
```

`helper.nv`：

```nova
fn helper() : str {
    return "helper";
}
```

期望：

```text
before:helper
```

目的：

```text
防止 LoadedSource.text 和 line map 顺序错位
```

---

### 10.6 Import negative tests

确认这些错误都有稳定位置：

```text
missing import
cyclic import
malformed import
```

示例 missing import：

```nova
import "missing.nv";

fn main() : void {
    return;
}
```

期望包含：

```text
main.nv:1:1: ImportError: cannot open import 'missing.nv'
```

示例 cycle：

```nova
// main.nv
import "a.nv";

fn main() : void {
    return;
}
```

```nova
// a.nv
import "main.nv";
```

期望包含：

```text
a.nv:1:1: ImportError: cyclic import involving 'main.nv'
```

---

## 11. Nova tool import tests

除了 C++ compiler import tests，也要确认 Nova tools 能处理 imported source：

```text
Nova frontend check imported source
Nova checker check imported source
Nova codegen compile imported source
```

最低建议覆盖：

```text
tests/tools/frontend_import/positive/duplicate_same_file
tests/tools/frontend_import/positive/nested_relative
tests/tools/frontend_import/negative/missing_location
tests/tools/frontend_import/negative/cycle_location
```

如果已有类似测试，可以复用并补缺口。

---

# Part B：Runtime / Builtin / Stdlib Cleanup

---

## 12. 当前问题

当前 runtime / builtin 可能存在这些状态：

```text
runtime/nova_runtime.h 中保留一些历史 API
runtime/nova_runtime.c 中有 legacy implementation
C++ sema builtin table 与 runtime API 不完全一致
Nova checker builtin table 与 C++ sema 不完全一致
Nova codegen 对部分 runtime call direct call
Nova codegen 对 vec_* 有 special-case
str_vec_* 可能已经不是主线能力，但 runtime 中仍保留
缺少 docs/standard_library.md 描述当前支持的 builtin/runtime API
```

Step 24 要把这些边界整理清楚。

---

## 13. Runtime API 分类

整理 runtime API 时，把每个函数归入一类：

```text
user-facing builtin:
  Nova 程序可以直接调用
  C++ sema / Nova checker 应该认识
  docs/standard_library.md 应该记录

compiler-generated helper:
  用户不应该直接依赖
  codegen 可能生成
  docs 可以标为 internal

legacy:
  历史遗留
  主线不再使用
  可以删除，或保留但明确标注

runtime-internal:
  只在 nova_runtime.c 内部使用
  不放进 public docs
```

---

## 14. 建议 user-facing builtins

当前建议保留的 user-facing builtins：

```text
print_int(value: int) : void
print_str(value: str) : void

str_eq(a: str, b: str) : bool
str_concat(a: str, b: str) : str
str_len(s: str) : int
str_get(s: str, index: int) : int
str_slice(s: str, start: int, end: int) : str
str_starts_with(s: str, prefix: str) : bool
str_contains(s: str, needle: str) : bool
str_ends_with(s: str, suffix: str) : bool

int_to_str(value: int) : str

read_file(path: str) : str
write_file(path: str, content: str) : void

buf_new() : int
buf_push_str(buf: int, value: str) : void
buf_push_int(buf: int, value: int) : void
buf_to_str(buf: int) : str

arg_count() : int
arg_get(index: int) : str

nova_runtime_error(message: str) : void
```

`str_ends_with` 如果 runtime 还没有，可以在本 step 加上。

---

## 15. Vec API 状态

Nova typed vec 现在是语言内建泛型风格：

```nova
vec<int>
vec<str>
vec<MyStruct>
```

用户层面使用：

```nova
vec_new()
vec_push(xs, value)
vec_get(xs, index)
vec_set(xs, index, value)
vec_len(xs)
```

这些不是普通 C runtime direct call，而是 codegen special-case。

Step 24 要明确：

```text
vec_* 是 language builtin / compiler-known operation
不是 nova_runtime.h 中固定的 user-facing C function family
```

C codegen 可以继续生成 per-type helper：

```text
NovaVec_int
NovaVec_str
NovaVec_Point
...
```

---

## 16. str_vec_* legacy 决策

检查：

```text
runtime/nova_runtime.h
runtime/nova_runtime.c
tests/
lib/
tools/
```

确认是否还有主线依赖：

```text
str_vec_new
str_vec_push
str_vec_get
str_vec_len
...
```

如果没有任何主线和测试依赖：

```text
删除 str_vec_* declarations
删除 str_vec_* implementations
删除相关 tests 或迁移到 typed vec tests
docs 中不列为 supported API
```

如果仍有 legacy tests 依赖：

```text
暂时保留 runtime implementation
不加入 C++ sema builtin table
不加入 Nova checker builtin table
docs 中标为 legacy/internal
```

不要让它处在“runtime 有，但语言层没人承认”的模糊状态。

---

## 17. C++ sema builtin table

C++ semantic analyzer 应该明确知道所有 user-facing builtins。

检查并对齐：

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
str_ends_with
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
```

要求：

```text
参数数量正确
参数类型正确
返回类型正确
nova_runtime_error 作为 no-return call
```

`vec_*` 可以继续由特殊逻辑处理，不一定放进普通 builtin function table。

---

## 18. Nova checker builtin table

Nova checker 应该和 C++ sema 对齐。

检查：

```text
is_builtin_function
builtin_return_type
check_builtin_call
no-return call list
```

要求：

```text
C++ sema 接受的 user-facing builtin，Nova checker 也接受
Nova checker 接受的 user-facing builtin，C++ sema 也接受
return type 一致
argument type 一致
```

如果实现上没有集中表，也可以先用函数分支，但 docs 要保持一致。

---

## 19. Nova codegen runtime call behavior

Nova codegen 应该明确区分：

```text
normal user function call
runtime builtin direct call
compiler-known vec operation
no-return runtime call
```

要求：

```text
普通 builtin runtime call 直接生成同名 C call
str + str 继续生成 str_concat(lhs, rhs)
vec_new / vec_push / vec_get / vec_set / vec_len 继续 special-case
unknown call 使用 user function symbol lookup
```

如果 `str_ends_with` 加入 runtime，也要确保：

```text
checker 能识别
codegen 能直接生成 str_ends_with(...)
docs 有记录
tests 覆盖
```

---

## 20. Runtime header cleanup

整理：

```text
runtime/nova_runtime.h
runtime/nova_runtime.c
```

目标：

```text
header 中只暴露 user-facing runtime API 和 codegen 必须调用的 API
internal helper 尽量 static
legacy API 明确保留或删除
实现与 header 一致
```

检查：

```text
header declared but c file missing
c file implemented but header missing
unused declarations
unused implementations
```

---

## 21. docs/standard_library.md

新增：

```text
docs/standard_library.md
```

内容至少包含：

```text
Runtime and builtin overview
Printing
String functions
File I/O
Buffers
Arguments
Errors
Vectors
Legacy/internal APIs
```

建议格式：

```markdown
# Nova Standard Library and Builtins

## Printing

### print_int(value: int) : void

Prints an integer.

### print_str(value: str) : void

Prints a string.

## Strings

### str_eq(a: str, b: str) : bool
...
```

Vectors 要说明：

```text
vec<T> 是 language builtin，由 compiler/codegen special-case
不是一组固定的 runtime C functions
```

Legacy 部分说明：

```text
str_vec_* is removed
```

或者：

```text
str_vec_* is legacy/internal and not part of the supported language surface
```

按实际决定写。

---

# Part C：测试要求

---

## 22. Runtime / builtin tests

至少新增或确认这些测试：

```text
tests/codegen/positive/string_ends_with.nv
tests/tools/typecheck/positive/string_ends_with.nv
tests/tools/codegen/string_ends_with.nv
```

如果本 step 加 `str_ends_with`。

测试示例：

```nova
fn main() : void {
    if (str_ends_with("nova.nv", ".nv")) {
        print_str("yes");
    } else {
        print_str("no");
    }
}
```

期望：

```text
yes
```

---

## 23. Builtin mismatch negative tests

确认 checker/sema 会拒绝错误 builtin 用法：

```nova
fn main() : void {
    print_int("hello");
}
```

期望：

```text
type error
```

再比如：

```nova
fn main() : void {
    let x : int = str_contains("abc", "a");
}
```

如果 `bool` 不能赋给 `int`，应该拒绝。

---

## 24. Legacy API tests

如果删除 `str_vec_*`，确认下面这种不再被接受：

```nova
fn main() : void {
    let xs : int = str_vec_new();
}
```

如果保留 legacy runtime implementation 但不作为 language builtin，也应该由 checker/sema 拒绝。

---

## 25. Import regression tests

Step 24 结束时，import tests 至少覆盖：

```text
duplicate same file
duplicate parent path
diamond import
nested relative import
import in middle
missing import
cyclic import
malformed import
Nova frontend imported source
Nova codegen imported source
```

---

# Part D：推荐实现顺序

---

## 26. Step A：冻结 Step 23

先确认当前状态稳定：

```bash
cmake --build build --parallel
ctest --test-dir build -j8 --output-on-failure
scripts/bootstrap_stage1.sh
```

---

## 27. Step B：Import tests 补齐

先不大改 source_loader，只补测试：

```text
duplicate_same_file
duplicate_parent_path
diamond
nested_relative
import_in_middle
missing/cycle/malformed location
Nova frontend import cases
Nova codegen import cases
```

如果测试暴露问题，再修 C++ / Nova loader。

---

## 28. Step C：Runtime inventory

列出当前 runtime API：

```bash
grep -R "^[a-zA-Z_].*) " runtime/nova_runtime.h runtime/nova_runtime.c
grep -R "str_vec_" .
grep -R "str_ends_with" .
```

手动整理成表：

```text
name
signature
category
used by
supported?
```

---

## 29. Step D：Builtin table 对齐

更新：

```text
C++ sema builtin table
Nova checker builtin handling
Nova codegen builtin/runtime handling
no-return function list
```

确认 C++ / Nova 行为一致。

---

## 30. Step E：Runtime cleanup

根据 inventory 决定：

```text
删除 str_vec_*，或标记 legacy/internal
新增 str_ends_with，如果本 step 决定支持
清理 header / implementation mismatch
```

---

## 31. Step F：Docs

新增：

```text
docs/standard_library.md
```

并在 README 或 docs index 中链接它。

---

## 32. Step G：Full regression

运行：

```bash
cmake --build build --parallel
ctest --test-dir build -j8 --output-on-failure
scripts/bootstrap_stage1.sh
```

如果影响 Nova codegen / frontend，也跑：

```bash
WORK="${TMPDIR:-/tmp}/nova_bootstrap_stage1"

"$WORK/nova_codegen_stage1" tools/nova_frontend.nv /tmp/nova_frontend_stage1.c
cc /tmp/nova_frontend_stage1.c runtime/nova_runtime.c -I runtime -o /tmp/nova_frontend_stage1

"$WORK/nova_codegen_stage1" tools/nova_checker.nv /tmp/nova_checker_stage1.c
cc /tmp/nova_checker_stage1.c runtime/nova_runtime.c -I runtime -o /tmp/nova_checker_stage1

"$WORK/nova_codegen_stage1" tools/nova_codegen.nv /tmp/nova_codegen_stage1_again.c
```

---

# Part E：验收标准

---

## 33. 合格

```text
import ./ 和 ../ 行为稳定
diamond import 不重复展开
missing / cyclic / malformed import diagnostics 稳定
C++ import tests 通过
runtime API inventory 完成
C++ sema / Nova checker 对 user-facing builtin 的认识一致
ctest 全过
```

---

## 34. 良好

```text
Nova source_loader 和 C++ source_loader 行为基本一致
Nova frontend/checker/codegen imported source tests 通过
str_vec_* 状态明确：删除或标记 legacy/internal
str_ends_with 支持完成
docs/standard_library.md 存在并覆盖主要 builtin
bootstrap_stage1 通过
```

---

## 35. 优秀

```text
runtime/nova_runtime.h 和 nova_runtime.c 无明显未使用 public legacy API
builtin return type / arg type 在 C++ sema、Nova checker、Nova codegen 中一致
standard library docs 与实际 builtin table 对齐
所有 import edge case 在 C++ 和 Nova tools 两边都有代表性测试
```

---

## 36. 完成标志

Step 24 完成时，应该可以说：

```text
Nova's import behavior is stable, and its runtime/builtin surface is documented and consistent across the C++ seed compiler and Nova-written tools.
```

至少这些成立：

```text
equivalent import paths only expand once
diamond import works
cycle/missing/malformed import errors point at import statements
imported source diagnostics still point at original files
runtime user-facing API list is clear
C++ sema and Nova checker agree on builtins
Nova codegen agrees with checker on runtime calls
docs/standard_library.md documents supported builtins
```

---

## 37. 后续编号调整

因为原 Step 24 和 Step 25 已经合并，后续 Phase 2 编号建议顺延：

```text
旧 Step 26 — Self-hosted build driver
  -> 新 Step 25

旧 Step 27 — Nova codegen parity expansion
  -> 新 Step 26

旧 Step 28 — VS Code developer tooling
  -> 新 Step 27

旧 Step 29 — Phase 2 self-hosting milestone
  -> 新 Step 28
```

Step 24 之后，项目会进入更明确的 self-hosted workflow 和 parity expansion。
