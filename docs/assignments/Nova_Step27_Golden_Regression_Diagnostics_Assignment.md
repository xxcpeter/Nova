# Nova Project — Step 27
## 题目：Golden Test / Regression Cleanup + Diagnostics Polish

---

## 1. 当前起点

Step 26 已经完成：

```text
VS Code extension 可用
.nv 文件识别正常
syntax highlighting 覆盖当前 Nova 语言表面
snippets / tasks / problem matcher / README 已补齐
```

Phase 2 前面已经完成：

```text
Step 21:
  stage0 -> stage1 nova_codegen

Step 22:
  stage-generated tools 可运行
  str + str 支持
  Nova checker missing-return / no-return 检查

Step 23:
  source-aware diagnostics

Step 24:
  import v2 finalization
  runtime / builtin / stdlib cleanup
  docs/standard_library.md

Step 25:
  self-hosted build driver
  stage0 / stage1 / stage2 workflow
  codegen parity representative matrix

Step 26:
  VS Code developer tooling
```

现在测试和 diagnostics 已经很多，但也有历史演进留下的问题：

```text
golden files 命名和分布可能不够统一
CTest labels 可能不够清晰
一些旧 bug 可能只靠集成测试覆盖，没有 focused regression
user-facing error 和 internal error 的边界需要更清楚
diagnostics 格式需要统一
docs/testing.md 需要跟随最新测试结构更新
```

Step 27 的目标就是做一次 regression 和 diagnostics 质量收口。

---

## 2. 本次作业目标

Step 27 的目标是：

> **整理 golden tests、补齐关键 regression tests、统一 diagnostics 规范，并更新测试文档。**

核心成果：

```text
1. CTest labels 更清楚
2. golden test 分类更清楚
3. 关键历史 bug 有 focused regression
4. diagnostics 格式统一
5. user-facing diagnostics 尽量都有 file:line:column
6. internal errors 明确标记为 internal error
7. docs/testing.md 更新
8. 全量 ctest 和 self_host 通过
```

这一步不是“纯测试 step”。  
它的贡献是提高项目的长期可维护性，降低后续 Phase 2/3 改动破坏已有能力的风险。

---

## 3. 本阶段定位

Step 27 是 quality / regression / diagnostics cleanup。

它应该解决：

```text
测试结构随着 Step 1–26 演进变复杂
一些测试目录含义不够清晰
有些 CTest labels 不好筛选
diagnostics 格式和位置历史不一致
user-facing errors 可能仍有无位置版本
internal error 和 source error 可能混在一起
docs/testing.md 与当前测试结构不完全同步
```

它不要求解决：

```text
新语言功能
新 runtime API
新 import 语义
新 VS Code 功能
真正 module system
完整 LSP diagnostics
code frame / multi-span diagnostics
```

---

## 4. 本阶段不做什么

Step 27 不做：

```text
break / continue / for loop
module namespace
export/private
runtime process API
Nova build driver
formatter
LSP
source-map diagnostics v2
diagnostic code frames
warning system
fix-it suggestions
```

这些后续 step 或 Phase 3 再做。

---

# Part A：Golden Test Cleanup

---

## 5. 当前测试类型

当前 Nova 测试大致分为两大类：

```text
C++ compiler tests:
  tests/lexer/
  tests/parser/
  tests/sema/
  tests/codegen/
  tests/import/

Nova-written tool tests:
  tests/tools/tokenizer/
  tests/tools/expr_parser/
  tests/tools/checker/
  tests/tools/typecheck/
  tests/tools/frontend/
  tests/tools/frontend_import/
  tests/tools/codegen/
```

Step 27 不要求大规模移动目录，但要让含义清楚。

---

## 6. Golden file 后缀规范

建议继续保留现有后缀：

```text
*.tok
  token output

*.out
  parse output / stdout output / generic expected output

*.err
  expected stderr diagnostics

*.check
  checker summary output
```

不要把这些加入 `.gitignore`：

```text
*.tok
*.out
*.err
*.check
```

这些都是测试 golden files。

---

## 7. 命名建议

新增测试时尽量用描述性名字：

```text
good:
  import_in_middle
  duplicate_parent_path
  vec_get_field_access
  missing_return_no_return_call
  malformed_import_location

bad:
  test1
  case_a
  new_bug
```

命名应该描述：

```text
feature_or_bug_condition
```

例如：

```text
else_if_condition_codegen
vec_get_call_type_inference
field_access_on_vec_get_result
source_loader_import_middle_line_map
```

---

## 8. 保持目录语义

推荐语义：

```text
tests/codegen/positive/
  C++ compiler codegen tests

tests/tools/codegen/
  Nova-written codegen tool tests

tests/import/
  C++ source_loader/import tests

tests/tools/frontend_import/
  Nova-written frontend import tests
```

