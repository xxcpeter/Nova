# Nova Project — Step 13
## 题目：迁移 Nova tools 到 `TokenKind enum`，并把 parser trace 升级为最小 parse tree

---

## 1. 本次作业目标

在 Step 12 中，Nova 已经支持了 payload-less enum：

```nova
enum Color {
    Red;
    Green;
}
```

这一步让 Nova 可以更自然地表达编译器里的“有限类别”，例如 token kind。

Step 13 的目标是把这个能力真正用到 Nova tools 里，同时避免反复重写已有工具。

本阶段要完成两件事：

1. **把 `nova_tokenizer.nv` / `nova_parser.nv` 中的 `Token.kind: str` 迁移为 `Token.kind: TokenKind`。**
2. **把 parser 的输出从直接 trace 字符串，升级为一个最小 parse tree，再通过 dumper 输出 trace。**

也就是说，Step 11 是：

```text
source -> tokenize(source) -> vec<Token> -> parse_program(tokens) -> trace string
```

Step 13 要变成：

```text
source -> tokenize(source) -> vec<Token> -> parse_program_tree(tokens) -> ParseNode
                                                     |
                                                     v
                                               dump_parse_tree(node) -> trace string
```

为了兼容已有测试，外部仍然保留：

```nova
fn parse_program(tokens: vec<Token>) : str
```

但它内部调用新的 parse tree parser。

---

## 2. 为什么 Step 13 要包含 Step 12.5

Step 12.5 原本只是“把 tools 里的 token kind 从 str 迁移成 enum”。

但 Step 13 的 parser tree 也需要更稳定的 kind 表示，否则 tree node / parser 判断还是大量字符串比较。

所以本阶段合并处理：

```text
Step 12.5: TokenKind enum migration
Step 13: parser trace -> parse tree
```

这样 tools 只迁移一次，不反复重写。

---

## 3. 本阶段核心原则

本阶段仍然不做完整 Nova AST。

你要实现的是：

> **generic parse tree**

而不是：

> **typed AST**

也就是说，不要急着做：

```nova
struct FunctionDecl { ... }
struct LetStmt { ... }
struct Expr { ... }
```

而是先做一个通用节点：

```nova
struct ParseNode {
    kind: ParseNodeKind;
    name: str;
    value: str;
    children: vec<ParseNode>;
}
```

这样 Nova 在没有 tagged union 的情况下，也能表达层级结构。

---

## 4. 本阶段不做什么

本阶段明确不做：

- full AST
- semantic analysis in Nova
- codegen in Nova
- tagged union
- pattern matching
- parser error recovery
- expression AST
- module/import
- replacing C++ compiler frontend

本阶段只做：

```text
TokenKind enum migration
+
generic ParseNode tree
+
trace dumper compatibility
```

---

## 5. 推荐文件策略

现在 Nova 还没有 module/import，所以不要强行拆共享库。

继续保留：

```text
tools/
  nova_tokenizer.nv
  nova_parser.nv
```

建议做法：

- `nova_tokenizer.nv`：迁移为 enum TokenKind，仍然作为 tokenizer tool。
- `nova_parser.nv`：内部也使用 enum TokenKind，并包含 tokenizer 代码副本。
- 两个 tool 的外部输出保持不变。

等以后实现 module/import 后，再整理成：

```text
lib/
  token.nv
  tokenizer.nv
  parser.nv

tools/
  nova_tokenizer.nv
  nova_parser.nv
```

现在不要提前做这个拆分。

---

## 6. Part A：迁移 TokenKind enum

### 6.1 定义 TokenKind

建议在 `nova_tokenizer.nv` 和 `nova_parser.nv` 中都定义：

```nova
enum TokenKind {
    KW_FN;
    KW_LET;
    KW_IF;
    KW_ELSE;
    KW_WHILE;
    KW_RETURN;
    KW_STRUCT;
    KW_ENUM;
    KW_VEC;
    KW_INT;
    KW_BOOL;
    KW_STR;
    KW_VOID;
    KW_TRUE;
    KW_FALSE;

    IDENT;
    INT_LITERAL;
    STR_LITERAL;

    LPAREN;
    RPAREN;
    LBRACE;
    RBRACE;
    COMMA;
    SEMIC;
    COLON;
    DOT;

    ASSIGN;
    PLUS;
    MINUS;
    STAR;
    SLASH;
    PERCENT;

    LT;
    GT;
    LE;
    GE;
    EQEQ;
    NEQ;
    ANDAND;
    OROR;
    BANG;

    EOF;
}
```

