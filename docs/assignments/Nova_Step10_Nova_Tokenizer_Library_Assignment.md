# Nova Project — Step 10
## 题目：用 Nova 实现真正的 Tokenizer Library

---

## 1. 本次作业目标

在 Step 7 中，你已经用 Nova 写了一个 lexer/token dump tool。它可以读取 Nova 源文件并直接输出 token 文本。

在 Step 8 和 Step 9 中，Nova 又获得了：

- `struct`
- field access
- field assignment
- `vec<T>`
- `vec_push / vec_get / vec_len / vec_set`

所以 Step 10 的目标是：

> **把 Step 7 的直接输出式 lexer，升级成真正返回 `vec<Token>` 的 tokenizer library。**

也就是说，Step 7 是：

```text
source -> scan -> write token dump
```

Step 10 要变成：

```text
source -> tokenize() -> vec<Token> -> dump_tokens()
```

这一步完成后，Nova 就拥有了后续 parser prototype 所需的输入层。

---

## 2. 本阶段结束时你应该具备什么

完成 Step 10 后，你应该能写：

```nova
let tokens : vec<Token> = tokenize(source);
print_int(vec_len(tokens));
print_str(vec_get(tokens, 0).lexeme);
```

并且仍然可以提供一个工具入口：

```bash
nova_tokenize input.nv output.tok
```

它内部不是边扫描边输出，而是：

```text
read file
-> tokenize(source)
-> dump_tokens(tokens)
-> write file
```

---

## 3. 本阶段核心意义

Step 10 是从“工具脚本”到“编译器组件”的过渡。

之前的 Nova lexer 只是能产生文本输出；现在你要让 Nova 程序能在内存里保存 token list。

这是后续这些任务的基础：

- Nova parser prototype
- Nova formatter
- Nova syntax checker
- Nova self-hosting compiler frontend

---

## 4. 本阶段不做什么

本阶段不做：

- parser
- AST in Nova
- enum 语法
- pattern matching
- error recovery
- token ownership / allocator
- replacing the C++ lexer
- perfect compatibility with C++ token dump

本阶段只做：

> **Nova-written tokenizer library producing `vec<Token>`.**

---

## 5. 建议文件结构

新增或重构：

```text
tools/
  nova_tokenizer.nv
```

或者如果你想保留 Step 7 的工具：

```text
tools/
  nova_lexer.nv          # Step 7 old direct-dump tool
  nova_tokenizer.nv      # Step 10 new token-list tool
```

测试可以放在：

```text
tests/tools/tokenizer/
  basic.nv
  basic.tok
  expressions.nv
  expressions.tok
  strings.nv
  strings.tok
  comments.nv
  comments.tok
  struct_vec.nv
  struct_vec.tok
```

---

## 6. 推荐 Token 表示

Nova 当前没有 enum，所以 token kind 先继续用 `str` 或 `int`。

### 方案 A：kind 使用 `str`

```nova
struct Token {
    kind: str;
    lexeme: str;
    line: int;
    column: int;
}
```

优点：

- dump 简单
- debug 友好
- 不需要 token kind table

缺点：

- 比 int 慢
- 后续 parser 判断 kind 会频繁做字符串比较

### 方案 B：kind 使用 `int`

```nova
struct Token {
    kind: int;
    lexeme: str;
    line: int;
    column: int;
}
```

优点：

- 更接近真实 compiler
- parser 判断更方便

缺点：

- 需要手写 token kind 常量函数
- dump 时要把 int 转回 kind name

### 本阶段推荐

为了快速完成 Step 10，建议先用 **方案 A：kind: str**。

后面如果要写 Nova parser，再考虑把 kind 换成 int。

---

## 7. Token struct

建议定义：

```nova
struct Token {
    kind: str;
    lexeme: str;
    line: int;
    column: int;
}
```

建议 helper：

```nova
fn make_token(kind: str, lexeme: str, line: int, column: int) : Token {
    return Token { kind: kind, lexeme: lexeme, line: line, column: column };
}
```

---

## 8. Tokenizer API

