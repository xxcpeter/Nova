# Nova Project — Step 22
## 题目：Self-hosting Codegen Parity II + Source Ergonomics

---

## 1. 当前起点

Step 21 已经完成：

```text
C++ seed compiler
  -> 编译 tools/nova_codegen.nv
  -> 生成 nova_codegen_stage0

nova_codegen_stage0
  -> 编译 tools/nova_codegen.nv
  -> 生成 nova_codegen_stage1
```

同时 Step 21 还完成了一个重要 cleanup：

```text
C++ sema 把 no-return error call 视作 control-flow terminating。
所有 dummy return 已删除。
```

现在 Phase 2 已经进入真正 self-hosting 路线。Step 22 的目标是继续推进：

```text
不只是能生成 stage1 nova_codegen，
还要让 stage-generated tools 能实际运行代表性输入。
```

---

## 2. 本次作业目标

Step 22 的主目标：

> **让 stage0 生成的 stage1 compiler tools 可实际使用。**

重点目标工具：

```text
tools/nova_codegen.nv
tools/nova_frontend.nv
tools/nova_checker.nv
tools/nova_expr_parser.nv
```

Step 22 完成后，应该可以做到：

```text
nova_codegen_stage1:
  能编译普通 codegen tests

nova_frontend_stage1:
  能 tokens / parse / check

nova_checker_stage1:
  能运行代表性 checker/typecheck tests

nova_expr_parser_stage1:
  能运行代表性 expression parser tests
```

---

## 3. 本阶段附带小改进

Step 22 可以吸收两项前面攒下来的小改进：

```text
1. 支持 str + str 作为 str_concat 的语法糖
2. Nova checker 补 no-return / missing-return 基础控制流检查
```

这两个改动都服务于 self-hosting：

```text
str + str:
  减少 compiler source 里大量嵌套 str_concat，提高可读性。

Nova checker no-return / missing-return:
  让 Nova-written checker 更接近 C++ sema 行为，
  也让 stage-generated checker 更可信。
```

---

## 4. 本阶段不做什么

Step 22 不做：

```text
stage1 -> stage2 正式 behavior comparison
source-aware diagnostics
import system v2
runtime / builtin API 大清理
str_ends_with
VS Code hover / definition
完整 C++ codegen parity
大规模替换所有 str_concat
```

这些后续 step 再做。

尤其：

```text
str_ends_with
```

建议留到 Step 25：Runtime / builtin / stdlib cleanup。

---

## 5. 主线任务 A：stage1 nova_codegen 能编译代表性程序

Step 21 已经有：

```text
nova_codegen_stage1 executable
```

Step 22 要确认它不只是能被编译出来，还能实际编译程序。

建议使用：

```text
tests/tools/codegen/hello.nv
tests/tools/codegen/vec_struct_basic.nv
tests/tools/codegen/vec_str_basic.nv
tests/tools/codegen/recursion.nv
```

手动验收示例：

```bash
scripts/bootstrap_stage1.sh

WORK="${TMPDIR:-/tmp}/nova_bootstrap_stage1"

"$WORK/nova_codegen_stage1" tests/tools/codegen/hello.nv /tmp/hello.stage1.c
cc /tmp/hello.stage1.c runtime/nova_runtime.c -I runtime -o /tmp/hello.stage1
/tmp/hello.stage1
```

期望：

```text
hello
```

再测试复杂一点：

```bash
"$WORK/nova_codegen_stage1" tests/tools/codegen/vec_struct_basic.nv /tmp/vec_struct_basic.stage1.c
cc /tmp/vec_struct_basic.stage1.c runtime/nova_runtime.c -I runtime -o /tmp/vec_struct_basic.stage1
/tmp/vec_struct_basic.stage1
```

期望输出匹配：

```text
tests/tools/codegen/vec_struct_basic.out
```

---

## 6. 主线任务 B：stage0 编译 frontend/checker/expr_parser

使用 stage0 生成 stage1 tools：

```bash
WORK="${TMPDIR:-/tmp}/nova_bootstrap_stage1"

"$WORK/nova_codegen_stage0" tools/nova_frontend.nv /tmp/nova_frontend_stage1.c
cc /tmp/nova_frontend_stage1.c runtime/nova_runtime.c -I runtime -o /tmp/nova_frontend_stage1

"$WORK/nova_codegen_stage0" tools/nova_checker.nv /tmp/nova_checker_stage1.c
cc /tmp/nova_checker_stage1.c runtime/nova_runtime.c -I runtime -o /tmp/nova_checker_stage1

"$WORK/nova_codegen_stage0" tools/nova_expr_parser.nv /tmp/nova_expr_parser_stage1.c
cc /tmp/nova_expr_parser_stage1.c runtime/nova_runtime.c -I runtime -o /tmp/nova_expr_parser_stage1
```

