# Nova Project — Step 19
## 题目：用 Nova 实现 Tiny C Codegen Prototype

---

## 1. 当前进度

到 Step 18 结束时，项目已经完成了几个关键节点：

```text
C++ compiler:
  source_loader 支持 import "path.nv";
  nova_compile 可以编译 import-based Nova tools

Nova tools:
  lib/token.nv
  lib/tokenizer.nv
  lib/parse_tree.nv
  lib/parser.nv
  lib/checker.nv
  lib/source_loader.nv

tools:
  nova_frontend.nv
  nova_checker.nv
  nova_expr_parser.nv
  nova_tokenizer.nv
```

现在 Nova-written frontend 已经可以做到：

```text
source
  -> load_source_with_imports
  -> tokenize
  -> parse_program_tree
  -> check_program
```

Step 19 的目标是：

> **在 Nova 里写一个小型 C codegen prototype，把一个小 Nova 子集转换成 C。**

这一步不是替换 C++ codegen，而是证明：

```text
Nova-written frontend can start generating code.
```

---

## 2. 本次作业目标

新增：

```text
lib/codegen_c.nv
tools/nova_codegen.nv
```

实现：

```bash
nova_codegen input.nv output.c
```

内部流程：

```text
source
  -> load_source_with_imports
  -> tokenize
  -> parse_program_tree
  -> check_program
  -> gen_c_program
  -> write output.c
```

然后用 C compiler 编译 generated C，并运行测试。

---

## 3. 本阶段定位

Step 19 是一个 prototype backend，不是完整 compiler backend。

它应该支持一个明确的小子集：

```text
function definitions
main function
int / bool / str
let
assignment
return
expr statement
if
while
function call
basic binary / unary expressions
runtime builtin calls
```

可选支持：

```text
struct literal
struct field access
enum member
vec intrinsics
```

但建议不要一开始就把全部 Step 16 类型能力都搬进 codegen。先保证 tiny subset 稳定。

---

## 4. 本阶段不做什么

Step 19 不做：

- 完整替换 C++ codegen
- module namespace
- advanced optimization
- nested vec
- full struct/enum/vec backend
- source map / original file diagnostics
- memory management cleanup
- runtime deprecated API cleanup
- self-host compiler

这些留给 Step 20 或之后。

---

## 5. 新增文件结构

建议新增：

```text
lib/codegen_c.nv
tools/nova_codegen.nv
```

工具入口：

```nova
import "../lib/source_loader.nv";
import "../lib/tokenizer.nv";
import "../lib/parser.nv";
import "../lib/checker.nv";
import "../lib/codegen_c.nv";

fn main() : void {
    ...
}
```

---

## 6. `tools/nova_codegen.nv`

建议写成 thin wrapper：

```nova
import "../lib/source_loader.nv";
import "../lib/tokenizer.nv";
import "../lib/parser.nv";
import "../lib/checker.nv";
import "../lib/codegen_c.nv";

fn main() : void {
    if (arg_count() != 3) {
        print_str("Usage: nova_codegen <input.nv> <output.c>");
        return;
    }

    let input_path : str = arg_get(1);
    let output_path : str = arg_get(2);

    let source : str = load_source_with_imports(input_path);
    let tokens : vec<Token> = tokenize(source);
    let tree : ParseNode = parse_program_tree(tokens);

    check_program(tree);

    let c_source : str = gen_c_program(tree);
    write_file(output_path, c_source);
    return;
}
```

---

## 7. `lib/codegen_c.nv` 核心 API

建议对外只暴露：

```nova
fn gen_c_program(program: ParseNode) : str
```

内部 helper：

```nova
fn gen_c_function(func: ParseNode) : str
fn gen_c_block(block: ParseNode, indent: int) : str
fn gen_c_stmt(stmt: ParseNode, indent: int) : str
fn gen_c_expr(expr: ParseNode) : str
fn gen_c_type(type_name: str) : str
fn emit_indent(out: int, indent: int) : void
```

这里继续用 `buf_new / buf_push_str / buf_to_str` 做字符串构造。

---

## 8. 支持的类型

Step 19 初始支持：

```text
int  -> int
bool -> bool
str  -> const char*
void -> void
```

C output 需要：

```c
#include <stdbool.h>
#include "nova_runtime.h"
```

建议生成文件头：

```c
#include <stdbool.h>
#include "nova_runtime.h"

```

