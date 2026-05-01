# Nova Project — Step 5
## 题目：实现 Nova v0 的 C 后端（Code Generation to C）

---

## 1. 本次作业目标

在 Step 4 中，你已经完成了：

- lexer
- parser
- AST
- semantic analysis
- 类型检查
- 作用域检查
- return 路径检查

Step 5 的目标是：

> **把语义合法的 Nova AST 翻译成 C 代码，并通过 C 编译器生成可执行程序。**

这一步结束后，你应该能跑通完整流程：

```text
Nova source
  -> lexer
  -> parser
  -> sema
  -> C codegen
  -> C compiler
  -> executable
```

本阶段的重点不是生成漂亮 C，也不是优化，而是：

- AST 到 C 的结构映射清楚
- 生成代码稳定
- runtime 边界清楚
- 生成的 C 能被 `cc` / `clang` / `gcc` 编译
- 正例程序能实际运行并得到预期输出

---

## 2. 本阶段结束时你应该具备什么

完成 Step 5 后，你应该能：

- 对 `.nv` 文件生成 `.c` 文件
- 把生成的 `.c` 文件和 runtime 一起编译成可执行文件
- 正确处理 Nova v0 的函数、语句和表达式
- 支持 runtime 函数调用，如 `print_int`, `print_str`, `str_concat`
- 运行已有正例程序
- 对语义错误仍然在 codegen 前拦截

---

## 3. 本阶段的核心原则

### 原则 A：先直接从 AST 生成 C

本阶段暂时不要引入 IR。

也就是说，不做：

```text
AST -> IR -> C
```

而是直接做：

```text
AST -> C
```

这会让 Step 5 更直接、更容易完成。

---

### 原则 B：代码生成只处理“已经通过语义检查”的 AST

Codegen 不负责再次检查：

- 变量是否定义
- 类型是否匹配
- 函数是否存在
- return 是否正确

这些已经由 Step 4 处理。

Codegen 可以假设输入 AST 是语义合法的。

---

### 原则 C：runtime 继续用 C 提供

Nova v0 的 `str`、打印、文件 IO、字符串拼接等暂时由 C runtime 支持。

Codegen 只需要生成对这些 runtime 函数的调用。

---

## 4. 本阶段不做什么

Step 5 明确不做：

- 优化
- SSA
- LLVM
- 原生机器码生成
- 寄存器分配
- 控制流图
- 借用检查
- GC
- struct / array / module
- 复杂 name mangling
- 高级 runtime

如果生成的 C 比较啰嗦，也没关系。第一目标是正确。

---

## 5. 建议文件结构

建议新增：

```text
include/
  codegen_c.h

src/
  codegen_c.cpp
  nova_compile.cpp

runtime/
  nova_runtime.h
  nova_runtime.c
```

可选：

```text
tests/codegen/
  positive/
```

---

## 6. 模块职责

---

## 6.1 `codegen_c.h`

这个文件定义 C 后端的接口。

建议定义：

```text
class CCodeGenerator
```

它负责：

- 接收 `Program`
- 输出 C 源码字符串
- 或直接写入 `std::ostream`

推荐接口：

```text
std::string generate_c(const Program& program);
```

或者：

```text
void generate_c(const Program& program, std::ostream& os);
```

我更推荐第二种，因为输出 C 代码通常是流式写入。

---

### `CCodeGenerator` 建议字段

```text
std::ostream& out_;
int indent_level_;
```

可选：

```text
std::string current_function_name_;
```

---

### `CCodeGenerator` 建议函数

顶层：

```text
void generate(const Program& program);
```

声明 / 函数：

```text
void emit_preamble();
void emit_function(const FunctionDecl& function);
void emit_param_list(const std::vector<ParamDecl>& params);
```

语句：

```text
void emit_stmt(const Stmt& stmt);
void emit_block(const BlockStmt& block);
void emit_let_stmt(const LetStmt& stmt);
void emit_if_stmt(const IfStmt& stmt);
void emit_while_stmt(const WhileStmt& stmt);
void emit_return_stmt(const ReturnStmt& stmt);
void emit_expr_stmt(const ExprStmt& stmt);
```

