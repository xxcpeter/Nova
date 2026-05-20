# Nova Project — Step 15
## 题目：用 Nova 实现 Expression Parser，把表达式从“跳过”升级为 ParseNode tree

---

## 1. 本次作业目标

在 Step 14 中，Nova 已经可以基于 `ParseNode` 做 declaration-level checker：

```text
source
  -> tokenize
  -> parse_program_tree
  -> check_program
```

但是到目前为止，parser 对表达式的处理仍然是：

```text
skip_expr_until_semic
skip_paren_expr
```

也就是说：

```nova
let x : int = 1 + 2 * 3;
return x;
if (x > 0) { ... }
```

这些表达式没有被结构化保存。

Step 15 的目标是：

> **给 Nova parser 增加 expression parser，把表达式解析成 `ParseNode` tree。**

完成后，parser 不再只是：

```text
LetStmt name=x type=int
ReturnStmt value
IfStmt
```

而是能在这些节点下面保存表达式子树。

例如：

```nova
let x : int = 1 + 2 * 3;
```

可以变成：

```text
LetStmt name=x type=int
  BinaryExpr op=+
    IntLiteral value=1
    BinaryExpr op=*
      IntLiteral value=2
      IntLiteral value=3
```

本阶段仍然不做表达式类型检查，那是 Step 16 的内容。

---

## 2. 本阶段核心意义

Step 15 是 Nova self-hosted frontend 的关键转折点。

之前：

```text
ParseNode tree only contains declarations and statements
```

Step 15 后：

```text
ParseNode tree also contains expressions
```

这会让 Step 16 能做真正的 expression type checker。

---

## 3. 本阶段不做什么

本阶段不做：

- expression type checking
- symbol resolution
- function signature checking
- field type checking
- struct literal field type checking
- vec intrinsic checking
- codegen
- full AST / tagged union

本阶段只做：

```text
tokens -> expression ParseNode
```

也就是 syntax-level expression parsing。

---

## 4. 文件策略

继续修改：

```text
tools/nova_checker.nv
```

或者如果你想保持 parser/checker 分离，也可以先在：

```text
tools/nova_parser.nv
```

中实现 expression parser，再复制到 checker。

但我建议 Step 15 直接在 `nova_checker.nv` 上推进，因为 Step 16 会接着用它做 type checking。

等 Step 18 有 import/module 后，再拆成：

```text
lib/tokenizer.nv
lib/parser.nv
lib/checker.nv
```

---

## 5. ParseNodeKind 扩展

当前大概已有：

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

Step 15 建议新增 expression node kinds：

```nova
enum ParseNodeKind {
    ...
    AssignExpr;
    BinaryExpr;
    UnaryExpr;
    CallExpr;
    FieldAccessExpr;
    StructLiteralExpr;
    StructFieldInit;
    EnumMemberExpr;
    IdentifierExpr;
    IntLiteralExpr;
    StrLiteralExpr;
    BoolLiteralExpr;
}
```

仍然用通用：

```nova
struct ParseNode {
    kind: ParseNodeKind;
    name: str;
    value: str;
    children: vec<ParseNode>;
    line: int;
    column: int;
}
```

表达式节点字段建议：

```text
IdentifierExpr:
  name = identifier

IntLiteralExpr:
  value = literal text

StrLiteralExpr:
  value = literal text

BoolLiteralExpr:
  value = "true" / "false"

BinaryExpr:
  name = operator lexeme
  children[0] = left
  children[1] = right

UnaryExpr:
  name = operator lexeme
  children[0] = operand

AssignExpr:
  children[0] = target
  children[1] = value

CallExpr:
  name = callee
  children = args

FieldAccessExpr:
  name = field name
  children[0] = object

EnumMemberExpr:
  name = enum name
  value = member name

StructLiteralExpr:
  name = struct name
  children = StructFieldInit nodes

StructFieldInit:
  name = field name
  children[0] = initializer expr
```

---

## 6. 新增 ExprParseResult

和现有 `ParseResult` 类似，新增：

```nova
struct ExprParseResult {
    node: ParseNode;
    next_pos: int;
}
```

所有 expression parser helper 都返回它。

---

## 7. 表达式优先级

建议实现和 C++ parser 一致的递归下降层级：

```text
assignment
logical_or
logical_and
equality
comparison
term
factor
unary
postfix
primary
```

对应函数：