如果某个工具失败，优先判断是不是 Nova codegen 还缺少某个表达式或 statement shape 的支持。

---

## 7. 主线任务 C：stage-generated frontend 可运行

`nova_frontend_stage1` 应该支持：

```text
tokens
parse
check
```

验收命令：

```bash
/tmp/nova_frontend_stage1 tokens tests/tools/frontend/tokens/basic.nv /tmp/basic.stage1.tok
cat /tmp/basic.stage1.tok

/tmp/nova_frontend_stage1 parse tests/tools/frontend/parse/expression.nv /tmp/expression.stage1.out
cat /tmp/expression.stage1.out

/tmp/nova_frontend_stage1 check tools/nova_checker.nv /tmp/nova_checker.stage1.check
head -20 /tmp/nova_checker.stage1.check
```

期望：

```text
check 输出以 Check OK 开头
```

---

## 8. 主线任务 D：stage-generated checker 可运行

`nova_checker_stage1` 应该能运行代表性 checker/typecheck 输入。

建议先跑：

```bash
/tmp/nova_checker_stage1 tests/tools/typecheck/positive/vec_basic.nv /tmp/vec_basic.stage1.check
cat /tmp/vec_basic.stage1.check
```

期望：

```text
Check OK
```

再跑一个 struct/enum/vector 综合输入：

```bash
/tmp/nova_checker_stage1 tests/tools/frontend/check/struct_enum_vec.nv /tmp/struct_enum_vec.stage1.check
cat /tmp/struct_enum_vec.stage1.check
```

---

## 9. 主线任务 E：stage-generated expr_parser 可运行

`nova_expr_parser_stage1` 应该能跑表达式 parser 测试。

示例：

```bash
/tmp/nova_expr_parser_stage1 tests/tools/expr_parser/expression_precedence.nv /tmp/expression_precedence.stage1.out
cat /tmp/expression_precedence.stage1.out
```

输出应该和：

```text
tests/tools/expr_parser/expression_precedence.out
```

一致。

如果当前 `nova_expr_parser.nv` 的 CLI 格式和这个命令不同，以现有 tool test driver 为准。

---

## 10. 可能遇到的 codegen blocker

Step 22 按“遇到 compiler source 实际 blocker 就补”的原则推进。

可能需要补的能力包括：

```text
CallExpr type inference for user functions
FieldAccessExpr type inference edge cases
StructLiteral field expected type
vec_new in nested contexts
vec_get on non-identifier expressions
nested block / if / while codegen shape
function return type lookup
scope lookup edge cases
runtime call return type inference
```

这些都属于：

```text
让 Nova codegen 能编译并运行 compiler source
```

不是随意扩语言。

---

# Part II：`str + str` 语法糖

---

## 11. 目标

支持：

```nova
let s : str = "hello, " + name;
```

语义等价于：

```nova
let s : str = str_concat("hello, ", name);
```

这只是语法糖，底层 runtime 仍然可以使用：

```text
str_concat
```

---

## 12. C++ sema 修改

在 C++ semantic analyzer 中，二元 `+` 当前大概率只支持 `int + int`。

需要增加：

```text
str + str -> str
```

建议规则：

```text
int + int -> int
str + str -> str
其他类型组合 -> type error
```

不建议支持：

```text
str + int
int + str
```

除非显式用：

```nova
str_concat("x=", int_to_str(x))
```

这样类型系统更简单。

---

## 13. C++ codegen 修改

当 `BinaryExpr op=+` 的表达式类型是 `str` 时，生成：

```c
str_concat(lhs, rhs)
```

而不是：

```c
(lhs + rhs)
```

因此 C++ AST/codegen 需要能知道这个 binary expr 的 resolved type。

如果当前 C++ AST 已经有：

```cpp
resolved_type
```

就使用它。

伪逻辑：

```cpp
if (expr.op == BinaryOp::Add && expr.resolved_type == TypeKind::Str) {
    return "str_concat(" + emit_expr(lhs) + ", " + emit_expr(rhs) + ")";
}
```

否则仍按数值加法生成。

---

## 14. Nova checker 修改

在 `lib/checker.nv` 的 binary expression type inference 中增加：

```text
op '+':
  int + int -> int
  str + str -> str
```

其他 arithmetic op 仍然只支持 `int`。

示例规则：

```nova
if (str_eq(op, "+")) {
    if (str_eq(left_type, "int") && str_eq(right_type, "int")) {
        return "int";
    }

    if (str_eq(left_type, "str") && str_eq(right_type, "str")) {
        return "str";
    }

    checker_error_at(expr.line, expr.column, ...);
}
```

---

## 15. Nova codegen 修改

在 `lib/codegen_c.nv` 的 binary expression generation 中：