表达式：

```text
void emit_expr(const Expr& expr);
void emit_assign_expr(const AssignExpr& expr);
void emit_binary_expr(const BinaryExpr& expr);
void emit_unary_expr(const UnaryExpr& expr);
void emit_call_expr(const CallExpr& expr);
void emit_identifier_expr(const IdentifierExpr& expr);
void emit_int_literal_expr(const IntLiteralExpr& expr);
void emit_str_literal_expr(const StrLiteralExpr& expr);
void emit_bool_literal_expr(const BoolLiteralExpr& expr);
```

辅助：

```text
std::string c_type(TypeKind type);
std::string c_binary_op(BinaryOp op);
std::string c_unary_op(UnaryOp op);
void indent();
void newline();
```

如果你的 AST 已经用了 visitor，也可以让 `CCodeGenerator` 实现一个 visitor。和 sema 一样，visitor 是很自然的做法。

---

## 6.2 `codegen_c.cpp`

这里实现所有 AST 到 C 的转换逻辑。

---

## 6.3 `nova_compile.cpp`

这是 Step 5 的命令行驱动程序。

它负责：

```text
read file
  -> lexer
  -> parser
  -> sema
  -> codegen C
  -> write .c file
```

建议先不要自动调用 `cc`，先只生成 C 文件。

例如：

```bash
nova_compile examples/positive/helloworld.nv -o build/helloworld.c
```

如果你暂时不想实现参数解析，可以先固定：

```bash
nova_compile input.nv output.c
```

最低要求：

```bash
nova_compile input.nv output.c
```

---

## 6.4 `runtime/nova_runtime.h`

声明 Nova runtime 函数。

建议初版：

```c
#pragma once

#include <stdbool.h>

void print_int(int x);
void print_str(const char* s);
bool str_eq(const char* a, const char* b);
char* str_concat(const char* a, const char* b);
char* read_file(const char* path);
void write_file(const char* path, const char* content);
```

---

## 6.5 `runtime/nova_runtime.c`

实现 runtime 函数。

最低要求必须实现：

- `print_int`
- `print_str`
- `str_eq`
- `str_concat`

`read_file` / `write_file` 可以先放简单实现，或在需要 bootstrap 时再完善。

---

## 7. Nova 类型到 C 类型的映射

建议映射：

| Nova | C |
|------|---|
| `int` | `int` |
| `bool` | `bool` |
| `str` | `const char*` 或 `char*` |
| `void` | `void` |

建议在 generated C 顶部包含：

```c
#include "nova_runtime.h"
```

因为 `bool` 和 runtime 函数都由 runtime header 提供。

---

## 8. 表达式生成规则

---

## 8.1 整数字面量

Nova:

```nova
123
```

C:

```c
123
```

---

## 8.2 布尔字面量

Nova:

```nova
true
false
```

C:

```c
true
false
```

---

## 8.3 字符串字面量

Nova:

```nova
"hello"
```

C:

```c
"hello"
```

因为 Nova lexer 保留的是原始 string lexeme，所以可以直接输出。

---

## 8.4 标识符

Nova:

```nova
x
```

C:

```c
x
```

---

## 8.5 一元表达式

Nova:

```nova
-x
!ok
```

C:

```c
(-x)
(!ok)
```

建议给表达式加括号，避免优先级问题。

---

## 8.6 二元表达式

Nova:

```nova
x + y
x < y
ok && flag
```

C:

```c
(x + y)
(x < y)
(ok && flag)
```

建议所有 binary expr 都输出括号。

---

## 8.7 赋值表达式

Nova:

```nova
x = expr
```

C:

```c
(x = expr)
```

---

## 8.8 函数调用

Nova:

```nova
foo(a, b)
```

C:

```c
foo(a, b)
```

runtime 函数也是普通函数调用。

---

## 9. 语句生成规则

---

## 9.1 BlockStmt

Nova:

```nova
{
    stmt;
}
```