```nova
fn parse_expression(tokens: vec<Token>, pos: int) : ExprParseResult
fn parse_assignment(tokens: vec<Token>, pos: int) : ExprParseResult
fn parse_logical_or(tokens: vec<Token>, pos: int) : ExprParseResult
fn parse_logical_and(tokens: vec<Token>, pos: int) : ExprParseResult
fn parse_equality(tokens: vec<Token>, pos: int) : ExprParseResult
fn parse_comparison(tokens: vec<Token>, pos: int) : ExprParseResult
fn parse_term(tokens: vec<Token>, pos: int) : ExprParseResult
fn parse_factor(tokens: vec<Token>, pos: int) : ExprParseResult
fn parse_unary(tokens: vec<Token>, pos: int) : ExprParseResult
fn parse_postfix(tokens: vec<Token>, pos: int) : ExprParseResult
fn parse_primary(tokens: vec<Token>, pos: int) : ExprParseResult
```

---

## 8. Operators

### Assignment

```nova
x = y
p.x = 1
```

Parse as right-associative:

```text
AssignExpr
  target
  value
```

Rule:

```text
assignment -> logical_or ("=" assignment)?
```

Do not type-check lvalue yet. Step 16 will check assignment target.

---

### Binary operators

Supported:

```text
||
&&
== !=
< <= > >=
+ -
* / %
```

All binary operators are left-associative.

```nova
1 + 2 * 3
```

should parse as:

```text
+
  1
  *
    2
    3
```

---

### Unary operators

Supported:

```text
!
-
```

Parse:

```nova
!flag
-x
```

as:

```text
UnaryExpr op=!
UnaryExpr op=-
```

---

### Postfix

Supported:

```nova
foo(a, b)
obj.field
vec_get(tokens, 0).kind
TokenKind.KW_FN
```

Postfix should support chaining:

```text
primary ( call | field_access )*
```

For now, parser can initially parse all `a.b` as `FieldAccessExpr`.

If object is a plain identifier starting with uppercase and field is identifier, you may optionally create `EnumMemberExpr`, but it is better to keep parser semantic-free and let Step 16 distinguish enum member vs struct field.

Recommended:

```text
TokenKind.KW_FN
```

still parses as:

```text
FieldAccessExpr name=KW_FN
  IdentifierExpr name=TokenKind
```

Step 16 can interpret it as enum member.

---

## 9. Primary expressions

Support:

```nova
123
"hello"
true
false
identifier
(expr)
StructName { field: expr, ... }
```

### Int literal

```text
IntLiteralExpr value=123
```

### String literal

```text
StrLiteralExpr value="hello"
```

Keep lexeme format as tokenizer currently produces it.

### Bool literal

```text
BoolLiteralExpr value=true
BoolLiteralExpr value=false
```

### Identifier

```text
IdentifierExpr name=x
```

### Grouping

For:

```nova
(1 + 2)
```

you can either:

1. return the inner expression directly, or
2. create `GroupingExpr`.

I recommend returning the inner expression directly for now.

### Struct literal

```nova
Point { x: 1, y: 2 }
```

Parse as:

```text
StructLiteralExpr name=Point
  StructFieldInit name=x
    IntLiteralExpr value=1
  StructFieldInit name=y
    IntLiteralExpr value=2
```

Parser can recognize struct literal by:

```text
IDENT LBRACE
```

This can theoretically conflict with block syntax, but in expression position it is fine.

---

## 10. Function call parsing

For:

```nova
foo(a, b + c)
```

Parse as:

```text
CallExpr name=foo
  IdentifierExpr name=a
  BinaryExpr op=+
    IdentifierExpr name=b
    IdentifierExpr name=c
```

Implementation idea:

```nova
fn finish_call(tokens, callee_node, pos) : ExprParseResult
```

But since current ParseNode stores callee name as `name`, initial implementation can support only identifier callees:

```nova
CallExpr name=foo
```

Later if you need first-class callable expressions, extend it.

---

## 11. Statement parser integration

Replace old skipping logic.

### Let statement

Old:

```nova
pos = skip_expr_until_semic(tokens, pos);
```

New:

```nova
pos = expect(tokens, pos, TokenKind.ASSIGN, "expected '=' after variable type");
let expr_result : ExprParseResult = parse_expression(tokens, pos);
let_stmt = add_child(let_stmt, expr_result.node);
pos = expr_result.next_pos;
pos = expect(tokens, pos, TokenKind.SEMIC, "expected ';' after variable declaration");
```