不要把 C++ compiler tests 和 Nova tool tests 混在一起。

如果某个 case 同时需要 C++ 和 Nova 覆盖，可以分别添加：

```text
tests/import/positive/<case>/
tests/tools/frontend_import/positive/<case>/
```

---

## 9. Expected prefix tests

Smoke tests 可以继续用 prefix compare，例如：

```text
Check OK
```

但普通功能 tests 应该尽量 exact compare。

规则建议：

```text
Smoke / large summary:
  prefix compare ok

Feature behavior:
  exact compare

Diagnostics:
  exact compare unless path/platform 不稳定
```

如果 diagnostics 已经 source-aware 且 path 稳定，优先 exact compare。

---

# Part B：CTest Label Cleanup

---

## 10. 为什么整理 labels

现在 test 数量已经很多，常用命令应该足够好用：

```bash
ctest --test-dir build -L nova_tool
ctest --test-dir build -L import
ctest --test-dir build -L selfhost
ctest --test-dir build -L codegen
```

Step 27 要确认 labels 足够清楚。

---

## 11. 推荐 labels

建议使用这些 label：

```text
lexer
parser
sema
codegen
import
runtime
nova_tool
frontend
checker
typecheck
selfhost
bootstrap
vscode
negative
positive
```

并且组合使用：

```text
nova_tool;frontend
nova_tool;codegen
import;positive
import;negative
selfhost;codegen
```

---

## 12. CTest label audit

运行：

```bash
ctest --test-dir build -N
```

如果要看 labels，可以查看 build 下生成的 CTest 文件，或在 CMakeLists 中确认。

Step 27 至少确认：

```text
nova_tool tests 有 nova_tool label
import tests 有 import label
self-host driver 有 selfhost 或 bootstrap label
negative tests 有 negative label
codegen tests 有 codegen label
```

---

## 13. 常用测试命令文档化

在 `docs/testing.md` 中确认这些命令存在：

```bash
ctest --test-dir build -j8 --output-on-failure
ctest --test-dir build -L nova_tool -j8 --output-on-failure
ctest --test-dir build -L import -j8 --output-on-failure
ctest --test-dir build -L selfhost --output-on-failure
ctest --test-dir build -R nova_tool_codegen -j8 --output-on-failure
```

---

# Part C：Focused Regression Tests

---

## 14. 为什么需要 focused regression

Self-host driver 和 full ctest 是 integration safety net。

但历史 bug 最好有 focused regression，原因是：

```text
失败更快
定位更准
不会等到 self_host 大流程才发现
```

Step 27 要把前面修过的关键坑补成小测试。

---

## 15. 建议补的 regression cases

### 15.1 Condition node 不应被当 statement

历史问题：

```text
CodegenError: unsupported statement type in codegen: Condition
```

测试建议：

```text
tests/tools/codegen/control_flow_nested.nv
tests/tools/codegen/control_flow_nested.out
```

输入：

```nova
fn choose(x: int) : int {
    if (x > 0) {
        if (x > 10) {
            return 2;
        } else {
            return 1;
        }
    }

    return 0;
}

fn main() : void {
    print_int(choose(12));
    print_int(choose(5));
    print_int(choose(0));
    return;
}
```

期望：

```text
2
1
0
```

---

### 15.2 else-if branch shape

如果 parser 对 `else if` 生成：

```text
ElseBranch
  IfStmt
```

需要单独测试。

```text
tests/tools/codegen/else_if_chain.nv
tests/tools/codegen/else_if_chain.out
```

输入：

```nova
fn classify(x: int) : str {
    if (x < 0) {
        return "neg";
    } else if (x == 0) {
        return "zero";
    } else {
        return "pos";
    }
}

fn main() : void {
    print_str(classify(-1));
    print_str(classify(0));
    print_str(classify(1));
    return;
}
```

期望：

```text
neg
zero
pos
```

---

### 15.3 vec_get CallExpr type inference

历史问题：

```text
unsupported expression type for type inference in codegen: CallExpr name=vec_get
```

测试建议：

```text
tests/tools/codegen/vec_get_field_access.nv
tests/tools/codegen/vec_get_field_access.out
```

输入：

```nova
struct Point {
    x: int;
    y: int;
}

fn main() : void {
    let points : vec<Point> = vec_new();
    vec_push(points, Point { x: 10, y: 20 });

    print_int(vec_get(points, 0).x);
    print_int(vec_get(points, 0).y);
    return;
}
```

期望：

```text
10
20
```

---

### 15.4 FieldAccessExpr type inference

测试：

```text
tests/tools/codegen/nested_field_access.nv
tests/tools/codegen/nested_field_access.out
```

输入：