C:

```c
{
    stmt;
}
```

---

## 9.2 LetStmt

Nova:

```nova
let x : int = 1;
```

C:

```c
int x = 1;
```

Nova:

```nova
let s : str = "hello";
```

C:

```c
const char* s = "hello";
```

---

## 9.3 ExprStmt

Nova:

```nova
foo();
```

C:

```c
foo();
```

---

## 9.4 IfStmt

Nova:

```nova
if (cond) {
    ...
} else {
    ...
}
```

C:

```c
if (cond) {
    ...
} else {
    ...
}
```

---

## 9.5 WhileStmt

Nova:

```nova
while (cond) {
    ...
}
```

C:

```c
while (cond) {
    ...
}
```

---

## 9.6 ReturnStmt

Nova:

```nova
return;
return expr;
```

C:

```c
return;
return expr;
```

---

## 10. 函数生成规则

Nova:

```nova
fn add(a: int, b: int) : int {
    return a + b;
}
```

C:

```c
int add(int a, int b) {
    return (a + b);
}
```

Nova:

```nova
fn main() : void {
    return;
}
```

C:

```c
int main(void) {
    return 0;
}
```

这里要特别注意：

Nova 的 `main : void` 最好生成 C 的：

```c
int main(void)
```

并在函数末尾补：

```c
return 0;
```

如果 Nova 源码里有 `return;`，在 C 的 `main` 里可以生成：

```c
return 0;
```

对于非 main 的 Nova `void` 函数，仍然生成 C 的 `void` 函数。

---

## 11. Runtime 函数处理

用户代码里调用：

```nova
print_int(x);
print_str(s);
str_concat(a, b);
```

C 里直接生成：

```c
print_int(x);
print_str(s);
str_concat(a, b);
```

只要 generated C 包含：

```c
#include "nova_runtime.h"
```

并且编译时链接：

```bash
cc generated.c runtime/nova_runtime.c -o program
```

即可。

---

## 12. 变量名和函数名

Nova v0 当前可以直接沿用原名。

暂时不做 name mangling。

也就是说：

```nova
fn add(...)
let x : int = ...
```

直接生成：

```c
int add(...)
int x = ...;
```

注意：Nova 关键字和 C 关键字目前有重合风险，比如用户变量名叫 `return` 不可能，因为它是 Nova keyword；但用户变量名叫 `printf` 理论上可能和 C 库冲突。v0 暂时不处理。

以后可以加统一前缀或 name mangling。

---

## 13. 代码生成风格建议

建议生成的 C 稳定、可读：

- 每层 block 缩进 4 个空格
- 每条语句一行
- binary / unary expression 加括号
- 函数之间空一行
- 顶部固定 include runtime header

例如：

```c
#include "nova_runtime.h"

int add(int a, int b) {
    return (a + b);
}

int main(void) {
    print_int(add(1, 2));
    return 0;
}
```

---

## 14. CMake 需要新增什么

新增 `nova_compile`：

```cmake
add_executable(nova_compile
    src/token.cpp
    src/lexer.cpp
    src/parser.cpp
    src/sema.cpp
    src/codegen_c.cpp
    src/nova_compile.cpp
)

target_include_directories(nova_compile
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(nova_compile
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)
```

如果你拆出了 `ast.cpp`、`ast_dump.cpp`，按需加入。

---

## 15. 建议测试结构

```text
tests/
└── codegen/
    ├── positive/
    │   ├── helloworld.nv
    │   ├── helloworld.out
    │   ├── arithmetic.nv
    │   ├── arithmetic.out
    │   ├── function_call.nv
    │   ├── function_call.out
    │   ├── recursion.nv
    │   ├── recursion.out
    │   ├── loop.nv
    │   ├── loop.out
    │   ├── string_concat.nv
    │   └── string_concat.out
```

Codegen 测试建议不是比较生成 C 文本，而是：

1. 编译 Nova 到 C
2. 用 C compiler 编译 generated C + runtime
3. 运行可执行文件
4. 比较程序 stdout