命名可以和你现有 tokenizer 输出一致。重点是内部统一用 enum。

---

### 6.2 修改 Token struct

旧版：

```nova
struct Token {
    kind: str;
    lexeme: str;
    line: int;
    column: int;
}
```

新版：

```nova
struct Token {
    kind: TokenKind;
    lexeme: str;
    line: int;
    column: int;
}
```

---

### 6.3 make_token

```nova
fn make_token(kind: TokenKind, lexeme: str, line: int, column: int) : Token {
    return Token {
        kind: kind,
        lexeme: lexeme,
        line: line,
        column: column
    };
}
```

---

### 6.4 token_kind_name

为了保持 `.tok` 输出不变，新增：

```nova
fn token_kind_name(kind: TokenKind) : str {
    if (kind == TokenKind.KW_FN) {
        return "KW_FN";
    }
    if (kind == TokenKind.IDENT) {
        return "IDENT";
    }
    if (kind == TokenKind.EOF) {
        return "EOF";
    }
    return "UNKNOWN";
}
```

实际实现要覆盖全部 TokenKind。

这样内部用 enum，但外部 dump 仍然是：

```text
KW_FN|fn|1|1
IDENT|main|1|4
EOF||4|1
```

已有 tokenizer golden tests 可以基本不改。

---

### 6.5 classify_token

旧版：

```nova
fn classify_token(lexeme: str) : str
```

新版：

```nova
fn classify_token(lexeme: str) : TokenKind
```

例如：

```nova
if (str_eq(lexeme, "fn")) {
    return TokenKind.KW_FN;
}
if (str_eq(lexeme, "struct")) {
    return TokenKind.KW_STRUCT;
}
if (str_eq(lexeme, "enum")) {
    return TokenKind.KW_ENUM;
}
if (str_eq(lexeme, "vec")) {
    return TokenKind.KW_VEC;
}
return TokenKind.IDENT;
```

---

### 6.6 check 函数迁移

旧版：

```nova
fn check(tokens: vec<Token>, pos: int, kind: str) : bool {
    return str_eq(vec_get(tokens, pos).kind, kind);
}
```

新版：

```nova
fn check(tokens: vec<Token>, pos: int, kind: TokenKind) : bool {
    return vec_get(tokens, pos).kind == kind;
}
```

这会让 parser 判断 token 更接近真实 compiler。

---

## 7. Part B：新增 parse tree 表示

### 7.1 ParseNodeKind

建议定义：

```nova
enum ParseNodeKind {
    Program;
    StructDecl;
    EnumDecl;
    FieldDecl;
    EnumMember;
    FunctionDecl;
    ParamDecl;
    BlockStmt;
    LetStmt;
    ReturnStmt;
    IfStmt;
    ThenBranch;
    ElseBranch;
    WhileStmt;
    ExprStmt;
}
```

这不是语言 AST 的最终设计，只是 parse trace tree 的节点类别。

---

### 7.2 ParseNode

建议定义：

```nova
struct ParseNode {
    kind: ParseNodeKind;
    name: str;
    value: str;
    children: vec<ParseNode>;
}
```

字段含义：

- `kind`：节点类别
- `name`：主要名称，例如 function name、field name、param name
- `value`：辅助值，例如 return type、let type
- `children`：子节点

示例：

```text
FunctionDecl:
  name = "main"
  value = "void"

LetStmt:
  name = "tokens"
  value = "vec<Token>"
```

---

### 7.3 make_node

```nova
fn make_node(kind: ParseNodeKind, name: str, value: str) : ParseNode {
    let children : vec<ParseNode> = vec_new();
    return ParseNode {
        kind: kind,
        name: name,
        value: value,
        children: children
    };
}
```

---

### 7.4 add_child

因为 `vec<T>` 是 handle semantics，`children` 可以被修改：

