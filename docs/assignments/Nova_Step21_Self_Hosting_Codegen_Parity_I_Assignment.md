# Nova Project — Step 21
## 题目：Self-hosting Codegen Parity I：让 stage0 编译出 stage1

---

## 1. 当前起点

Phase 1 已经完成，当前项目状态是：

```text
C++ nova_compile:
  可以编译 Nova tools

Nova frontend:
  可以 tokens / parse / check Nova source

Nova codegen:
  可以编译普通 Nova 程序
  可以生成 C
  generated C 可以被 cc 编译运行

Full CTest:
  213/213 passed
```

Phase 2 的目标是 self-hosting。当前 readiness report 显示：

```text
stage0 nova_codegen:
  可以编译普通程序

stage0 -> stage1:
  当前失败

错误：
  CodegenError: unsupported statement type in codegen: Condition
```

也就是说，Phase 2 的第一个任务不是新增语言特性，而是补齐 Nova codegen 对自己源码所需 parse tree 结构的支持。

---

## 2. 本次作业目标

Step 21 的目标是：

> **让 C++ seed compiler 编译出的 stage0 `nova_codegen` 能够编译 `tools/nova_codegen.nv`，生成并编译出可运行的 stage1 `nova_codegen`。**

目标链路：

```text
C++ nova_compile
  -> tools/nova_codegen.nv
  -> nova_codegen_stage0

nova_codegen_stage0
  -> tools/nova_codegen.nv
  -> nova_codegen_stage1.c
  -> nova_codegen_stage1 executable
```

Step 21 完成时，至少应该能做到：

```bash
./build/nova_compile tools/nova_codegen.nv /tmp/nova_codegen_stage0.c
cc /tmp/nova_codegen_stage0.c runtime/nova_runtime.c -I runtime -o /tmp/nova_codegen_stage0

/tmp/nova_codegen_stage0 tools/nova_codegen.nv /tmp/nova_codegen_stage1.c
cc /tmp/nova_codegen_stage1.c runtime/nova_runtime.c -I runtime -o /tmp/nova_codegen_stage1
```

---

## 3. 本阶段核心贡献

Step 21 的核心贡献是：

```text
补齐 Nova codegen 对 self-hosting 所需 ParseNode statement shape 的支持。
```

当前最明确的 blocker 是：

```text
Condition
```

它来自 Step 15 后的 expression-aware parse tree：

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

`Condition`、`ThenBranch`、`ElseBranch` 是 statement 的组成部分，但它们本身不应该被当作普通 statement 传给 `gen_c_stmt()`。

---

## 4. 本阶段不做什么

Step 21 不做：

```text
stage1 -> stage2 完整验证
stage1/stage2 behavior comparison
source-mapped diagnostics
import system v2
runtime cleanup
VS Code extension
完整 codegen parity
```

这些放在后续 Step 22–29。

Step 21 只聚焦：

```text
stage0 能编译 nova_codegen.nv 到 stage1
```

---

## 5. 当前失败原因分析

当前失败：

```text
CodegenError: unsupported statement type in codegen: Condition
```

说明某处 codegen 流程把 `Condition` 当成了普通 statement。

可能的问题形态：

```nova
gen_c_block(...)
  -> iterates block children
  -> gen_c_stmt(child)
```

如果传入的不是 `BlockStmt`，而是：

```text
IfStmt.children[0] == Condition
ThenBranch
ElseBranch
```

或者错误地把 branch wrapper 当 block 处理，就会把 `Condition` 落到 unsupported statement 分支。

---

## 6. 正确的 IfStmt codegen 结构

对于：

```text
IfStmt
  Condition
    BinaryExpr op=>
      IdentifierExpr name=x
      IntLiteralExpr value=0
  Then
    Block
      ...
  Else
    Block
      ...
```

codegen 应该：

1. 取 `stmt.children[0]` 作为 `Condition`
2. 取 `Condition.children[0]` 作为 condition expression
3. 取 `stmt.children[1]` 作为 `ThenBranch`
4. 取 `ThenBranch.children[0]` 作为 then block
5. 如果存在 `stmt.children[2]`，取它作为 `ElseBranch`
6. 取 `ElseBranch.children[0]` 作为 else block

伪代码：

```nova
fn gen_c_if_stmt(out: int, stmt: ParseNode, indent: int, function_name: str, return_type: str, symbols: CodegenSymbols, scope: CodegenScope) : void {
    let condition_node : ParseNode = vec_get(stmt.children, 0);
    let condition_expr : ParseNode = vec_get(condition_node.children, 0);
    let condition_c : str = gen_c_expr(out, condition_expr, symbols, scope, "bool");

    emit_c_line(out, indent, str_concat("if (", str_concat(condition_c, ") {")));

    let then_branch : ParseNode = vec_get(stmt.children, 1);
    let then_block : ParseNode = vec_get(then_branch.children, 0);
    gen_c_block(out, then_block, indent + 1, function_name, return_type, symbols, scope);

    if (vec_len(stmt.children) > 2) {
        emit_c_line(out, indent, "} else {");

        let else_branch : ParseNode = vec_get(stmt.children, 2);
        let else_block : ParseNode = vec_get(else_branch.children, 0);
        gen_c_block(out, else_block, indent + 1, function_name, return_type, symbols, scope);
    }

    emit_c_line(out, indent, "}");
}
```