建议核心函数：

```nova
fn tokenize(source: str) : vec<Token>
```

行为：

- 输入完整源代码字符串
- 返回 token vector
- 最后包含 EOF token

示例：

```nova
let tokens : vec<Token> = tokenize(source);
```

---

## 9. Dump API

建议从 token list 生成 dump 文本：

```nova
fn dump_tokens(tokens: vec<Token>) : str
```

输出格式继续使用 Step 7 的：

```text
KIND|lexeme|line|column
```

例如：

```text
KW_FN|fn|1|1
IDENT|main|1|4
EOF||4|1
```

---

## 10. Tool main

工具入口：

```nova
fn main() : void {
    if (arg_count() != 3) {
        print_str("Usage: nova_tokenizer <input.nv> <output.tok>");
        return;
    }

    let input_path : str = arg_get(1);
    let output_path : str = arg_get(2);

    let source : str = read_file(input_path);
    let tokens : vec<Token> = tokenize(source);
    let output : str = dump_tokens(tokens);

    write_file(output_path, output);
    return;
}
```

---

## 11. 推荐内部结构

因为 Nova 现在还没有 mutable global state、class、method，所以 tokenizer 内部可以先保持普通函数风格。

### 最小可行写法

在 `tokenize(source)` 里维护：

```nova
let tokens : vec<Token> = vec_new();
let pos : int = 0;
let line : int = 1;
let column : int = 1;
```

然后循环扫描。

不过 Nova 的参数是按值传递，目前没有引用参数，所以如果你想把 `pos/line/column` 拆到 helper 函数里，会比较麻烦。

因此本阶段可以接受：

> `tokenize()` 是一个较长函数。

这是合理的。

---

## 12. 可选：LexerState struct

如果你希望结构更清楚，可以定义：

```nova
struct LexerState {
    source: str;
    pos: int;
    line: int;
    column: int;
    tokens: vec<Token>;
}
```

但要注意：

- 现在 struct 是按值复制。
- 如果你把 `LexerState` 传给函数并修改字段，不一定会影响调用者。
- `tokens` 是 handle semantics，所以 vector 本体能共享，但 `pos/line/column` 是 int，传参后修改不会回写。

所以在没有引用 / pointer / mutable borrow 之前，不建议过度拆分 state-mutating helper。

### 推荐策略

- `is_digit / is_alpha / keyword_kind` 这类纯函数可以拆。
- 修改 `pos/line/column` 的扫描逻辑先留在 `tokenize()` 主循环里。

---

## 13. 字符 helper

建议保留 Step 7 的 helper：

```nova
fn is_digit(c: int) : bool
fn is_alpha(c: int) : bool
fn is_alnum(c: int) : bool
fn is_ident_start(c: int) : bool
fn is_ident_continue(c: int) : bool
fn is_whitespace(c: int) : bool
```

仍然使用 ASCII int。

---

## 14. Keyword helper

建议：

```nova
fn keyword_kind(lexeme: str) : str
```

示例：

```nova
if (str_eq(lexeme, "fn")) {
    return "KW_FN";
}
if (str_eq(lexeme, "let")) {
    return "KW_LET";
}
...
return "IDENT";
```

需要覆盖：

- `fn`
- `let`
- `if`
- `else`
- `while`
- `return`
- `struct`
- `vec`
- `true`
- `false`
- `int`
- `bool`
- `str`
- `void`

---

## 15. Token emission

之前 Step 7 可能是直接：

```nova
emit_token(out, kind, lexeme, line, column);
```

Step 10 应改成：

```nova
vec_push(tokens, make_token(kind, lexeme, line, column));
```

也就是说：

```text
emit token text
```

变成：

```text
append Token struct
```

---

## 16. Dump tokens

`dump_tokens(tokens)` 可以这样组织：