```nova
fn add_child(parent: ParseNode, child: ParseNode) : ParseNode {
    vec_push(parent.children, child);
    return parent;
}
```

注意：Nova struct 是按值传递，所以这里最好返回 `parent`，让调用者写：

```nova
node = add_child(node, child);
```

虽然 `children` vector handle 本身会共享，但返回 parent 可以让语义更清楚，后续如果 struct copy 规则变化也更稳。

---

## 8. Parser API 迁移

保留旧接口：

```nova
fn parse_program(tokens: vec<Token>) : str
```

新增内部接口：

```nova
fn parse_program_tree(tokens: vec<Token>) : ParseNode
```

旧接口变成 wrapper：

```nova
fn parse_program(tokens: vec<Token>) : str {
    let tree : ParseNode = parse_program_tree(tokens);
    return dump_parse_tree(tree);
}
```

这样旧 tests 不需要大改。

---

## 9. parse helper 返回结构

当前 parser prototype 大概率有很多函数：

```nova
fn parse_function(tokens: vec<Token>, pos: int, out: int, indent: int) : int
```

Step 13 建议改成返回一个 parse result：

```nova
struct ParseResult {
    node: ParseNode;
    next_pos: int;
}
```

然后：

```nova
fn parse_function(tokens: vec<Token>, pos: int) : ParseResult
```

示例：

```nova
let result : ParseResult = parse_function(tokens, pos);
program = add_child(program, result.node);
pos = result.next_pos;
```

---

## 10. parse_program_tree

伪结构：

```nova
fn parse_program_tree(tokens: vec<Token>) : ParseNode {
    let program : ParseNode = make_node(ParseNodeKind.Program, "", "");
    let pos : int = 0;

    while (!check(tokens, pos, TokenKind.EOF)) {
        if (check(tokens, pos, TokenKind.KW_STRUCT)) {
            let r : ParseResult = parse_struct(tokens, pos);
            program = add_child(program, r.node);
            pos = r.next_pos;
        } else if (check(tokens, pos, TokenKind.KW_ENUM)) {
            let r : ParseResult = parse_enum(tokens, pos);
            program = add_child(program, r.node);
            pos = r.next_pos;
        } else if (check(tokens, pos, TokenKind.KW_FN)) {
            let r : ParseResult = parse_function(tokens, pos);
            program = add_child(program, r.node);
            pos = r.next_pos;
        } else {
            parse_error(tokens, pos, "expected top-level declaration");
        }
    }

    return program;
}
```

---

## 11. parse_struct

Input:

```nova
struct Token {
    kind: TokenKind;
    lexeme: str;
}
```

Parse tree:

```text
StructDecl name=Token
  FieldDecl name=kind value=TokenKind
  FieldDecl name=lexeme value=str
```

Node:

```nova
ParseNodeKind.StructDecl
name = "Token"
value = ""
```

Field:

```nova
ParseNodeKind.FieldDecl
name = "kind"
value = "TokenKind"
```

---

## 12. parse_enum

Input:

```nova
enum TokenKind {
    KW_FN;
    IDENT;
    EOF;
}
```

Parse tree:

```text
EnumDecl name=TokenKind
  EnumMember name=KW_FN
  EnumMember name=IDENT
  EnumMember name=EOF
```

Node:

```nova
ParseNodeKind.EnumDecl
name = "TokenKind"
value = ""
```

Member:

```nova
ParseNodeKind.EnumMember
name = "KW_FN"
value = ""
```

---

## 13. parse_function

Input:

```nova
fn main() : void {
    return;
}
```

Parse tree:

```text
FunctionDecl name=main value=void
  BlockStmt
    ReturnStmt
```

Node:

```nova
ParseNodeKind.FunctionDecl
name = "main"
value = "void"
```

Param:

```nova
ParseNodeKind.ParamDecl
name = "x"
value = "int"
```

---

## 14. parse_block / parse_stmt

`parse_block` 返回：

```nova
ParseResult {
    node: BlockStmt,
    next_pos: ...
}
```

`parse_stmt` 可以先支持：

- let
- return
- if
- while
- nested block
- expr stmt

每个 statement 都返回一个 `ParseResult`。

---

## 15. expression 仍然可以跳过

Step 13 不要求 expression tree。

继续保留：

