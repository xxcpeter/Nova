# Nova Project — Step 11
## 题目：用 Nova 实现 Parser Prototype：`vec<Token>` -> parse trace / diagnostics

---

## 1. 本次作业目标

在 Step 10 中，你已经把 Nova lexer 升级成了真正的 tokenizer library：

```text
source -> tokenize(source) -> vec<Token>
```

Step 11 的目标是：

> **用 Nova 写第一个 parser prototype。**

这一步暂时不要求构造完整 AST，也不要求替换 C++ parser。你的目标是先证明：

- Nova 可以读取 `vec<Token>`
- Nova 可以维护 parser position
- Nova 可以做 recursive descent parsing
- Nova 可以识别 Nova 的核心语法结构
- Nova 可以输出稳定 parse trace 或 parse diagnostics

也就是说，Step 11 的核心链路是：

```text
source
  -> tokenize(source)
  -> parse_program(tokens)
  -> parse trace / success / error
```

---

## 2. 本阶段核心意义

Step 11 是 Nova self-hosting 路线里的第二个真正编译器组件。

Step 10 解决了：

```text
字符流 -> token stream
```

Step 11 要开始解决：

```text
token stream -> syntax structure
```

但因为 Nova 目前还没有 enum / tagged union / inheritance / nullable user type 等能力，直接在 Nova 里构造完整 AST 会比较笨重。

所以这一步选择一个更稳的目标：

> **先实现 parser prototype，输出 parse trace。**

例如输入：

```nova
fn main() : void {
    return;
}
```

输出：

```text
Program
  Function name=main return=void
    Block
      ReturnStmt
```

这样你能验证 parser 的结构，而不用立刻解决完整 AST 表示问题。

---

## 3. 本阶段不做什么

本阶段明确不做：

- 完整 AST in Nova
- semantic analysis in Nova
- codegen in Nova
- replacing the C++ parser
- parser error recovery
- Pratt parser 完整重写
- enum / tagged union
- modules
- source span / multi-token range

本阶段只做：

> **用 Nova 写一个 recursive descent parser prototype，并输出 trace。**

---

## 4. 建议文件结构

建议新增：

```text
tools/
  nova_parser.nv
```

如果你想保留 Step 10 的 tokenizer：

```text
tools/
  nova_tokenizer.nv
  nova_parser.nv
```

测试建议新增：

```text
tests/tools/parser/
  minimal_function.nv
  minimal_function.out
  let_return.nv
  let_return.out
  if_else.nv
  if_else.out
  while_loop.nv
  while_loop.out
  struct_decl.nv
  struct_decl.out
  vec_decl.nv
  vec_decl.out
  missing_semicolon.nv
  missing_semicolon.err
```

---

## 5. 推荐输出格式：parse trace

本阶段不要输出 JSON，也不要试图复刻 C++ AST dump。

推荐使用简单缩进 trace：

```text
Program
  Function name=main return=void
    Block
      ReturnStmt
```

优点：

- Nova 用 `buf_*` 很容易生成
- golden test 容易比较
- 人眼可读
- 不需要复杂 AST 数据结构

---

## 6. Token 结构复用

沿用 Step 10 的：

```nova
struct Token {
    kind: str;
    lexeme: str;
    line: int;
    column: int;
}
```

Parser 输入：

```nova
fn parse_program(tokens: vec<Token>) : str
```

返回 parse trace 字符串。

也可以拆成：

```nova
fn parse_program(tokens: vec<Token>) : ParseResult
```

但 Step 11 初版不建议这么复杂。

---

## 7. Parser 状态设计

### 7.1 不推荐过度拆 `ParserState`

你可能会想定义：

```nova
struct ParserState {
    tokens: vec<Token>;
    pos: int;
    out: int;
}
```

但当前 Nova 的 struct 是按值传递，`pos` 是 `int`，如果你把 `ParserState` 传给 helper 并修改 `pos`，不会自动回写到调用者。