```nova
fn dump_tokens(tokens: vec<Token>) : str {
    let out : int = buf_new();
    let i : int = 0;

    while (i < vec_len(tokens)) {
        let t : Token = vec_get(tokens, i);
        buf_push_str(out, t.kind);
        buf_push_str(out, "|");
        buf_push_str(out, t.lexeme);
        buf_push_str(out, "|");
        buf_push_int(out, t.line);
        buf_push_str(out, "|");
        buf_push_int(out, t.column);
        buf_push_str(out, "\n");
        i = i + 1;
    }

    return buf_to_str(out);
}
```

---

## 17. Scanner behavior

Token rules should match Step 7:

- whitespace skipped
- line comments skipped
- block comments skipped
- identifiers and keywords
- integer literals
- string literals with escape skipping
- single-character punctuation
- double-character operators first
- EOF token at end

新增 Step 9/Step 10 related token support:

- `KW_STRUCT`
- `KW_VEC`
- `DOT`
- `<`
- `>`

`<` and `>` are still comparison operators in expression context; tokenizer does not need to know type context.

---

## 18. Testing strategy

测试仍然使用 tool test：

```text
compile tools/nova_tokenizer.nv
run generated tool on input .nv
compare generated .tok
```

可以复用 Step 7 的 `RunNovaToolTest.cmake`，只需要把 `TOOL_SOURCE` 改成：

```text
tools/nova_tokenizer.nv
```

---

## 19. 推荐测试用例

### 19.1 `tests/tools/tokenizer/basic.nv`

```nova
fn main() : void {
    return;
}
```

### `tests/tools/tokenizer/basic.tok`

```text
KW_FN|fn|1|1
IDENT|main|1|4
LPAREN|(|1|8
RPAREN|)|1|9
COLON|:|1|11
KW_VOID|void|1|13
LBRACE|{|1|18
KW_RETURN|return|2|5
SEMIC|;|2|11
RBRACE|}|3|1
EOF||4|1
```

---

### 19.2 `tests/tools/tokenizer/struct_vec.nv`

```nova
struct Token {
    kind: str;
    line: int;
}

fn main() : void {
    let tokens : vec<Token> = vec_new();
    return;
}
```

### `tests/tools/tokenizer/struct_vec.tok`

```text
KW_STRUCT|struct|1|1
IDENT|Token|1|8
LBRACE|{|1|14
IDENT|kind|2|5
COLON|:|2|9
KW_STR|str|2|11
SEMIC|;|2|14
IDENT|line|3|5
COLON|:|3|9
KW_INT|int|3|11
SEMIC|;|3|14
RBRACE|}|4|1
KW_FN|fn|6|1
IDENT|main|6|4
LPAREN|(|6|8
RPAREN|)|6|9
COLON|:|6|11
KW_VOID|void|6|13
LBRACE|{|6|18
KW_LET|let|7|5
IDENT|tokens|7|9
COLON|:|7|16
KW_VEC|vec|7|18
LT|<|7|21
IDENT|Token|7|22
GT|>|7|27
ASSIGN|=|7|29
IDENT|vec_new|7|31
LPAREN|(|7|38
RPAREN|)|7|39
SEMIC|;|7|40
KW_RETURN|return|8|5
SEMIC|;|8|11
RBRACE|}|9|1
EOF||10|1
```

---

### 19.3 `tests/tools/tokenizer/field_access.nv`

```nova
fn main() : void {
    print_int(token.line);
    token.line = token.line + 1;
    return;
}
```

### `tests/tools/tokenizer/field_access.tok`

```text
KW_FN|fn|1|1
IDENT|main|1|4
LPAREN|(|1|8
RPAREN|)|1|9
COLON|:|1|11
KW_VOID|void|1|13
LBRACE|{|1|18
IDENT|print_int|2|5
LPAREN|(|2|14
IDENT|token|2|15
DOT|.|2|20
IDENT|line|2|21
RPAREN|)|2|25
SEMIC|;|2|26
IDENT|token|3|5
DOT|.|3|10
IDENT|line|3|11
ASSIGN|=|3|16
IDENT|token|3|18
DOT|.|3|23
IDENT|line|3|24
PLUS|+|3|29
INT_LITERAL|1|3|31
SEMIC|;|3|32
KW_RETURN|return|4|5
SEMIC|;|4|11
RBRACE|}|5|1
EOF||6|1
```