如果你当前没有单独 `gen_c_if_stmt`，也可以直接在 `gen_c_stmt()` 的 `IfStmt` 分支里按这个逻辑写。

---

## 7. 正确的 WhileStmt codegen 结构

对于：

```text
WhileStmt
  Condition
    BinaryExpr op=<
      IdentifierExpr name=i
      IntLiteralExpr value=10
  Block
    ...
```

codegen 应该：

1. 取 `stmt.children[0]` 作为 `Condition`
2. 取 `Condition.children[0]` 作为 condition expression
3. 取 `stmt.children[1]` 作为 body block

伪代码：

```nova
fn gen_c_while_stmt(out: int, stmt: ParseNode, indent: int, function_name: str, return_type: str, symbols: CodegenSymbols, scope: CodegenScope) : void {
    let condition_node : ParseNode = vec_get(stmt.children, 0);
    let condition_expr : ParseNode = vec_get(condition_node.children, 0);
    let condition_c : str = gen_c_expr(out, condition_expr, symbols, scope, "bool");

    emit_c_line(out, indent, str_concat("while (", str_concat(condition_c, ") {")));

    let body : ParseNode = vec_get(stmt.children, 1);
    gen_c_block(out, body, indent + 1, function_name, return_type, symbols, scope);

    emit_c_line(out, indent, "}");
}
```

不要把 `Condition` 或 `Block.children[0]` 误传给 `gen_c_block()`。

---

## 8. gen_c_stmt 的职责边界

`gen_c_stmt()` 应该只接收真正的 statement node：

```text
BlockStmt
LetStmt
ReturnStmt
ExprStmt
IfStmt
WhileStmt
```

它不应该收到：

```text
Condition
ThenBranch
ElseBranch
BinaryExpr
AssignExpr
CallExpr
```

如果收到这些，说明上层传错了层级。

建议保留 unsupported 分支：

```nova
codegen_error_at(stmt, str_concat("unsupported statement type in codegen: ", parse_node_kind_name(stmt.kind)));
```

但 Step 21 应该让正常 self-host path 不再触发 `Condition`。

---

## 9. ReturnStmt 的 expected type

为了 self-hosting，`ReturnStmt` 应该继续使用当前函数 return type 作为 expected type。

正确模式：

```nova
if (vec_len(stmt.children) == 0) {
    if (str_eq(function_name, "main")) {
        emit_c_line(out, indent, "return 0;");
    } else {
        emit_c_line(out, indent, "return;");
    }
} else {
    let expr : ParseNode = vec_get(stmt.children, 0);
    let expr_c : str = gen_c_expr(out, expr, symbols, scope, current_return_type);
    emit_c_line(out, indent, str_concat("return ", str_concat(expr_c, ";")));
}
```

不要靠 `infer_codegen_expr_type()` 去推导复杂 return expression。

---

## 10. Block scope 注意事项

如果 `gen_c_block()` 已经使用 `CodegenScope`，需要确认 block 内局部变量不会污染兄弟 block。

如果当前实现采用单个 append-only scope，并从后往前查变量，也可以暂时接受。  
但如果要更准确，可以为 block 创建局部 scope depth。

Step 21 不要求重构 scope，只要 self-hosting path 能正确生成即可。

---

## 11. 建议新增 bootstrap 脚本

虽然 Step 21 的贡献不是“测试”，但需要一个可重复入口验证 stage0 -> stage1。

建议新增：

```text
scripts/bootstrap_stage1.sh
```

内容：

```bash
#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

cmake -S . -B build
cmake --build build --parallel

WORK="${TMPDIR:-/tmp}/nova_bootstrap_stage1"
rm -rf "$WORK"
mkdir -p "$WORK"

echo "[1/4] Build stage0 nova_codegen with C++ seed compiler"
./build/nova_compile tools/nova_codegen.nv "$WORK/nova_codegen_stage0.c"

echo "[2/4] Compile stage0"
cc "$WORK/nova_codegen_stage0.c" runtime/nova_runtime.c -I runtime -o "$WORK/nova_codegen_stage0"

echo "[3/4] Use stage0 to generate stage1 C"
"$WORK/nova_codegen_stage0" tools/nova_codegen.nv "$WORK/nova_codegen_stage1.c"

echo "[4/4] Compile stage1"
cc "$WORK/nova_codegen_stage1.c" runtime/nova_runtime.c -I runtime -o "$WORK/nova_codegen_stage1"

echo "stage1 bootstrap OK"
echo "work dir: $WORK"
```