This means `LetStmt` has initializer expression as child.

---

### Return statement

Old:

```nova
return_stmt.value = "value";
pos = skip_expr_until_semic(tokens, pos);
```

New:

```nova
if (!check(tokens, pos, TokenKind.SEMIC)) {
    let expr_result : ExprParseResult = parse_expression(tokens, pos);
    return_stmt.value = "value";
    return_stmt = add_child(return_stmt, expr_result.node);
    pos = expr_result.next_pos;
}
pos = expect(tokens, pos, TokenKind.SEMIC, "expected ';' after return statement");
```

---

### Expr statement

Old:

```nova
pos = skip_expr_until_semic(tokens, pos);
return ExprStmt
```

New:

```nova
let expr_result : ExprParseResult = parse_expression(tokens, pos);
let stmt : ParseNode = make_node(ParseNodeKind.ExprStmt, "", "", line, column);
stmt = add_child(stmt, expr_result.node);
pos = expr_result.next_pos;
pos = expect(tokens, pos, TokenKind.SEMIC, "expected ';' after expression statement");
```

---

### If / while condition

Old:

```nova
pos = skip_paren_expr(tokens, pos);
```

New:

```nova
pos = expect(tokens, pos, TokenKind.LPAREN, "expected '(' after if");
let cond : ExprParseResult = parse_expression(tokens, pos);
let condition_node : ParseNode = make_node(ParseNodeKind.Condition, "", "", line, column);
condition_node = add_child(condition_node, cond.node);
if_stmt = add_child(if_stmt, condition_node);
pos = cond.next_pos;
pos = expect(tokens, pos, TokenKind.RPAREN, "expected ')' after if condition");
```

This requires adding:

```nova
Condition;
```

to `ParseNodeKind`.

If you want to keep old trace compatible, `dump_parse_tree` can choose not to print `Condition`, or print it only in new Step 15 tests. I recommend printing it in Step 15-specific tests.

---

## 12. Dumping expression tree

Update `parse_node_header`.

Suggested mappings:

```text
AssignExpr                  -> "AssignExpr"
BinaryExpr name=+           -> "BinaryExpr op=+"
UnaryExpr name=!            -> "UnaryExpr op=!"
CallExpr name=foo           -> "CallExpr name=foo"
FieldAccessExpr name=kind   -> "FieldAccessExpr name=kind"
StructLiteralExpr name=Point -> "StructLiteralExpr name=Point"
StructFieldInit name=x      -> "FieldInit name=x"
IdentifierExpr name=x       -> "IdentifierExpr name=x"
IntLiteralExpr value=1      -> "IntLiteralExpr value=1"
StrLiteralExpr value="hi"   -> "StrLiteralExpr value="hi""
BoolLiteralExpr value=true  -> "BoolLiteralExpr value=true"
Condition                   -> "Condition"
```

---

## 13. Backward compatibility

Step 15 will naturally change parser trace output because expressions now appear in tree.

So do one of these:

### Option A: update parser `.out` tests

Accept new richer output.

### Option B: keep old parser tests and add `dump_parse_tree_shallow`

Not recommended. It adds complexity.

I recommend Option A:

> Step 15 intentionally changes parser output.

Update only parser/checker tests that compare trace. Tokenizer tests remain unchanged.

Checker positive output can remain unchanged because `check_program()` can ignore expression children for now.

---

## 14. Checker changes in Step 15

The declaration checker should ignore expression nodes for now.

For block checking:

```nova
LetStmt:
    check let declared type
    ignore initializer child

ReturnStmt:
    ignore value expression

ExprStmt:
    ignore expression

IfStmt:
    check Then/Else body
    ignore Condition child

WhileStmt:
    check body
    ignore Condition child
```

If you add `Condition` as child before `ThenBranch`, update `check_block` accordingly.

Recommended `IfStmt` child layout:

```text
IfStmt
  Condition
    ...
  Then
    Block
  Else
    Block
```

Recommended `WhileStmt` child layout:

```text
WhileStmt
  Condition
    ...
  Block
```

This is cleaner for Step 16.

---

## 15. Tests

### 15.1 `tests/tools/parser/expression_precedence.nv`

```nova
fn main() : void {
    let x : int = 1 + 2 * 3;
    return;
}
```

Expected trace shape:

```text
Program
  Function name=main return=void
    Block
      LetStmt name=x type=int
        BinaryExpr op=+
          IntLiteralExpr value=1
          BinaryExpr op=*
            IntLiteralExpr value=2
            IntLiteralExpr value=3
      ReturnStmt
```