```text
BinaryExpr op=+
```

如果表达式推断类型是 `str`，生成：

```c
str_concat(lhs, rhs)
```

否则生成：

```c
(lhs + rhs)
```

注意：

```text
gen_c_expr(BinaryExpr)
```

可能需要调用：

```nova
infer_codegen_expr_type(expr, symbols, scope)
```

或通过 expected type 判断。

更稳的是：

```nova
let result_type : str = infer_codegen_expr_type(expr, symbols, scope);
if (str_eq(expr.value, "+") && str_eq(result_type, "str")) {
    return str_concat("str_concat(", str_concat(left_c, str_concat(", ", str_concat(right_c, ")"))));
}
```

如果 `infer_codegen_expr_type(BinaryExpr)` 还不支持，需要补：

```text
int + int -> int
str + str -> str
comparisons -> bool
logical ops -> bool
```

至少要覆盖 `+`。

---

## 16. 测试建议：C++ codegen

新增或更新：

### `tests/codegen/positive/string_plus.nv`

```nova
fn main() : void {
    let a : str = "hello";
    let b : str = " world";
    print_str(a + b);
    return;
}
```

### `tests/codegen/positive/string_plus.out`

```text
hello world
```

---

## 17. 测试建议：Nova typecheck

### `tests/tools/typecheck/positive/string_plus.nv`

```nova
fn main() : void {
    let a : str = "hello";
    let b : str = " world";
    let c : str = a + b;
    print_str(c);
    return;
}
```

### `tests/tools/typecheck/positive/string_plus.out`

```text
Check OK
Structs: 0
Enums: 0
Functions: 1
```

如果你的 checker summary 数字不同，按实际输出更新。

---

## 18. 测试建议：Nova codegen

### `tests/tools/codegen/string_plus.nv`

```nova
fn main() : void {
    let a : str = "hello";
    let b : str = " world";
    print_str(a + b);
    return;
}
```

### `tests/tools/codegen/string_plus.out`

```text
hello world
```

---

## 19. 是否替换现有 `str_concat`

Step 22 不建议大规模替换所有 `str_concat`。

建议：

```text
1. 先支持 str + str
2. 只替换少量明显难读的嵌套 str_concat
3. 保留 str_concat runtime API
```

大规模风格替换可以后面单独做，避免 Step 22 diff 太大。

---

# Part III：Nova checker no-return / missing-return

---

## 20. 当前状态

C++ sema 已经支持：

```text
no-return error call 视作 control-flow terminating
```

例如：

```nova
fn fail() : str {
    codegen_error("failed");
}
```

不再需要：

```nova
return "";
```

但 `lib/checker.nv` 当前只检查：

```text
已有 ReturnStmt 的类型是否匹配
```

它没有完整 missing-return / all-paths-return 检查。

Step 22 可以补一个基础版，让 Nova checker 行为更接近 C++ sema。

---

## 21. no-return 函数列表

建议在 `checker.nv` 中增加：

```nova
fn is_no_return_call_name(name: str) : bool {
    return str_eq(name, "nova_runtime_error") ||
           str_eq(name, "parse_error") ||
           str_eq(name, "checker_error") ||
           str_eq(name, "checker_error_at") ||
           str_eq(name, "codegen_error") ||
           str_eq(name, "codegen_error_at") ||
           str_eq(name, "codegen_error_at_location") ||
           str_eq(name, "import_error");
}
```

如果 Nova 当前不支持 `||` 换行风格，按现有代码风格写成嵌套 `if`。

---

## 22. stmt_always_exits

新增：

```nova
fn expr_is_no_return_call(expr: ParseNode) : bool
fn stmt_always_exits(stmt: ParseNode) : bool
fn block_always_exits(block: ParseNode) : bool
```

规则：

```text
ReturnStmt -> true
ExprStmt(CallExpr no-return) -> true
BlockStmt -> block_always_exits
IfStmt -> then exits && else exists && else exits
WhileStmt -> false
其他 -> false
```

### ExprStmt no-return

```text
ExprStmt
  CallExpr name=codegen_error
```

应该返回 true。

### IfStmt

```text
if (...) {
    return ...
} else {
    checker_error(...)
}
```

应该返回 true。

### WhileStmt

先保守返回 false。

不做复杂分析：

```text
while (true) { return ... }
```

---

## 23. block_always_exits

规则：

```text
空 block -> false
从前到后扫描 statement
如果某个 stmt_always_exits -> true
扫描完都没有 -> false
```

也就是说：

```nova
{
    let x : int = 1;
    return x;
    let y : int = 2;
}
```

看到 `return x` 后就可以判定 block exits。

Step 22 不要求 unreachable code warning。

---

## 24. 函数 missing-return 检查