所以本阶段建议：

> **在 `parse_program` 或少数几个大函数中直接维护 `pos`。**

这和 Step 10 的 tokenizer 类似。

---

## 8. 两种实现路线

### 方案 A：单函数大 parser

在 `parse_program(tokens)` 里用一个大循环和一些局部变量完成解析。

优点：

- 最简单
- 没有状态传递问题

缺点：

- 函数会比较长
- 不像真正 parser

### 方案 B：有限 helper + 返回新 pos

每个 parse helper 返回新的 position。

例如：

```nova
fn parse_function(tokens: vec<Token>, pos: int, out: int) : int
```

表示：

- 输入当前 `pos`
- 把 trace 写进 `out`
- 返回 parse 后的新 `pos`

推荐本阶段使用 **方案 B**。

因为它避开了引用参数问题，又能保持 parser 结构清楚。

---

## 9. 推荐 helper 风格

### 9.1 token access

```nova
fn token_kind(tokens: vec<Token>, pos: int) : str
fn token_lexeme(tokens: vec<Token>, pos: int) : str
fn token_line(tokens: vec<Token>, pos: int) : int
fn token_column(tokens: vec<Token>, pos: int) : int
```

这些只是薄包装：

```nova
let t : Token = vec_get(tokens, pos);
return t.kind;
```

---

### 9.2 check / expect

```nova
fn check(tokens: vec<Token>, pos: int, kind: str) : bool
fn expect(tokens: vec<Token>, pos: int, kind: str, message: str) : int
```

`expect` 成功时返回 `pos + 1`。

失败时输出错误并终止：

```text
ParseError at 2:11: expected ';' after return statement
```

由于 Nova 当前没有异常，建议 runtime 增加或复用：

```nova
nova_runtime_error(message: str) : void
```

如果你不想让错误包含 line/column 拼接太复杂，也可以先输出：

```text
ParseError: expected SEMIC
```

但建议尽量包含 line/column。

---

### 9.3 trace output

```nova
fn emit_indent(out: int, indent: int) : void
fn emit_line(out: int, indent: int, text: str) : void
```

例如：

```nova
emit_line(out, 2, "ReturnStmt");
```

输出：

```text
    ReturnStmt
```

建议每一级缩进 2 个空格。

---

## 10. 本阶段需要 parse 的语法范围

Step 11 不要求完整表达式树，但要能跳过/识别表达式边界。

建议支持以下语法结构：

### 10.1 Program

```text
program -> top_decl* EOF
```

top-level 可以是：

- `struct` declaration
- `fn` declaration

---

### 10.2 Struct declaration

```nova
struct Point {
    x: int;
    y: int;
}
```

Trace：

```text
Struct name=Point
  Field name=x type=int
  Field name=y type=int
```

---

### 10.3 Function declaration

```nova
fn add(a: int, b: int) : int {
    return a + b;
}
```

Trace：

```text
Function name=add return=int
  Param name=a type=int
  Param name=b type=int
  Block
    ReturnStmt value
```

---

### 10.4 Type

需要识别：

- `int`
- `bool`
- `str`
- `void`
- struct type：`IDENT`
- vector type：`vec<T>`

Trace 中 type 可以直接输出源码形式：

```text
vec<int>
vec<Token>
```

---

### 10.5 Block

```nova
{
    stmt*
}
```

Trace：

```text
Block
  LetStmt name=x type=int
  ReturnStmt
```

---

### 10.6 Statements

建议支持：

- `let`
- `return`
- `if / else`
- `while`
- nested block
- expression statement

---

### 10.7 Let statement

```nova
let x : int = expr;
let tokens : vec<Token> = vec_new();
```

Trace：

```text
LetStmt name=x type=int
LetStmt name=tokens type=vec<Token>
```

可以不输出 initializer 的完整表达式树，先跳过到 `;` 即可。

---

### 10.8 Return statement

```nova
return;
return expr;
```

Trace：