如果已有 runtime header 里已经 include 了 `stdbool.h`，重复 include 也没问题。

---

## 9. `gen_c_type`

建议：

```nova
fn gen_c_type(type_name: str) : str {
    if (str_eq(type_name, "int")) {
        return "int";
    }
    if (str_eq(type_name, "bool")) {
        return "bool";
    }
    if (str_eq(type_name, "str")) {
        return "const char*";
    }
    if (str_eq(type_name, "void")) {
        return "void";
    }

    codegen_error(str_concat("unsupported type in tiny codegen: ", type_name));
    return "";
}
```

本阶段如果遇到 `Point` / `Color` / `vec<int>`，可以先报：

```text
CodegenError: unsupported type in tiny codegen: Point
```

---

## 10. 函数声明与定义

为了支持 forward call / recursion，C codegen 需要先生成 prototypes，再生成 definitions。

Nova:

```nova
fn add(x: int, y: int) : int {
    return x + y;
}

fn main() : void {
    print_int(add(1, 2));
    return;
}
```

C:

```c
int add(int x, int y);

int add(int x, int y) {
    return (x + y);
}

int main(int argc, char** argv) {
    nova_runtime_init(argc, argv);
    print_int(add(1, 2));
    return 0;
}
```

注意：

- Nova `main() : void` 要特殊生成 C `int main(int argc, char** argv)`。
- C main 里调用 `nova_runtime_init(argc, argv);`
- Nova `return;` 在 main 里生成 `return 0;`
- 其他 void 函数里的 `return;` 生成 `return;`

---

## 11. Function prototype 生成

建议：

```nova
fn gen_c_function_signature(func: ParseNode, is_prototype: bool) : str
```

普通函数：

```text
<return_type> <name>(<params>)
```

main 特判：

```text
int main(int argc, char** argv)
```

参数：

```text
Param name=x type=int -> int x
Param name=s type=str -> const char* s
```

---

## 12. Block / statement 生成

### 12.1 BlockStmt

```text
Block
  stmt...
```

生成：

```c
{
    ...
}
```

或者函数体外层由 caller 写 `{}`，`gen_c_block_body` 只生成内部语句。两种都可以，保持一致即可。

推荐：

```nova
fn gen_c_block_body(block: ParseNode, indent: int, current_function_name: str, current_return_type: str) : str
```

---

### 12.2 LetStmt

ParseNode:

```text
LetStmt name=x type=int
  expr
```

C:

```c
int x = <expr>;
```

---

### 12.3 Assignment ExprStmt

Nova:

```nova
x = x + 1;
```

Parse tree:

```text
ExprStmt
  AssignExpr
    IdentifierExpr name=x
    BinaryExpr op=+
      IdentifierExpr name=x
      IntLiteralExpr value=1
```

C:

```c
x = (x + 1);
```

Since assignment is an expression, `ExprStmt` can simply generate:

```c
<expr>;
```

---

### 12.4 ReturnStmt

Nova:

```nova
return;
return x;
```

C:

```c
return;
return x;
```

For C `main`, empty return should be:

```c
return 0;
```

---

### 12.5 IfStmt

ParseNode:

```text
IfStmt
  Condition
    expr
  Then
    Block
  Else
    Block
```

C:

```c
if (<expr>) {
    ...
} else {
    ...
}
```

---

### 12.6 WhileStmt

ParseNode:

```text
WhileStmt
  Condition
    expr
  Block
```

C:

```c
while (<expr>) {
    ...
}
```

---

## 13. Expression generation

### 13.1 Literals

```text
IntLiteralExpr value=1   -> 1
StrLiteralExpr value=hi  -> "hi" or existing lexeme form
BoolLiteralExpr true     -> true
BoolLiteralExpr false    -> false
```

Use whatever tokenizer currently stores in `expr.value`.

If string literal lexeme already contains escaped content without quotes, add quotes.  
If it already contains quotes, do not double quote.

---

### 13.2 IdentifierExpr

```text
IdentifierExpr name=x -> x
```

---

### 13.3 UnaryExpr

```text
UnaryExpr op=-
  expr
```

C:

```c
(-<expr>)
```

```text
UnaryExpr op=!
```

C:

```c
(!<expr>)
```

---

### 13.4 BinaryExpr

```text
BinaryExpr op=+
  left
  right
```

C:

```c
(<left> + <right>)
```