在 checker 检查函数体时增加：

```text
如果 function return type != void
并且 body block 不 always exits
则报 missing return
```

错误文案可以参考 C++ sema 当前格式。

建议：

```text
CheckerError at line:column: missing return in non-void function
```

或者沿用 C++ sema 的 expected 文案。

---

## 25. 测试建议：Nova checker positive

### `tests/tools/typecheck/positive/noreturn_error_call.nv`

```nova
fn fail() : str {
    checker_error("failed");
}

fn main() : void {
    return;
}
```

### `.out`

```text
Check OK
Structs: 0
Enums: 0
Functions: 2
```

---

## 26. 测试建议：Nova checker positive if/else

### `tests/tools/typecheck/positive/noreturn_if_else.nv`

```nova
fn choose(x: bool) : str {
    if (x) {
        return "yes";
    } else {
        checker_error("no");
    }
}

fn main() : void {
    return;
}
```

### `.out`

```text
Check OK
Structs: 0
Enums: 0
Functions: 2
```

---

## 27. 测试建议：Nova checker negative missing return

### `tests/tools/typecheck/negative/missing_return.nv`

```nova
fn bad(x: bool) : int {
    if (x) {
        return 1;
    }
}

fn main() : void {
    return;
}
```

### `missing_return.err`

```text
CheckerError at 1:1: missing return in non-void function
```

具体位置按你 checker 当前 location 策略调整。

---

## 28. 注意不要扩大过多

Step 22 只需要基础 control-flow 检查，不需要：

```text
unreachable code warning
while(true) reasoning
break/continue
exception-like flow
definite assignment
```

---

# Part IV：Step 22 推荐实现顺序

---

## 29. 推荐顺序

### Step A：确认 stage1 codegen 可运行普通程序

先跑：

```bash
scripts/bootstrap_stage1.sh
```

然后用 `nova_codegen_stage1` 编译：

```text
hello
vec_struct_basic
recursion
```

如果失败，先修 codegen blocker。

---

### Step B：stage0 生成 frontend/checker/expr_parser

用 `nova_codegen_stage0` 生成：

```text
nova_frontend_stage1
nova_checker_stage1
nova_expr_parser_stage1
```

如果失败，修对应 codegen parity gap。

---

### Step C：运行 stage-generated tools

跑：

```text
frontend tokens / parse / check
checker representative tests
expr_parser representative tests
```

---

### Step D：实现 `str + str`

按顺序同步：

```text
C++ sema
C++ codegen
Nova checker
Nova codegen
tests
```

---

### Step E：实现 Nova checker no-return / missing-return

新增：

```text
stmt_always_exits
block_always_exits
missing return check
```

并加 positive / negative tests。

---

### Step F：全量测试

最后跑：

```bash
cmake --build build --parallel
ctest --test-dir build --output-on-failure
scripts/bootstrap_stage1.sh
```

---

## 30. 验收标准

### 合格

```text
stage1 nova_codegen 能编译 hello.nv
stage0 能生成 nova_frontend_stage1
stage0 能生成 nova_checker_stage1
stage0 能生成 nova_expr_parser_stage1
```

### 良好

```text
nova_frontend_stage1 tokens / parse / check 可运行
nova_checker_stage1 可运行代表性 typecheck 输入
nova_expr_parser_stage1 可运行代表性 parser 输入
str + str 在 C++ compiler 和 Nova tools 中都支持
```

### 优秀

```text
Nova checker 支持 no-return / missing-return 基础控制流检查
不再需要 dummy return
新增相关 tests
全量 CTest 通过
bootstrap_stage1 脚本通过
```

---

## 31. 完成标志

Step 22 完成时，应该可以说：

```text
stage-generated Nova compiler tools are usable on representative inputs.
```

至少这些流程应该成功：

```bash
scripts/bootstrap_stage1.sh
```

```bash
WORK="${TMPDIR:-/tmp}/nova_bootstrap_stage1"

"$WORK/nova_codegen_stage1" tests/tools/codegen/hello.nv /tmp/hello.stage1.c
cc /tmp/hello.stage1.c runtime/nova_runtime.c -I runtime -o /tmp/hello.stage1
/tmp/hello.stage1
```

```bash
"$WORK/nova_codegen_stage0" tools/nova_frontend.nv /tmp/nova_frontend_stage1.c
cc /tmp/nova_frontend_stage1.c runtime/nova_runtime.c -I runtime -o /tmp/nova_frontend_stage1

/tmp/nova_frontend_stage1 check tools/nova_checker.nv /tmp/check.stage1.out
head -20 /tmp/check.stage1.out
```

期望：

```text
Check OK
```

这标志着 Phase 2 从“能生成下一代 codegen”推进到“stage-generated compiler tools 可实际使用”。