---

### 15.2 `tests/tools/parser/expression_call_field.nv`

```nova
fn main() : void {
    print_str(vec_get(tokens, 0).lexeme);
    return;
}
```

Expected shape:

```text
ExprStmt
  CallExpr name=print_str
    FieldAccessExpr name=lexeme
      CallExpr name=vec_get
        IdentifierExpr name=tokens
        IntLiteralExpr value=0
```

---

### 15.3 `tests/tools/parser/expression_struct_literal.nv`

```nova
struct Point {
    x: int;
    y: int;
}

fn main() : void {
    let p : Point = Point { x: 1, y: 2 };
    return;
}
```

Expected shape:

```text
LetStmt name=p type=Point
  StructLiteralExpr name=Point
    FieldInit name=x
      IntLiteralExpr value=1
    FieldInit name=y
      IntLiteralExpr value=2
```

---

### 15.4 `tests/tools/parser/expression_assignment.nv`

```nova
fn main() : void {
    x = x + 1;
    return;
}
```

Expected shape:

```text
ExprStmt
  AssignExpr
    IdentifierExpr name=x
    BinaryExpr op=+
      IdentifierExpr name=x
      IntLiteralExpr value=1
```

---

### 15.5 `tests/tools/parser/condition_expr.nv`

```nova
fn main() : void {
    if (x > 0) {
        return;
    }
    while (x < 10) {
        x = x + 1;
    }
    return;
}
```

Expected shape includes:

```text
IfStmt
  Condition
    BinaryExpr op=>
      IdentifierExpr name=x
      IntLiteralExpr value=0
  Then
    Block
      ReturnStmt
WhileStmt
  Condition
    BinaryExpr op=<
      IdentifierExpr name=x
      IntLiteralExpr value=10
```

---

## 16. Negative parser tests

Add a few syntax-level expression errors:

### missing expression after assignment

```nova
fn main() : void {
    let x : int = ;
    return;
}
```

Expected:

```text
ParseError at 2:19: expected expression
```

### missing right paren in call

```nova
fn main() : void {
    print_int(1;
    return;
}
```

Expected:

```text
ParseError at 2:16: expected ')' after arguments
```

### malformed struct literal

```nova
fn main() : void {
    let p : Point = Point { x 1 };
    return;
}
```

Expected:

```text
ParseError at 2:29: expected ':' after struct literal field name
```

---

## 17. Implementation order

### Step A: Extend ParseNodeKind

Add expression node kinds and update `parse_node_header`.

### Step B: Add ExprParseResult

Implement struct and helper constructors.

### Step C: Implement primary/unary/postfix

Start with:

- literal
- identifier
- grouping
- function call
- field access

### Step D: Implement binary precedence

Add factor/term/comparison/equality/logical.

### Step E: Implement assignment

Right-associative assignment.

### Step F: Implement struct literal

Recognize `IDENT LBRACE` in expression position.

### Step G: Integrate into statements

Replace skip functions in:

- let
- return
- expr stmt
- if/while condition

### Step H: Update checker to ignore expression children

Make sure Step 14 checker still passes.

### Step I: Update parser tests

Parser trace becomes richer.

---

## 18. Common mistakes

### Mistake 1: parsing assignment as left-associative

`a = b = c` should parse as:

```text
a = (b = c)
```

### Mistake 2: treating field access as unary

`.` is postfix, not unary.

### Mistake 3: parsing call only before field access

Need chaining:

```nova
vec_get(tokens, 0).lexeme
```

### Mistake 4: letting expression parser consume semicolon

`parse_expression` should stop before `;`. Statement parser consumes `;`.

### Mistake 5: conditions still using skip_paren_expr

Step 15 should parse condition expression.

---

## 19. Acceptance criteria

### Pass

- expression tree appears in parser trace
- precedence works
- function call works
- field access works
- let/return/expr stmt integrate expressions

### Good

- struct literals parse
- assignment parses
- if/while conditions parse
- checker tests still pass

### Excellent

- expression parse errors have line/column
- parser trace is stable and readable
- Step 16 can use expression nodes directly for type checking

---

## 20. Completion marker

Step 15 is complete when:

```text
Nova parser no longer skips expressions.
```

Instead, expressions become part of the `ParseNode` tree and can be consumed by Step 16's expression type checker.