```nova
struct Point {
    x: int;
    y: int;
}

struct Line {
    start: Point;
    end: Point;
}

fn main() : void {
    let line : Line = Line {
        start: Point { x: 1, y: 2 },
        end: Point { x: 3, y: 4 }
    };

    print_int(line.start.x);
    print_int(line.end.y);
    return;
}
```

期望：

```text
1
4
```

---

### 15.5 import in middle line map

历史问题：

```text
LoadedSource.text 和 line map 顺序错位
```

测试已在 Step 24 中建议，如果还没加，需要保留。

---

### 15.6 EOF source location

如果 Step 23 修过 EOF 行号，需要测试 parse error at EOF。

例如：

```text
tests/tools/frontend/negative/parse/eof_missing_rbrace.nv
tests/tools/frontend/negative/parse/eof_missing_rbrace.err
```

输入：

```nova
fn main() : void {
    return;
```

期望 error 指向合理 EOF location。

---

### 15.7 malformed import source location

如果 Step 23 修过：

```text
malformed import directive
```

需要保留 regression。

---

### 15.8 no-return missing-return

如果 Step 22 修过 no-return control flow，需要确保：

```text
noreturn_error_call
noreturn_if_else
missing_return
```

都在 tests 中。

---

## 16. Regression tests 不要过量

不要为了每个 helper 写测试。

优先覆盖：

```text
历史真实 bug
容易回归的 import/diagnostic/codegen shape
self-hosting blocker
```

---

# Part D：Diagnostics Polish

---

## 17. Diagnostics 目标格式

Step 23 后推荐统一：

```text
file:line:column: ErrorKind: message
```

例如：

```text
tests/tools/typecheck/negative/missing_return.nv:1:1: CheckerError: missing return statement in function 'bad'
```

所有 user-facing error 尽量使用这个格式。

---

## 18. Error kind 规范

建议使用固定 kind：

```text
LexerError
ParserError
SemanticError
CheckerError
CodegenError
ImportError
```

不要混用：

```text
parse error
lexer error
TypeError
runtime checker error
```

除非是旧 C++ sema 保留的特定格式，但长期应统一。

---

## 19. internal error 规范

内部 invariant 失败可以没有 source location，但必须明确写：

```text
internal error: ...
```

例如：

```nova
checker_error("internal error: struct '" + name + "' not found in symbol table");
```

这是可以接受的，因为它不是用户源码错误。

但 user-facing error 不应该写成 internal error。

---

## 20. grep audit

Step 27 建议做一次 grep audit：

```bash
grep -R "checker_error(" -n lib tools
grep -R "codegen_error(" -n lib tools
grep -R "parser_error(" -n lib tools
grep -R "lexer_error(" -n lib tools
grep -R "import_error(" -n lib tools
```

目标：

```text
无位置 error 只剩 internal error 或 fallback
user-facing error 都改成 *_error_at(...)
```

如果有例外，在代码旁边加注释说明：

```nova
// Internal invariant: caller must check has_struct first.
checker_error("internal error: ...");
```

---

## 21. diagnostics.nv 边界

`diagnostics.nv` 应保持底层，不依赖：

```text
ParseNode
Token
LoadedSource
```

它只应该提供：

```nova
fn format_source_location(file: str, line: int, column: int) : str
fn lexer_error_at(file: str, line: int, column: int, message: str) : void
fn parser_error_at(file: str, line: int, column: int, message: str) : void
fn checker_error_at(file: str, line: int, column: int, message: str) : void
fn codegen_error_at(file: str, line: int, column: int, message: str) : void
fn import_error_at(file: str, line: int, column: int, message: str) : void
```

Node-specific wrappers 应该放在：

```text
parser.nv
checker.nv
codegen_c.nv
tokenizer.nv
source_loader.nv
```

而不是 diagnostics.nv。

---

## 22. C++ diagnostics audit

同样检查：

```bash
grep -R "Error at" -n src include
grep -R "SemanticError" -n src include tests
grep -R "ParserError" -n src include tests
grep -R "ImportError" -n src include tests
```

目标：

```text
source-aware location 使用一致
import errors 使用 import site
imported source errors 使用 imported file local location
```

---

# Part E：Golden Update Policy

---

## 23. 什么时候更新 golden

只有这些情况应该更新 golden：

```text
diagnostic 格式有意变更
source-aware path 有意加入
错误位置从错误位置改成正确位置
runtime/builtin 行为有意变更
parse/tree output 有意变更
```

不要因为临时 debug output 更新 golden。

---

## 24. Golden update 流程

建议在 `docs/testing.md` 里写：

```text
1. 先运行 failing test，确认 actual output 是正确的新行为
2. 手动更新对应 .out/.err/.tok/.check
3. 重新运行目标 test
4. 运行相关 label test
5. 最后运行 full ctest
```