---

### 19.4 `tests/tools/tokenizer/strings_comments.nv`

```nova
// comment
fn main() : void {
    print_str("a\n");
    /* block */ print_str("b\"");
    return;
}
```

### `tests/tools/tokenizer/strings_comments.tok`

```text
KW_FN|fn|2|1
IDENT|main|2|4
LPAREN|(|2|8
RPAREN|)|2|9
COLON|:|2|11
KW_VOID|void|2|13
LBRACE|{|2|18
IDENT|print_str|3|5
LPAREN|(|3|14
STR_LITERAL|"a\n"|3|15
RPAREN|)|3|20
SEMIC|;|3|21
IDENT|print_str|4|17
LPAREN|(|4|26
STR_LITERAL|"b\""|4|27
RPAREN|)|4|32
SEMIC|;|4|33
KW_RETURN|return|5|5
SEMIC|;|5|11
RBRACE|}|6|1
EOF||7|1
```

---

## 20. CMake test hook

如果复用 `RunNovaToolTest.cmake`，可以加：

```cmake
function(add_nova_tokenizer_tool_test name)
    add_test(
        NAME nova_tool_tokenizer_${name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/nova_tokenizer.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/tokenizer/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/tokenizer/${name}.tok
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/tokenizer_${name}
            -DCC=cc
            -P ${NOVA_TOOL_TEST_DRIVER}
    )
    set_tests_properties(nova_tool_tokenizer_${name} PROPERTIES LABELS "nova_tool;tokenizer")
endfunction()

add_nova_tokenizer_tool_test(basic)
add_nova_tokenizer_tool_test(struct_vec)
add_nova_tokenizer_tool_test(field_access)
add_nova_tokenizer_tool_test(strings_comments)
```

---

## 21. 推荐实现顺序

### Step A：定义 Token struct

先写：

```nova
struct Token { ... }
fn make_token(...) : Token
```

### Step B：把 emit text 改成 vec_push Token

先让 `tokenize()` 返回 `vec<Token>`。

### Step C：实现 dump_tokens

把 `vec<Token>` 转回 `.tok` 文本。

### Step D：迁移 Step 7 的扫描逻辑

把原来的 direct dump scanner 改成 token vector scanner。

### Step E：增加 struct/vec/dot 测试

确保 tokenizer 能扫描 Step 8/9 的新语法。

### Step F：接 CTest

复用 tool test driver。

---

## 22. 本阶段最容易犯的错误

### 错误 1：过度拆分 mutable state

Nova 目前没有引用参数。不要把 `pos/line/column` 拆到很多会修改它们的 helper 里，否则修改不会回写。

### 错误 2：忘记 EOF token

`tokenize()` 最后必须 push EOF。

### 错误 3：Token vector 没有使用 `vec<Token>`

这一步的核心目标就是让 token 成为结构化数据，不要退回字符串 dump。

### 错误 4：dump_tokens 里 field access / vec_get codegen 出错

`vec_get(tokens, i).kind` 会测试 Step 9 的组合能力。

### 错误 5：keyword 表漏掉新关键字

必须包含：

- `struct`
- `vec`

---

## 23. 验收标准

### 合格

- 能定义 `Token`
- `tokenize(source)` 返回 `vec<Token>`
- `dump_tokens(tokens)` 输出正确
- basic tokenizer test 通过

### 良好

- 支持 struct / vec / field access token
- 支持 comments / strings
- 与 Step 7 lexer dump 基本一致
- CTest 自动化通过

### 优秀

- 代码结构清楚
- Token API 可复用
- 后续 parser 可以直接使用 `vec<Token>`
- 不再依赖 direct dump scanner

---

## 24. 完成标志

Step 10 完成的标志是：

> Nova 可以用 `struct Token` 和 `vec<Token>` 在内存中表示 token stream。

这意味着后面可以开始实现：

```text
vec<Token> -> parser state -> AST-like data
```

这是 Nova self-hosting frontend 的第二块基础。