Supported ops:

```text
+ - * / %
< <= > >=
== !=
&& ||
```

---

### 13.5 AssignExpr

```text
AssignExpr
  target
  value
```

C:

```c
(<target> = <value>)
```

For statement expression:

```c
(x = (x + 1));
```

is acceptable.

---

### 13.6 CallExpr

```text
CallExpr name=print_int
  expr
```

C:

```c
print_int(<expr>)
```

For tiny codegen, runtime builtin calls and user function calls can share this path.

---

## 14. Optional expression features

These are optional in Step 19.

### 14.1 Struct field access

```nova
p.x
```

C:

```c
p.x
```

Only if you also support struct type generation.

### 14.2 Struct literal

```nova
Point { x: 1, y: 2 }
```

C compound literal:

```c
((Point){ .x = 1, .y = 2 })
```

Optional.

### 14.3 Enum member

```nova
Color.Red
```

C:

```c
Color_Red
```

Only if you also support enum generation.

### 14.4 Vec intrinsics

`vec_new / vec_push / vec_get / vec_set / vec_len` require type-specific helper names, so they are not recommended for the minimal Step 19.

You can defer vec codegen to after the tiny prototype works.

---

## 15. Codegen diagnostics

新增：

```nova
fn codegen_error(message: str) : void {
    nova_runtime_error(str_concat("CodegenError: ", message));
}

fn codegen_error_at(node: ParseNode, message: str) : void {
    nova_runtime_error(
        str_concat(
            "CodegenError at ",
            str_concat(
                int_to_str(node.line),
                str_concat(":", str_concat(int_to_str(node.column), str_concat(": ", message)))
            )
        )
    );
}
```

Examples:

```text
CodegenError: unsupported type in tiny codegen: Point
CodegenError at 2:13: unsupported expression in tiny codegen
```

---

## 16. Generated C runner

新增一个 runner：

```text
cmake/RunNovaCodegenToolTest.cmake
```

它做：

```text
1. compile tools/nova_codegen.nv -> nova_codegen.c
2. cc nova_codegen.c runtime/nova_runtime.c -> nova_codegen_exe
3. run nova_codegen_exe input.nv generated.c
4. cc generated.c runtime/nova_runtime.c -> generated_exe
5. run generated_exe
6. compare stdout with expected .out
```

这是 Step 19 的核心测试链路。

---

## 17. CMake helper

```cmake
set(NOVA_CODEGEN_TOOL_TEST_DRIVER
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunNovaCodegenToolTest.cmake
)

function(add_nova_codegen_tool_test name)
    add_test(
        NAME nova_tool_codegen_${name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/nova_codegen.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/codegen/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/codegen/${name}.out
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/codegen_${name}
            -DCC=cc
            -P ${NOVA_CODEGEN_TOOL_TEST_DRIVER}
    )

    set_tests_properties(
        nova_tool_codegen_${name}
        PROPERTIES LABELS "nova_tool;codegen"
    )
endfunction()
```

---

## 18. Positive tests

### 18.1 `tests/tools/codegen/hello.nv`

```nova
fn main() : void {
    print_str("hello");
    return;
}
```

### `tests/tools/codegen/hello.out`

```text
hello
```

---

### 18.2 `tests/tools/codegen/arithmetic.nv`

```nova
fn main() : void {
    let x : int = 1 + 2 * 3;
    print_int(x);
    return;
}
```

### `tests/tools/codegen/arithmetic.out`

```text
7
```

---

### 18.3 `tests/tools/codegen/function_call.nv`

```nova
fn add(x: int, y: int) : int {
    return x + y;
}

fn main() : void {
    print_int(add(20, 22));
    return;
}
```

### `tests/tools/codegen/function_call.out`

```text
42
```

---

### 18.4 `tests/tools/codegen/if_else.nv`

```nova
fn main() : void {
    let x : int = 3;
    if (x > 0) {
        print_str("positive");
    } else {
        print_str("non-positive");
    }
    return;
}
```

### `tests/tools/codegen/if_else.out`

```text
positive
```

---

### 18.5 `tests/tools/codegen/while_loop.nv`

```nova
fn main() : void {
    let i : int = 0;
    while (i < 3) {
        print_int(i);
        i = i + 1;
    }
    return;
}
```

### `tests/tools/codegen/while_loop.out`

```text
0
1
2
```

---