```text
ReturnStmt
ReturnStmt value
```

如果 `return` 后直接是 `;`，输出 `ReturnStmt`。

否则跳过表达式到 `;`，输出 `ReturnStmt value`。

---

### 10.9 If statement

```nova
if (cond) {
    ...
} else {
    ...
}
```

Trace：

```text
IfStmt
  Then
    Block
      ...
  Else
    Block
      ...
```

Step 11 可以强制 `if/else` body 是 block，因为你当前 Nova examples 基本都这样写。

---

### 10.10 While statement

```nova
while (cond) {
    ...
}
```

Trace：

```text
WhileStmt
  Block
    ...
```

---

### 10.11 Expr statement

```nova
print_int(x);
x = x + 1;
vec_push(tokens, t);
```

Trace：

```text
ExprStmt
```

Step 11 初版可以不解析表达式内部，只需要跳过到 `;`。

---

## 11. 表达式处理策略

本阶段不需要构造完整表达式树。

建议先实现一个：

```nova
fn skip_expr_until_semic(tokens: vec<Token>, pos: int) : int
```

它从 `pos` 开始跳过 token，直到遇到顶层 `SEMIC`。

但要注意括号和 struct literal：

```nova
Point { x: 1, y: 2 };
foo(a, b);
```

所以要维护简单 nesting：

- `(` 增加 paren depth
- `)` 减少 paren depth
- `{` 增加 brace depth
- `}` 减少 brace depth

遇到 `SEMIC` 且 depth 都为 0 时停止。

---

## 12. 条件表达式跳过

对于：

```nova
if (x > 0) { ... }
while (i < n) { ... }
```

建议写：

```nova
fn skip_paren_expr(tokens: vec<Token>, pos: int) : int
```

要求当前 token 是 `LPAREN`，然后跳到匹配的 `RPAREN` 后面。

---

## 13. Type parser

建议实现：

```nova
fn parse_type_text(tokens: vec<Token>, pos: int) : TypeParseResult
```

但 Nova 现在没有 tuple。可以用 struct：

```nova
struct TypeParseResult {
    text: str;
    next_pos: int;
}
```

示例：

```nova
let r : TypeParseResult = parse_type_text(tokens, pos);
pos = r.next_pos;
```

这也是 Step 11 很好的 struct + vec 使用练习。

`parse_type_text` 支持：

```text
int
bool
str
void
Token
vec<int>
vec<Token>
```

---

## 14. Parse result

你可以让 `parse_program` 直接返回 trace string：

```nova
fn parse_program(tokens: vec<Token>) : str
```

如果成功，返回 trace。

如果失败，直接 runtime error。

这比设计完整 `ParseResult` 简单。

---

## 15. Tool main

建议工具入口：

```nova
fn main() : void {
    if (arg_count() != 3) {
        print_str("Usage: nova_parser <input.nv> <output.parse>");
        return;
    }

    let input_path : str = arg_get(1);
    let output_path : str = arg_get(2);

    let source : str = read_file(input_path);
    let tokens : vec<Token> = tokenize(source);
    let trace : str = parse_program(tokens);

    write_file(output_path, trace);
    return;
}
```

可以直接把 Step 10 的 tokenizer code 复制进 `nova_parser.nv`，或者后面再考虑模块化。

---

## 16. 测试方式

继续复用 `RunNovaToolTest.cmake`。

只需要把工具源改成：

```text
tools/nova_parser.nv
```

输入是 `.nv`，期望是 `.out` 或 `.parse`。

建议文件结构：

```text
tests/tools/parser/
  minimal_function.nv
  minimal_function.out
  let_return.nv
  let_return.out
  if_else.nv
  if_else.out
  while_loop.nv
  while_loop.out
  struct_decl.nv
  struct_decl.out
  vec_decl.nv
  vec_decl.out
```

---

## 17. 推荐测试用例

### 17.1 `tests/tools/parser/minimal_function.nv`

```nova
fn main() : void {
    return;
}
```