避免一次性盲目覆盖所有 golden。

---

## 25. 不要提交临时输出

确认 `.gitignore` 仍然忽略：

```text
build/
Testing/
tmp/
*.generated.c
*.tmp.c
*.log
```

但不忽略：

```text
*.out
*.err
*.tok
*.check
```

---

# Part F：docs/testing.md 更新

---

## 26. docs/testing.md 必须包含

更新：

```text
docs/testing.md
```

至少包含：

```text
test categories
golden file meaning
CTest labels
common commands
how to add C++ compiler test
how to add Nova tool test
how to add import test
how to add self-host test
how to update golden files
how to debug failing generated C
```

---

## 27. 推荐命令

```bash
cmake --build build --parallel
ctest --test-dir build -j8 --output-on-failure
ctest --test-dir build -L nova_tool -j8 --output-on-failure
ctest --test-dir build -L import -j8 --output-on-failure
ctest --test-dir build -L selfhost --output-on-failure
ctest --test-dir build -R nova_tool_codegen -j8 --output-on-failure
scripts/self_host.sh
```

---

# Part G：Optional helper scripts

---

## 28. Optional regression script

可以新增：

```text
scripts/run_regression.sh
```

内容：

```bash
#!/usr/bin/env bash
set -euo pipefail

cmake --build build --parallel
ctest --test-dir build -j8 --output-on-failure
scripts/self_host.sh
```

这不是必须，但很方便。

---

## 29. Optional diagnostics audit script

可以新增：

```text
scripts/audit_diagnostics.sh
```

内容可以只是 grep：

```bash
#!/usr/bin/env bash
set -euo pipefail

grep -R "checker_error(" -n lib tools || true
grep -R "codegen_error(" -n lib tools || true
grep -R "parser_error(" -n lib tools || true
grep -R "lexer_error(" -n lib tools || true
grep -R "import_error(" -n lib tools || true
```

用于人工 review。

---

# Part H：推荐实现顺序

---

## 30. Step A：Audit current test labels

检查 CMakeLists 中 labels。

确保：

```text
nova_tool
import
codegen
frontend
checker
typecheck
selfhost
negative
positive
```

有合理覆盖。

---

## 31. Step B：Add focused regression tests

先补历史 bug regression：

```text
Condition node
else-if branch
vec_get call type inference
nested field access
import in middle
EOF source location
malformed import location
no-return / missing-return
```

---

## 32. Step C：Diagnostics grep audit

执行：

```bash
grep -R "checker_error(" -n lib tools
grep -R "codegen_error(" -n lib tools
grep -R "parser_error(" -n lib tools
grep -R "lexer_error(" -n lib tools
grep -R "import_error(" -n lib tools
```

把 user-facing errors 改成 source-aware versions。

---

## 33. Step D：C++ diagnostics audit

检查 C++ diagnostic formatting 和 source-aware locations。

如果有旧格式是 intentional，保留并记录。

---

## 34. Step E：Update docs/testing.md

同步测试分类、labels、golden update policy、self-host tests。

---

## 35. Step F：Full regression

运行：

```bash
cmake --build build --parallel
ctest --test-dir build -j8 --output-on-failure
scripts/self_host.sh
```

---

# Part I：验收标准

---

## 36. 合格

```text
关键历史 bug 有 focused regression tests
CTest labels 足够清晰
docs/testing.md 更新
user-facing diagnostics 基本都有 file:line:column
internal errors 明确标记
ctest 全过
```

---

## 37. 良好

```text
diagnostics grep audit 只剩合理 fallback/internal error
golden update policy 写入 docs/testing.md
selfhost tests label 清楚
import/codegen/checker/typecheck regression 覆盖充分
scripts/self_host.sh 通过
```

---

## 38. 优秀

```text
新增 regression/audit helper script
C++ 和 Nova diagnostics 风格基本统一
常见 historical bugs 都有 focused tests
测试分类和文档足够新开发者理解
```

---

## 39. 完成标志

Step 27 完成时，应该可以说：

```text
Nova's regression suite and diagnostics conventions are stable enough to support continued self-hosting development.
```

至少这些成立：

```text
historical self-hosting bugs are covered by focused tests
golden files are organized and documented
diagnostics are source-aware and consistently formatted
CTest labels allow targeted test runs
docs/testing.md explains how to add and update tests
full CTest and self_host pass
```

---

## 40. 后续关联

Step 28 将继续：

```text
Nova Compiler Driver + Toolchain UX Cleanup
```

也就是：

```text
tools/nova_compile.nv
unified CLI usage
docs/tools.md
bad-args tests
stage-generated nova_compile smoke
```

Step 27 只负责测试与 diagnostics 的工程质量收口，不继续增加 compiler feature。 