### 18.6 `tests/tools/codegen/bool_logic.nv`

```nova
fn main() : void {
    let a : bool = true;
    let b : bool = false;

    if (a && !b) {
        print_str("ok");
    }

    return;
}
```

### `tests/tools/codegen/bool_logic.out`

```text
ok
```

---

### 18.7 `tests/tools/codegen/string_concat.nv`

```nova
fn main() : void {
    let s : str = str_concat("hello", " world");
    print_str(s);
    return;
}
```

### `tests/tools/codegen/string_concat.out`

```text
hello world
```

---

### 18.8 `tests/tools/codegen/recursion.nv`

```nova
fn fact(n: int) : int {
    if (n <= 1) {
        return 1;
    }
    return n * fact(n - 1);
}

fn main() : void {
    print_int(fact(5));
    return;
}
```

### `tests/tools/codegen/recursion.out`

```text
120
```

---

## 19. Negative codegen tests

Optional for Step 19. If you add them, use a negative runner that expects `nova_codegen` to fail.

Example unsupported type:

### `tests/tools/codegen/negative/struct_not_supported.nv`

```nova
struct Point {
    x: int;
}

fn main() : void {
    let p : Point = Point { x: 1 };
    return;
}
```

### `tests/tools/codegen/negative/struct_not_supported.err`

```text
CodegenError: unsupported type in tiny codegen: Point
```

This is optional because Step 19 already has many moving parts.

---

## 20. Implementation order

### Step A：新增 `lib/codegen_c.nv`

先实现：

```nova
gen_c_program
gen_c_type
gen_c_expr for literals / identifiers / binary / call
```

### Step B：新增 `tools/nova_codegen.nv`

Thin wrapper：

```text
load -> tokenize -> parse -> check -> gen_c_program -> write
```

### Step C：生成 hello

只支持：

```nova
fn main() : void {
    print_str("hello");
    return;
}
```

跑通 generated C。

### Step D：支持 let / arithmetic / print_int

跑通 arithmetic。

### Step E：支持 function prototype / function call

跑通 function_call。

### Step F：支持 if / while / assignment

跑通 if_else / while_loop。

### Step G：支持 bool logic / recursion

跑通 bool_logic / recursion。

### Step H：接入 CMake runner

把手动流程变成 CTest。

---

## 21. 常见错误

### 错误 1：没有生成 function prototypes

递归和 forward call 会在 C 编译阶段失败。

### 错误 2：main 生成成 `void main()`

推荐生成：

```c
int main(int argc, char** argv)
```

并调用：

```c
nova_runtime_init(argc, argv);
```

### 错误 3：return; 在 main 中生成 `return;`

C main 应该：

```c
return 0;
```

### 错误 4：字符串 literal 双重加引号

如果 tokenizer value 已经是 `"hello"`，不要再生成 `""hello""`。

### 错误 5：表达式没加括号

尽量生成：

```c
(left op right)
```

避免优先级问题。

### 错误 6：没有 include runtime header

Generated C 应该 include：

```c
#include "nova_runtime.h"
```

### 错误 7：C 编译 generated C 时没有 `-I runtime`

runner 里要加：

```text
-I${RUNTIME_DIR}
```

---

## 22. 验收标准

### 合格

- 新增 `tools/nova_codegen.nv`
- 能生成 C 文件
- `hello / arithmetic / function_call` 通过
- CMake runner 跑通

### 良好

- 支持 if / while / assignment
- 支持 bool logic
- 支持 recursion
- generated C 可读

### 优秀

- 支持 basic str runtime calls
- negative unsupported feature diagnostics 稳定
- 和 C++ codegen 在 tiny subset 上 stdout 一致

---

## 23. 完成标志

Step 19 完成时，应该能跑：

```bash
nova_codegen tests/tools/codegen/hello.nv hello.c
cc hello.c runtime/nova_runtime.c -I runtime -o hello
./hello
```

输出：

```text
hello
```

并且 CTest 中有：

```text
nova_tool_codegen_hello
nova_tool_codegen_arithmetic
nova_tool_codegen_function_call
...
```

这说明 Nova-written toolchain 已经从 frontend 迈入 backend prototype。

下一步 Step 20 就可以作为最终 bootstrap milestone，把：

```text
C++ compiler compiles Nova tools
Nova frontend checks Nova tools
Nova codegen generates tiny C programs
```

整理成完整演示。