```nova
fn skip_expr_until_semic(tokens: vec<Token>, pos: int) : int
fn skip_paren_expr(tokens: vec<Token>, pos: int) : int
```

然后：

```nova
ExprStmt
LetStmt
ReturnStmt value
```

仍然只记录 statement 级别结构。

这一步的重点是：

> parse tree for declarations/statements

不是 expression AST。

---

## 16. dump_parse_tree

新增：

```nova
fn dump_parse_tree(node: ParseNode) : str
```

内部可以用递归 helper：

```nova
fn dump_parse_node(out: int, node: ParseNode, indent: int) : void
```

如果 Nova 支持递归函数，直接递归遍历 children：

```nova
fn dump_parse_node(out: int, node: ParseNode, indent: int) : void {
    emit_indent(out, indent);
    buf_push_str(out, parse_node_header(node));
    buf_push_str(out, "\n");

    let i : int = 0;
    while (i < vec_len(node.children)) {
        dump_parse_node(out, vec_get(node.children, i), indent + 1);
        i = i + 1;
    }
}
```

---

## 17. Trace 输出兼容策略

为了让 Step 11 的 `.out` 测试尽量不改，`dump_parse_tree` 输出应尽量保持原格式。

例如：

```text
Program
  Function name=main return=void
    Block
      ReturnStmt
```

虽然内部 node 叫：

```text
FunctionDecl
BlockStmt
```

但 dumper 可以输出旧格式：

```text
Function name=main return=void
Block
```

这样旧 tests 可以继续用。

---

## 18. parse_node_header

建议：

```nova
fn parse_node_header(node: ParseNode) : str
```

示例映射：

```text
ParseNodeKind.Program      -> "Program"
StructDecl                 -> "Struct name=<name>"
EnumDecl                   -> "Enum name=<name>"
FieldDecl                  -> "Field name=<name> type=<value>"
EnumMember                 -> "Member name=<name>"
FunctionDecl               -> "Function name=<name> return=<value>"
ParamDecl                  -> "Param name=<name> type=<value>"
BlockStmt                  -> "Block"
LetStmt                    -> "LetStmt name=<name> type=<value>"
ReturnStmt, value=""       -> "ReturnStmt"
ReturnStmt, value="value"  -> "ReturnStmt value"
IfStmt                     -> "IfStmt"
ThenBranch                 -> "Then"
ElseBranch                 -> "Else"
WhileStmt                  -> "WhileStmt"
ExprStmt                   -> "ExprStmt"
```

---

## 19. Tests

### 19.1 Existing parser tests

已有 Step 11 parser tests 应尽量继续通过：

```text
minimal_function
let_return
if_else
while_loop
struct_decl
vec_decl
nested_blocks
```

如果输出格式保持兼容，基本不用改。

---

### 19.2 新增 enum parser test

#### `tests/tools/parser/enum_decl.nv`

```nova
enum TokenKind {
    KW_FN;
    IDENT;
    EOF;
}

fn main() : void {
    return;
}
```

#### `tests/tools/parser/enum_decl.out`

```text
Program
  Enum name=TokenKind
    Member name=KW_FN
    Member name=IDENT
    Member name=EOF
  Function name=main return=void
    Block
      ReturnStmt
```

---

### 19.3 tokenizer enum migration test

如果 tokenizer `.tok` 输出不变，原 tokenizer tests 应该继续通过。

建议新增一个包含 enum 的 tokenizer 输入：

#### `tests/tools/tokenizer/enum_decl.nv`

```nova
enum Color {
    Red;
    Green;
}

fn main() : void {
    let c : Color = Color.Red;
    return;
}
```

#### `tests/tools/tokenizer/enum_decl.tok`

```text
KW_ENUM|enum|1|1
IDENT|Color|1|6
LBRACE|{|1|12
IDENT|Red|2|5
SEMIC|;|2|8
IDENT|Green|3|5
SEMIC|;|3|10
RBRACE|}|4|1
KW_FN|fn|6|1
IDENT|main|6|4
LPAREN|(|6|8
RPAREN|)|6|9
COLON|:|6|11
KW_VOID|void|6|13
LBRACE|{|6|18
KW_LET|let|7|5
IDENT|c|7|9
COLON|:|7|11
IDENT|Color|7|13
ASSIGN|=|7|19
IDENT|Color|7|21
DOT|.|7|26
IDENT|Red|7|27
SEMIC|;|7|30
KW_RETURN|return|8|5
SEMIC|;|8|11
RBRACE|}|9|1
EOF||10|1
```