这个脚本可以后续在 Step 26 升级成完整 self-host driver。

---

## 12. 可选 CTest

Step 21 可以先只加脚本，不必马上 CTest 化。

如果要接 CTest，可以新增：

```cmake
add_test(
    NAME bootstrap_stage1_codegen
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/bootstrap_stage1.sh
)
set_tests_properties(bootstrap_stage1_codegen PROPERTIES LABELS "bootstrap;self_host")
```

如果担心 CI 太慢，可以先不默认接入，或者后续 Step 26 再统一接入。

---

## 13. Debug 建议

如果 stage0 仍失败，建议让 codegen error 打印更具体的 node kind。

例如：

```nova
fn parse_node_kind_name(kind: ParseNodeKind) : str
```

如果已有 `parse_node_header()`，也可以用它。

错误信息建议包含：

```text
CodegenError at line:column: unsupported statement type in codegen: Condition
```

如果还是难定位，可以临时在 `gen_c_stmt()` 输出：

```text
generating statement: <kind>
```

但不要让 debug 输出进入最终 tests。

---

## 14. 推荐实现顺序

### Step A：复现当前失败

手动运行：

```bash
./build/nova_compile tools/nova_codegen.nv /tmp/nova_codegen_stage0.c
cc /tmp/nova_codegen_stage0.c runtime/nova_runtime.c -I runtime -o /tmp/nova_codegen_stage0
/tmp/nova_codegen_stage0 tools/nova_codegen.nv /tmp/nova_codegen_stage1.c
```

确认失败仍是：

```text
CodegenError: unsupported statement type in codegen: Condition
```

---

### Step B：检查 IfStmt / WhileStmt parse tree shape

用 frontend parse mode：

```bash
./build/nova_compile tools/nova_frontend.nv /tmp/nova_frontend.c
cc /tmp/nova_frontend.c runtime/nova_runtime.c -I runtime -o /tmp/nova_frontend

/tmp/nova_frontend parse tools/nova_codegen.nv /tmp/nova_codegen.parse
```

搜索：

```bash
grep -n "IfStmt\|WhileStmt\|Condition\|Then\|Else" /tmp/nova_codegen.parse | head -80
```

确认结构。

---

### Step C：修 `gen_c_stmt(IfStmt)`

确保它只把 Block 传给 `gen_c_block()`。

---

### Step D：修 `gen_c_stmt(WhileStmt)`

同样确保它只把 Block 传给 `gen_c_block()`。

---

### Step E：确认 `ReturnStmt` 使用当前 return type

避免复杂 return expression 影响 stage1。

---

### Step F：运行 stage0 -> stage1

```bash
scripts/bootstrap_stage1.sh
```

或手动命令。

---

## 15. 新增测试样例

如果你想给 Step 21 加一个小的 regression test，可以新增一个 Nova codegen test，专门覆盖 branch wrapper shape。

### `tests/tools/codegen/control_flow_nested.nv`

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

### `tests/tools/codegen/control_flow_nested.out`

```text
2
1
0
```

CMake：

```cmake
add_nova_codegen_tool_test(control_flow_nested)
```

这个测试能覆盖：

```text
IfStmt
Condition
ThenBranch
ElseBranch
nested IfStmt
ReturnStmt complex branch
```

---

## 16. 验收标准

### 合格

```text
Condition blocker 修复
stage0 能生成 nova_codegen_stage1.c
nova_codegen_stage1.c 能编译成 executable
```

### 良好

```text
新增 scripts/bootstrap_stage1.sh
control_flow_nested codegen test 通过
stage1 executable 能编译 hello.nv
```

### 优秀

```text
stage1 executable 能编译 vec_struct_basic.nv
Step 21 的 bootstrap 脚本输出清楚
失败时能定位到具体 unsupported node
```

---

## 17. 完成标志

Step 21 完成时，下面流程应该成功：

```bash
./build/nova_compile tools/nova_codegen.nv /tmp/nova_codegen_stage0.c
cc /tmp/nova_codegen_stage0.c runtime/nova_runtime.c -I runtime -o /tmp/nova_codegen_stage0

/tmp/nova_codegen_stage0 tools/nova_codegen.nv /tmp/nova_codegen_stage1.c
cc /tmp/nova_codegen_stage1.c runtime/nova_runtime.c -I runtime -o /tmp/nova_codegen_stage1
```

并且可选验证：

```bash
/tmp/nova_codegen_stage1 tests/tools/codegen/hello.nv /tmp/hello.stage1.c
cc /tmp/hello.stage1.c runtime/nova_runtime.c -I runtime -o /tmp/hello.stage1
/tmp/hello.stage1
```

输出：

```text
hello
```

这标志着 Phase 2 的第一步完成：Nova-generated codegen 已经可以生成下一代 Nova codegen。