### `tests/tools/parser/minimal_function.out`

```text
Program
  Function name=main return=void
    Block
      ReturnStmt
```

---

### 17.2 `tests/tools/parser/let_return.nv`

```nova
fn id(x: int) : int {
    let y : int = x;
    return y;
}
```

### `tests/tools/parser/let_return.out`

```text
Program
  Function name=id return=int
    Param name=x type=int
    Block
      LetStmt name=y type=int
      ReturnStmt value
```

---

### 17.3 `tests/tools/parser/if_else.nv`

```nova
fn choose(flag: bool) : int {
    if (flag) {
        return 1;
    } else {
        return 2;
    }
}
```

### `tests/tools/parser/if_else.out`

```text
Program
  Function name=choose return=int
    Param name=flag type=bool
    Block
      IfStmt
        Then
          Block
            ReturnStmt value
        Else
          Block
            ReturnStmt value
```

---

### 17.4 `tests/tools/parser/while_loop.nv`

```nova
fn main() : void {
    let i : int = 0;
    while (i < 3) {
        i = i + 1;
    }
    return;
}
```

### `tests/tools/parser/while_loop.out`

```text
Program
  Function name=main return=void
    Block
      LetStmt name=i type=int
      WhileStmt
        Block
          ExprStmt
      ReturnStmt
```

---

### 17.5 `tests/tools/parser/struct_decl.nv`

```nova
struct Token {
    kind: str;
    line: int;
    column: int;
}

fn main() : void {
    return;
}
```

### `tests/tools/parser/struct_decl.out`

```text
Program
  Struct name=Token
    Field name=kind type=str
    Field name=line type=int
    Field name=column type=int
  Function name=main return=void
    Block
      ReturnStmt
```

---

### 17.6 `tests/tools/parser/vec_decl.nv`

```nova
struct Token {
    kind: str;
}

fn main() : void {
    let tokens : vec<Token> = vec_new();
    vec_push(tokens, Token { kind: "fn" });
    return;
}
```

### `tests/tools/parser/vec_decl.out`

```text
Program
  Struct name=Token
    Field name=kind type=str
  Function name=main return=void
    Block
      LetStmt name=tokens type=vec<Token>
      ExprStmt
      ReturnStmt
```

---

### 17.7 `tests/tools/parser/nested_blocks.nv`

```nova
fn main() : void {
    let x : int = 1;
    {
        let x : int = 2;
        print_int(x);
    }
    return;
}
```

### `tests/tools/parser/nested_blocks.out`

```text
Program
  Function name=main return=void
    Block
      LetStmt name=x type=int
      Block
        LetStmt name=x type=int
        ExprStmt
      ReturnStmt
```

---

## 18. Negative parser tests

如果你想做负例，建议先从很少几个开始。因为当前 Nova parser prototype 直接 runtime error，测试 stderr 会依赖 runtime 格式。

### 18.1 `tests/tools/parser/missing_semicolon.nv`

```nova
fn main() : void {
    return
}
```

### `tests/tools/parser/missing_semicolon.err`

```text
Nova runtime error: ParseError at 3:1: expected ';' after return statement
```

---

### 18.2 `tests/tools/parser/missing_rbrace.nv`

```nova
fn main() : void {
    return;
```

### `tests/tools/parser/missing_rbrace.err`

```text
Nova runtime error: ParseError at 3:1: expected '}' after block
```

负例可以晚一点接。Step 11 重点还是正例 trace。

---

## 19. CMake hook

可以基于已有 `RunNovaToolTest.cmake` 加一个新 helper：