这比比较 generated C 更稳定。

---

## 16. 正例测试建议

---

### `helloworld.nv`

```nova
fn main() : void {
    print_str("hello world");
    return;
}
```

期望输出：

```text
hello world
```

---

### `arithmetic.nv`

```nova
fn main() : void {
    print_int(1 + 2 * 3);
    return;
}
```

期望输出：

```text
7
```

---

### `function_call.nv`

```nova
fn add(a: int, b: int) : int {
    return a + b;
}

fn main() : void {
    print_int(add(10, 32));
    return;
}
```

期望输出：

```text
42
```

---

### `recursion.nv`

```nova
fn fact(n: int) : int {
    if (n > 0) {
        return n * fact(n - 1);
    } else {
        return 1;
    }
}

fn main() : void {
    print_int(fact(5));
    return;
}
```

期望输出：

```text
120
```

---

### `loop.nv`

```nova
fn main() : void {
    let x : int = 3;
    while (x > 0) {
        print_int(x);
        x = x - 1;
    }
    return;
}
```

期望输出：

```text
3
2
1
```

---

### `string_concat.nv`

```nova
fn main() : void {
    let s : str = str_concat("a", "b");
    print_str(s);
    return;
}
```

期望输出：

```text
ab
```

---

## 17. Codegen 测试脚本建议

后面可以写一个 CMake script：

```text
RunCodegenTest.cmake
```

它做：

```text
nova_compile input.nv generated.c
cc generated.c runtime/nova_runtime.c -I runtime -o generated_exe
run generated_exe
compare stdout with .out
```

这比 parser/sema 的纯文本比较多一步，但更接近真实编译器测试。

---

## 18. 推荐实现顺序

---

### Step A：生成最小 C

先支持：

```nova
fn main() : void {
    return;
}
```

生成：

```c
#include "nova_runtime.h"

int main(void) {
    return 0;
}
```

---

### Step B：支持 literal 和 print

跑通：

```nova
print_int(1);
print_str("hello");
```

---

### Step C：支持表达式

跑通：

```nova
1 + 2 * 3
```

---

### Step D：支持 let 和 assignment

跑通：

```nova
let x : int = 1;
x = x + 1;
```

---

### Step E：支持 if / while

跑通条件和循环。

---

### Step F：支持函数调用和递归

跑通 `fact(5)`。

---

### Step G：接 codegen tests

用真实 C compiler 编译运行。

---

## 19. 验收标准

### 合格

- 能生成 C 文件
- C 文件能通过 `cc` 编译
- `helloworld` 和简单 arithmetic 可运行

### 良好

- 支持函数调用、递归、if、while
- 支持 runtime 字符串函数
- 有自动化 codegen tests

### 优秀

- 生成代码稳定可读
- main 特殊处理正确
- runtime 边界清晰
- 错误程序不会进入 codegen

---

## 20. 本阶段最容易犯的错误

### 错误 1：在 codegen 再做语义检查

不要这样。Codegen 假设 sema 已经通过。

---

### 错误 2：C main 生成成 `void main`

建议生成：

```c
int main(void)
```

---

### 错误 3：字符串拼接直接用 `+`

C 里不能这样。必须走 runtime：

```c
str_concat(a, b)
```

Nova 源码里本来就应使用 `str_concat`。

---

### 错误 4：表达式不加括号导致优先级问题

建议初版所有复合表达式都加括号。

---

### 错误 5：runtime header include 路径没处理好

生成 C 里写：

```c
#include "nova_runtime.h"
```

编译时加：

```bash
-I runtime
```

---

## 21. 完成标志

Step 5 完成的标志是：

> **Nova 正例程序可以经过完整编译链生成可执行文件，并运行得到正确输出。**

也就是：

```text
.nv -> .c -> executable -> expected stdout
```

---

## 22. 下一步预告

Step 6 可以做两件事之一：

1. 扩充 runtime，使 Nova 更适合写 compiler 本身
2. 开始把 codegen 或 lexer 的一部分用 Nova 重写，为 bootstrap 做准备