---

## 20. CMake additions

If you already have:

```cmake
add_nova_tokenizer_tool_test(...)
add_nova_parser_tool_test(...)
```

Add:

```cmake
add_nova_tokenizer_tool_test(enum_decl)
add_nova_parser_tool_test(enum_decl)
```

---

## 21. 推荐实现顺序

### Step A：迁移 tokenizer Token.kind

- 定义 `TokenKind`
- 修改 `Token.kind`
- 修改 `make_token`
- 修改 `classify_token`
- 添加 `token_kind_name`
- 确保 tokenizer tests 输出不变

### Step B：迁移 parser check/expect

- `check(..., kind: TokenKind)`
- `expect(..., kind: TokenKind, ...)`
- 所有调用点从 `"KW_FN"` 改成 `TokenKind.KW_FN`

### Step C：增加 ParseNodeKind / ParseNode / ParseResult

先写结构，不急着改全部 parser。

### Step D：parse_program_tree

让它返回 `ParseNode`，再用 dumper 输出原 trace。

### Step E：逐个迁移 parse helper

按顺序：

1. `parse_struct`
2. `parse_enum`
3. `parse_function`
4. `parse_block`
5. `parse_stmt`

每迁移一个，就跑一次 parser tests。

### Step F：新增 enum tests

确保 tokenizer 和 parser 都能识别 enum。

---

## 22. 本阶段最容易犯的错误

### 错误 1：TokenKind enum 化后 dump 输出变化

内部变 enum，不代表 `.tok` 输出要变。用 `token_kind_name()` 保持兼容。

### 错误 2：ParseNode 按值传递导致 children 修改丢失

因为 `children` 是 `vec<ParseNode>` handle，所以 `vec_push(node.children, child)` 会修改底层 vector。但仍建议 `add_child` 返回 node，调用者重新赋值。

### 错误 3：一次性迁移全部 parser

不要一次改完。先让 `parse_program_tree` 只处理 function，再逐步迁移。

### 错误 4：忘记 parser tool 中也要复制 TokenKind 定义

没有 module/import 前，`nova_parser.nv` 里也需要定义 tokenizer 相关结构。

### 错误 5：dump tree 和旧 trace 格式不一致

旧测试会失败。要么保持兼容，要么明确更新 golden files。

---

## 23. 验收标准

### 合格

- `Token.kind` 从 `str` 改成 `TokenKind`
- tokenizer tests 仍然通过
- parser 能通过 enum TokenKind 做 check
- parser trace 输出不变或只做明确小改

### 良好

- parser 内部生成 `ParseNode`
- `dump_parse_tree` 输出兼容 Step 11 trace
- 支持 enum declaration trace
- parser tests 自动化通过

### 优秀

- parser helper 全部返回 `ParseResult`
- parse tree 结构清晰
- 旧接口保留，新接口可用于 Step 14
- 没有整份重写 tools，而是小步迁移

---

## 24. 完成标志

Step 13 完成的标志是：

> Nova tools 内部已经使用 enum 表示 token kind，并且 parser prototype 不再只是直接输出 trace，而是先构造一个最小 parse tree，再 dump 成 trace。

这意味着后面可以继续从：

```text
parse trace
```

推进到：

```text
real parse tree
```

再推进到：

```text
semantic checks / lowering
```

而不需要反复推倒重写 tokenizer/parser。

---

## 25. 后续路线

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
Step 12  payload-less enum
Step 13  tools enum migration + parse tree
```

后续建议：

```text
Step 14  用 Nova parse tree 做 basic syntax-level checks / symbol collection prototype
Step 15  bootstrap milestone：Nova frontend tool 能处理 Nova 子集源码并与 C++ compiler 输出对照
```

如果 Step 13 做完后你发现 generic `ParseNode` 表达能力还不够，再考虑 Step 14 加 tagged union 或 module/import。但当前目标是先把已有工具稳定演进。