```cmake
function(add_nova_parser_tool_test name)
    add_test(
        NAME nova_tool_parser_${name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/nova_parser.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/parser/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/parser/${name}.out
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/parser_${name}
            -DCC=cc
            -P ${NOVA_TOOL_TEST_DRIVER}
    )
    set_tests_properties(nova_tool_parser_${name} PROPERTIES LABELS "nova_tool;parser")
endfunction()

add_nova_parser_tool_test(minimal_function)
add_nova_parser_tool_test(let_return)
add_nova_parser_tool_test(if_else)
add_nova_parser_tool_test(while_loop)
add_nova_parser_tool_test(struct_decl)
add_nova_parser_tool_test(vec_decl)
add_nova_parser_tool_test(nested_blocks)
```

---

## 20. 推荐实现顺序

### Step A：复制 tokenizer library

先让 `nova_parser.nv` 能调用：

```nova
tokenize(source)
```

并复用 `Token` struct。

### Step B：实现 trace helper

实现：

```nova
emit_line(out, indent, text)
```

先输出固定 `Program` 也可以。

### Step C：实现 token helper

实现：

```nova
check
expect
token_kind
token_lexeme
```

### Step D：实现 parse_type_text

先支持 builtin type，再支持 `vec<T>`。

### Step E：实现 parse_function

先让 minimal function 通过。

### Step F：实现 parse_block / parse_stmt

支持 return、let、expr stmt。

### Step G：实现 if / while / struct decl

支持主要语法结构。

### Step H：补测试

逐个加 test，不要一次性全写。

---

## 21. 本阶段最容易犯的错误

### 错误 1：试图构造完整 AST

现在 Nova 缺少 enum/tagged union，完整 AST 会很别扭。先输出 trace。

### 错误 2：helper 修改 pos 但没有返回

Nova 当前没有引用参数，所以所有移动 parser position 的函数都应该返回新 pos。

### 错误 3：表达式跳过没有处理嵌套

`foo(Point { x: 1 });` 这种表达式里有括号和大括号，跳到 `;` 时要维护 depth。

### 错误 4：type parser 处理不了 `vec<Token>`

Step 11 测试应包含 vector type。

### 错误 5：trace 格式不稳定

缩进和文本一旦定下来，不要频繁改，否则 golden test 很难维护。

---

## 22. 验收标准

### 合格

- `nova_parser.nv` 能编译运行
- 能 parse minimal function
- 能输出稳定 trace
- basic tests 通过

### 良好

- 支持 struct / function / block / let / return / if / while
- 支持 `vec<T>` type trace
- 能跳过常见表达式
- 多个 parser tool tests 通过

### 优秀

- 错误信息包含 line/column
- 负例测试可运行
- parser code 结构清楚
- 可以作为 Step 12 的基础

---

## 23. 完成标志

Step 11 完成的标志是：

> Nova 可以用自己写的 tokenizer + parser prototype 识别 Nova 程序结构。

也就是说：

```text
Nova source
  -> Nova tokenizer
  -> vec<Token>
  -> Nova parser prototype
  -> parse trace
```

这是 Nova frontend 自举路线的重要节点。

---

## 24. 后续路线更新

当前进度：

```text
Step 1   Nova v0 spec / project setup
Step 2   C++ lexer
Step 3   C++ parser / AST
Step 4   semantic analysis
Step 5   C codegen
Step 6   runtime / core library
Step 7   Nova-written lexer dump tool
Step 8   struct
Step 9   typed vec<T>
Step 10  Nova tokenizer library
Step 11  Nova parser prototype
```

建议总路线仍然控制在约 15 步：

```text
Step 12  增加 enum / tagged union 或 token kind int 化，改善 AST/parser 表达能力
Step 13  Nova parser 从 trace 升级为简化 AST / parse tree
Step 14  Nova frontend 做 basic semantic checks 或生成简化 C
Step 15  bootstrap milestone：Nova 工具链处理 Nova 子集源码，与 C++ compiler 对照
```

如果 Step 11 做完后你发现 parser 因为缺 enum/tagged union 很别扭，Step 12 就优先实现 enum/tagged union。

如果 parser prototype 写起来还算顺，Step 12 也可以选择实现 `Option<T>` / nullable struct handle，但 enum/tagged union 对编译器项目更有价值。
