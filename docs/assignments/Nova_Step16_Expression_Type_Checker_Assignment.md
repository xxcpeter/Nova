# Nova Project — Step 16
## 题目：用 Nova 实现表达式级类型检查器

---

## 1. 当前进度对齐

当前 Nova tools 的核心结构应该是：

```nova
enum TokenKind {
    ...
}

struct Token {
    kind: TokenKind;
    lexeme: str;
    line: int;
    column: int;
}

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

    // Step 15 后新增的表达式节点
    Condition;
    AssignExpr;
    BinaryExpr;
    UnaryExpr;
    CallExpr;
    FieldAccessExpr;
    StructLiteralExpr;
    StructFieldInitExpr;
    IdentifierExpr;
    IntLiteralExpr;
    StrLiteralExpr;
    BoolLiteralExpr;
}

struct ParseNode {
    kind: ParseNodeKind;
    name: str;
    value: str;
    children: vec<ParseNode>;
    line: int;
    column: int;
}
```

注意：

> `ParseNode.kind` 是 `ParseNodeKind`，不是字符串。  
> Step 16 的类型检查分发必须基于 `node.kind == ParseNodeKind.Xxx`。  
> `node.name` 和 `node.value` 才是字符串载荷。

本阶段说“类型用字符串表示”，指的是：

```text
int
bool
str
void
Point
TokenKind
vec<int>
vec<Token>
```

这些类型名暂时继续用 `str` 保存，而不是说 `ParseNode.kind` 用字符串。

---

## 2. 本次作业目标

在 Step 15 中，表达式已经进入 ParseNode tree，不再只是被跳过。

Step 16 的目标是：

> **基于 expression ParseNode，实现 Nova 写的表达式级类型检查器。**

也就是说，`nova_checker.nv` 从 Step 14 的声明级检查：

```text
top-level symbols
struct / enum / function signatures
main signature
let declared type
```

升级为：

```text
let initializer type check
return expression type check
if / while condition check
binary / unary expression type check
assignment type check
function call check
field access check
enum member access check
struct literal check
vec intrinsic check
```

完成后，Nova checker 能真正理解函数体里的表达式。

---

## 3. 本阶段不做什么

本阶段不做：

- codegen
- module/import
- nested vec
- 泛型函数
- 完整错误恢复
- 替换 C++ sema
- tagged union
- ownership / lifetime
- 类型对象系统

本阶段只做：

```text
ParseNode expression tree -> expression type checking
```

---

## 4. 文件策略

继续修改：

```text
tools/nova_checker.nv
```

原因：

- Step 14 的 declaration checker 已经在这里。
- Step 15 的 expression parser 已经给它提供 expression nodes。
- Step 16 正好继续补表达式类型检查。

等 Step 18 做 import/module 后，再拆成：

```text
lib/tokenizer.nv
lib/parser.nv
lib/checker.nv
lib/types.nv
```

---

## 5. 类型表示：暂时继续用 str

Step 16 不需要新增复杂 `Type` struct。继续用字符串表示类型：

```text
int
bool
str
void
Point
Color
vec<int>
vec<Point>
```

建议保留 / 新增 helper：

```nova
fn type_int() : str {
    return "int";
}

fn type_bool() : str {
    return "bool";
}

fn type_str() : str {
    return "str";
}

fn type_void() : str {
    return "void";
}

fn type_unknown() : str {
    return "<unknown>";
}

fn same_type(a: str, b: str) : bool {
    return str_eq(a, b);
}
```

对于 `vec<T>`，继续使用已有字符串 helper：

```nova
fn is_vec_type(type_name: str) : bool
fn vec_element_type(type_name: str) : str
```

注意：仍然不支持 nested vec：

```nova
vec<vec<int>>
```

---

## 6. SymbolTables 升级

Step 14 的 `SymbolTables` 只保存名字：

```nova
struct SymbolTables {
    all_names: vec<str>;
    structs: vec<str>;
    enums: vec<str>;
    functions: vec<str>;
}
```

Step 16 需要能查字段、枚举成员、函数参数类型，所以需要升级。

---

### 6.1 StructFieldInfo

```nova
struct StructFieldInfo {
    name: str;
    type_name: str;
}
```

---

### 6.2 StructInfo

```nova
struct StructInfo {
    name: str;
    fields: vec<StructFieldInfo>;
}
```

---

### 6.3 EnumInfo

```nova
struct EnumInfo {
    name: str;
    members: vec<str>;
}
```

---

### 6.4 ParamInfo

```nova
struct ParamInfo {
    name: str;
    type_name: str;
}
```

---

### 6.5 FunctionInfo

```nova
struct FunctionInfo {
    name: str;
    return_type: str;
    params: vec<ParamInfo>;
}
```

---

### 6.6 新的 SymbolTables

推荐改成：

```nova
struct SymbolTables {
    all_names: vec<str>;
    structs: vec<StructInfo>;
    enums: vec<EnumInfo>;
    functions: vec<FunctionInfo>;
}
```

如果你想少改，也可以临时保留旧的 `vec<str>`，再增加：

```nova
struct_infos: vec<StructInfo>;
enum_infos: vec<EnumInfo>;
function_infos: vec<FunctionInfo>;
```

但更推荐直接升级成 info 结构。

---

## 7. 必备查找 helper

因为 Nova 目前没有 map，所以继续用线性查找。

```nova
fn has_struct(symbols: SymbolTables, name: str) : bool
fn find_struct(symbols: SymbolTables, name: str) : StructInfo

fn has_enum(symbols: SymbolTables, name: str) : bool
fn find_enum(symbols: SymbolTables, name: str) : EnumInfo

fn has_function(symbols: SymbolTables, name: str) : bool
fn find_function(symbols: SymbolTables, name: str) : FunctionInfo

fn struct_has_field(info: StructInfo, field_name: str) : bool
fn struct_field_type(info: StructInfo, field_name: str) : str

fn enum_has_member(info: EnumInfo, member_name: str) : bool
```

查找失败时可以报：

```nova
checker_error("internal error: missing struct info");
```

正常路径应该先 `has_*` 再 `find_*`。

---

## 8. collect_top_level_symbols 的新职责

Step 14 只收名字。Step 16 要收完整信息。

### StructDecl

ParseNode 形态：

```text
Struct name=Point
  Field name=x type=int
  Field name=y type=int
```

收成：

```nova
StructInfo {
    name: "Point",
    fields: [
        StructFieldInfo { name: "x", type_name: "int" },
        StructFieldInfo { name: "y", type_name: "int" }
    ]
}
```

---

### EnumDecl

ParseNode 形态：

```text
Enum name=Color
  Member name=Red
  Member name=Green
```

收成：

```nova
EnumInfo {
    name: "Color",
    members: ["Red", "Green"]
}
```

---

### FunctionDecl

ParseNode 形态：

```text
Function name=f return=int
  Param name=x type=int
  Param name=y type=str
  Block
```

收成：

```nova
FunctionInfo {
    name: "f",
    return_type: "int",
    params: [
        ParamInfo { name: "x", type_name: "int" },
        ParamInfo { name: "y", type_name: "str" }
    ]
}
```

---

## 9. 类型名检查要改用新的 SymbolTables

原来可能是：

```nova
vec_str_contains(symbols.structs, type_name)
```

升级后应该是：

```nova
has_struct(symbols, type_name)
has_enum(symbols, type_name)
```

`is_known_type` 应该变成：

```nova
fn is_known_type(type_name: str, symbols: SymbolTables) : bool {
    if (is_builtin_type(type_name)) {
        return true;
    }

    if (has_struct(symbols, type_name)) {
        return true;
    }

    if (has_enum(symbols, type_name)) {
        return true;
    }

    if (is_vec_type(type_name)) {
        let elem : str = vec_element_type(type_name);

        if (str_eq(elem, "void")) {
            return false;
        }

        if (is_vec_type(elem)) {
            return false;
        }

        return is_known_type(elem, symbols);
    }

    return false;
}
```

---

## 10. Variable scope

Step 16 需要变量类型。

### 10.1 VarInfo

```nova
struct VarInfo {
    name: str;
    type_name: str;
}
```

### 10.2 Scope

```nova
struct Scope {
    vars: vec<VarInfo>;
}
```

### 10.3 ScopeStack

```nova
struct ScopeStack {
    scopes: vec<Scope>;
}
```

### 10.4 helper

```nova
fn make_scope_stack() : ScopeStack
fn push_scope(stack: ScopeStack) : ScopeStack
fn pop_scope(stack: ScopeStack) : ScopeStack

fn declare_var(stack: ScopeStack, name: str, type_name: str, line: int, column: int) : ScopeStack
fn has_var_in_current_scope(stack: ScopeStack, name: str) : bool
fn has_var(stack: ScopeStack, name: str) : bool
fn resolve_var_type(stack: ScopeStack, name: str, line: int, column: int) : str
```

因为 Nova struct 是按值传递，修改 stack 的函数建议返回新值：

```nova
stack = push_scope(stack);
stack = declare_var(stack, "x", "int", line, column);
stack = pop_scope(stack);
```

---

## 11. Function body checking

新增：

```nova
fn validate_function_bodies(program: ParseNode, symbols: SymbolTables) : void
```

它遍历 `program.children`，对每个 `FunctionDecl` 调用：

```nova
fn check_function_body(func_node: ParseNode, symbols: SymbolTables) : void
```

流程：

1. 找到对应 `FunctionInfo`
2. 创建 `ScopeStack`
3. push function scope
4. 把参数加入 scope
5. 找到函数 body block
6. 调用：

```nova
check_block_types(body, symbols, stack, function_return_type);
```

---

## 12. check_program 顺序

Step 16 推荐顺序：

```nova
fn check_program(tree: ParseNode) : str {
    let symbols : SymbolTables = collect_top_level_symbols(tree);

    validate_declarations(tree, symbols);
    check_main_function(symbols);
    validate_function_bodies(tree, symbols);

    return summary;
}
```

原因：

- 先收集所有 top-level info，函数互相调用才可见。
- 先 validate declarations，保证 type name 基础正确。
- 再检查函数体。

---

## 13. Statement type checking

新增：

```nova
fn check_block_types(block: ParseNode, symbols: SymbolTables, stack: ScopeStack, return_type: str) : void
```

它遍历 block 的 children，根据 `stmt.kind` 分发。

---

### 13.1 LetStmt

ParseNode：

```text
LetStmt name=x type=int
  <initializer expr>
```

检查：

1. let type exists
2. let type not void
3. 当前 scope 不重复
4. initializer expression type 等于 declared type
5. declare variable

伪代码：

```nova
let declared_type : str = stmt.value;

check_type_exists(declared_type, symbols, "variable declaration", stmt.line, stmt.column);
check_non_void_type(declared_type, symbols, "variable declaration", stmt.line, stmt.column);

if (vec_len(stmt.children) != 1) {
    checker_error_at(stmt.line, stmt.column, "variable declaration requires initializer");
}

let init_expr : ParseNode = vec_get(stmt.children, 0);
let init_type : str = infer_expr_type_expected(init_expr, symbols, stack, declared_type);

if (!same_type(init_type, declared_type)) {
    checker_error_at(
        stmt.line,
        stmt.column,
        str_concat(
            "type error in variable initializer: expected ",
            str_concat(declared_type, str_concat(", got ", init_type))
        )
    );
}

stack = declare_var(stack, stmt.name, declared_type, stmt.line, stmt.column);
```

注意：如果 `check_block_types` 内部需要保留 let 后的变量，函数需要返回新的 `ScopeStack`，或者在 block 内部使用本地 mutable `stack` 变量。

推荐签名：

```nova
fn check_block_types(block: ParseNode, symbols: SymbolTables, stack: ScopeStack, return_type: str) : ScopeStack
```

---

### 13.2 ReturnStmt

ParseNode：

```text
ReturnStmt
```

或：

```text
ReturnStmt
  <expr>
```

规则：

- return type 是 `void`：
  - 无 child 合法
  - 有 child 报错
- return type 非 `void`：
  - 必须有 child
  - child type 必须等于 return type

---

### 13.3 ExprStmt

ParseNode：

```text
ExprStmt
  <expr>
```

检查：

```nova
infer_expr_type(expr, symbols, stack);
```

结果可以忽略。

---

### 13.4 IfStmt

推荐 Step 15 后结构：

```text
IfStmt
  Condition
    <expr>
  Then
    Block
  Else
    Block
```

检查：

1. condition child 存在
2. condition expr type must be bool
3. then block 递归检查
4. else block 如果存在，递归检查

---

### 13.5 WhileStmt

推荐结构：

```text
WhileStmt
  Condition
    <expr>
  Block
```

检查：

1. condition type must be bool
2. body block 递归检查

---

## 14. Expression type inference

核心函数：

```nova
fn infer_expr_type(expr: ParseNode, symbols: SymbolTables, stack: ScopeStack) : str {
    return infer_expr_type_expected(expr, symbols, stack, "");
}

fn infer_expr_type_expected(expr: ParseNode, symbols: SymbolTables, stack: ScopeStack, expected_type: str) : str
```

分发时必须基于 `expr.kind`：

```nova
if (expr.kind == ParseNodeKind.IntLiteralExpr) {
    return "int";
} else if (expr.kind == ParseNodeKind.IdentifierExpr) {
    ...
}
```

不要用字符串比较节点类型。

---

## 15. Literal expressions

### IntLiteralExpr

```nova
return "int";
```

### StrLiteralExpr

```nova
return "str";
```

### BoolLiteralExpr

```nova
return "bool";
```

---

## 16. IdentifierExpr

ParseNode：

```text
IdentifierExpr name=x
```

规则：

- 如果变量存在，返回变量类型
- 否则报错：

```text
CheckerError at line:column: undefined variable 'x'
```

注意：

- struct name / enum name 不是变量。
- `Color.Red` 是 `FieldAccessExpr`，不是单独的 `IdentifierExpr`。

---

## 17. UnaryExpr

ParseNode：

```text
UnaryExpr op=-
  <expr>
```

也就是：

```nova
expr.kind == ParseNodeKind.UnaryExpr
expr.name == "-"
```

规则：

- `-x`: x must be int, result int
- `!x`: x must be bool, result bool

错误：

```text
CheckerError at line:column: unary '-' expects int
CheckerError at line:column: unary '!' expects bool
```

---

## 18. BinaryExpr

ParseNode：

```text
BinaryExpr op=+
  left
  right
```

也就是：

```nova
expr.kind == ParseNodeKind.BinaryExpr
expr.name == "+"
```

### Arithmetic

Operators:

```text
+ - * / %
```

Rules:

```text
int op int -> int
```

### Comparison

Operators:

```text
< <= > >=
```

Rules:

```text
int op int -> bool
```

### Equality

Operators:

```text
== !=
```

Rules:

- int / bool / str / enum: same type -> bool
- struct equality: 暂不支持
- vec equality: 暂不支持

### Logical

Operators:

```text
&& ||
```

Rules:

```text
bool op bool -> bool
```

---

## 19. AssignExpr

ParseNode：

```text
AssignExpr
  target
  value
```

规则：

1. target 必须可赋值
2. target type 和 value type 相同
3. result type 建议返回 target type

可赋值：

- `IdentifierExpr`
- struct field `FieldAccessExpr`

不可赋值：

- literal
- call
- binary expr
- enum member access

建议：

```nova
fn is_assignable_expr(expr: ParseNode, symbols: SymbolTables, stack: ScopeStack) : bool
```

注意 enum member access 也会是 `FieldAccessExpr`，所以需要在 `infer_field_access_type` 里区分它是不是 enum member。

---

## 20. CallExpr

ParseNode：

```text
CallExpr name=foo
  arg1
  arg2
```

规则：

1. 如果是用户函数：
   - 函数必须存在
   - argument count 匹配
   - 每个 argument type 匹配对应 parameter type
   - 返回 function return_type

2. 如果是 builtin / intrinsic：
   - 交给专门 handler

建议：

```nova
fn infer_call_expr_type(expr: ParseNode, symbols: SymbolTables, stack: ScopeStack, expected_type: str) : str
```

---

## 21. 分离 builtin / intrinsic handler

这是之前 TODO 里的：

> separate builtin caller handler

Step 16 正适合做。

建议：

```nova
fn is_runtime_builtin(name: str) : bool
fn infer_runtime_builtin_call(expr: ParseNode, symbols: SymbolTables, stack: ScopeStack, expected_type: str) : str

fn is_vec_intrinsic(name: str) : bool
fn infer_vec_intrinsic_call(expr: ParseNode, symbols: SymbolTables, stack: ScopeStack, expected_type: str) : str
```

---

## 22. Runtime builtins

至少覆盖当前 Nova tools 使用到的：

```text
print_int(int) -> void
print_str(str) -> void

str_eq(str, str) -> bool
str_concat(str, str) -> str
str_len(str) -> int
str_get(str, int) -> int
str_slice(str, int, int) -> str
str_starts_with(str, str) -> bool
str_contains(str, str) -> bool
int_to_str(int) -> str

read_file(str) -> str
write_file(str, str) -> void

buf_new() -> int
buf_push_str(int, str) -> void
buf_push_int(int, int) -> void
buf_to_str(int) -> str

arg_count() -> int
arg_get(int) -> str

nova_runtime_error(str) -> void
```

如果还有当前工具正在用的 runtime function，也要补进去。

---

## 23. Vec intrinsics

支持：

```text
vec_new() -> expected type required
vec_len(vec<T>) -> int
vec_push(vec<T>, T) -> void
vec_get(vec<T>, int) -> T
vec_set(vec<T>, int, T) -> void
```

### vec_new

`vec_new()` 需要 expected type。

合法：

```nova
let xs : vec<int> = vec_new();
```

此时 let 声明提供 expected type：

```text
vec<int>
```

非法：

```nova
vec_new();
```

因为没有上下文：

```text
CheckerError at 2:5: cannot infer type for vec_new
```

### vec_push

```nova
vec_push(xs, value)
```

规则：

- arg0 type must be `vec<T>`
- arg1 type must be `T`
- result `void`

### vec_get

```nova
vec_get(xs, index)
```

规则：

- arg0 type must be `vec<T>`
- arg1 type must be `int`
- result `T`

### vec_set

```nova
vec_set(xs, index, value)
```

规则：

- arg0 type must be `vec<T>`
- arg1 type must be `int`
- arg2 type must be `T`
- result `void`

### vec_len

```nova
vec_len(xs)
```

规则：

- arg0 type must be `vec<T>`
- result `int`

---

## 24. FieldAccessExpr

ParseNode：

```text
FieldAccessExpr name=field
  object
```

两种情况。

---

### 24.1 Enum member access

如果 object 是：

```text
IdentifierExpr name=Color
```

并且 `Color` 是 enum name，那么：

```nova
Color.Red
```

应该被解释为 enum member access。

规则：

- enum 必须存在
- member 必须存在
- result type 是 enum 名字，比如 `"Color"`

注意：要先检查这个情况，再尝试把 `Color` 当变量解析。否则会报 undefined variable。

---

### 24.2 Struct field access

如果 object expression type 是 struct 名字：

```nova
p.x
```

规则：

- struct 必须存在
- field 必须存在
- result type 是 field type

如果 object type 是 enum、int、str、bool、vec，则不能 field access。

---

## 25. StructLiteralExpr

ParseNode：

```text
StructLiteralExpr name=Point
  StructFieldInitExpr name=x
    IntLiteralExpr value=1
  StructFieldInitExpr name=y
    IntLiteralExpr value=2
```

规则：

1. struct type exists
2. no duplicate initializer field
3. every initializer field exists
4. every required field present
5. every field initializer type matches field type

Result type:

```text
Point
```

---

## 26. Expected type propagation

这一步最关键。

需要 expected type 的场景：

```nova
let xs : vec<int> = vec_new();
return vec_new();
vec_push(xs, Token { ... });
Point { x: ..., y: ... }
```

建议：

```nova
fn infer_expr_type(expr: ParseNode, symbols: SymbolTables, stack: ScopeStack) : str {
    return infer_expr_type_expected(expr, symbols, stack, "");
}
```

空字符串表示没有 expected type。

在这些地方传 expected type：

### let initializer

```nova
infer_expr_type_expected(init_expr, symbols, stack, declared_type)
```

### return expression

```nova
infer_expr_type_expected(return_expr, symbols, stack, return_type)
```

### function arguments

```nova
infer_expr_type_expected(arg_expr, symbols, stack, param_type)
```

### struct literal field initializer

```nova
infer_expr_type_expected(init_expr, symbols, stack, field_type)
```

### vec_push / vec_set value

```nova
infer_expr_type_expected(value_expr, symbols, stack, element_type)
```

---

## 27. Checker integration

Step 14 里 `validate_declarations` 继续保留。  
Step 16 新增：

```nova
fn validate_function_bodies(program: ParseNode, symbols: SymbolTables) : void
```

然后：

```nova
fn check_program(tree: ParseNode) : str {
    let symbols : SymbolTables = collect_top_level_symbols(tree);

    validate_declarations(tree, symbols);
    check_main_function(symbols);
    validate_function_bodies(tree, symbols);

    return summary;
}
```

---

## 28. 测试目录

新增：

```text
tests/tools/typecheck/positive/
tests/tools/typecheck/negative/
```

---

# 29. Positive tests

## 29.1 `tests/tools/typecheck/positive/let_return_int.nv`

```nova
fn id(x: int) : int {
    let y : int = x;
    return y;
}

fn main() : void {
    print_int(id(1));
    return;
}
```

## `tests/tools/typecheck/positive/let_return_int.out`

```text
Check OK
Structs: 0
Enums: 0
Functions: 2
```

---

## 29.2 `tests/tools/typecheck/positive/struct_literal_field_access.nv`

```nova
struct Point {
    x: int;
    y: int;
}

fn main() : void {
    let p : Point = Point { x: 1, y: 2 };
    print_int(p.x);
    return;
}
```

## `tests/tools/typecheck/positive/struct_literal_field_access.out`

```text
Check OK
Structs: 1
Enums: 0
Functions: 1
```

---

## 29.3 `tests/tools/typecheck/positive/enum_member_compare.nv`

```nova
enum Color {
    Red;
    Green;
}

fn main() : void {
    let c : Color = Color.Red;
    if (c == Color.Red) {
        print_str("red");
    }
    return;
}
```

## `tests/tools/typecheck/positive/enum_member_compare.out`

```text
Check OK
Structs: 0
Enums: 1
Functions: 1
```

---

## 29.4 `tests/tools/typecheck/positive/vec_basic.nv`

```nova
fn main() : void {
    let xs : vec<int> = vec_new();
    vec_push(xs, 1);
    vec_push(xs, 2);
    print_int(vec_get(xs, 0));
    return;
}
```

## `tests/tools/typecheck/positive/vec_basic.out`

```text
Check OK
Structs: 0
Enums: 0
Functions: 1
```

---

## 29.5 `tests/tools/typecheck/positive/while_assignment.nv`

```nova
fn main() : void {
    let i : int = 0;
    while (i < 3) {
        i = i + 1;
    }
    print_int(i);
    return;
}
```

## `tests/tools/typecheck/positive/while_assignment.out`

```text
Check OK
Structs: 0
Enums: 0
Functions: 1
```

---

# 30. Negative tests

## 30.1 `tests/tools/typecheck/negative/let_type_mismatch.nv`

```nova
fn main() : void {
    let x : int = "hello";
    return;
}
```

## `tests/tools/typecheck/negative/let_type_mismatch.err`

```text
CheckerError at 2:5: type error in variable initializer: expected int, got str
```

---

## 30.2 `tests/tools/typecheck/negative/return_type_mismatch.nv`

```nova
fn f() : int {
    return "hello";
}

fn main() : void {
    return;
}
```

## `tests/tools/typecheck/negative/return_type_mismatch.err`

```text
CheckerError at 2:5: return type mismatch: expected int, got str
```

---

## 30.3 `tests/tools/typecheck/negative/if_condition_not_bool.nv`

```nova
fn main() : void {
    if (1) {
        return;
    }
    return;
}
```

## `tests/tools/typecheck/negative/if_condition_not_bool.err`

```text
CheckerError at 2:9: if condition must be bool
```

---

## 30.4 `tests/tools/typecheck/negative/undefined_variable.nv`

```nova
fn main() : void {
    print_int(x);
    return;
}
```

## `tests/tools/typecheck/negative/undefined_variable.err`

```text
CheckerError at 2:15: undefined variable 'x'
```

---

## 30.5 `tests/tools/typecheck/negative/call_arg_mismatch.nv`

```nova
fn f(x: int) : void {
    return;
}

fn main() : void {
    f("hello");
    return;
}
```

## `tests/tools/typecheck/negative/call_arg_mismatch.err`

```text
CheckerError at 6:7: type error in argument 1 of call to 'f': expected int, got str
```

---

## 30.6 `tests/tools/typecheck/negative/unknown_field.nv`

```nova
struct Point {
    x: int;
}

fn main() : void {
    let p : Point = Point { x: 1 };
    print_int(p.y);
    return;
}
```

## `tests/tools/typecheck/negative/unknown_field.err`

```text
CheckerError at 7:17: unknown field 'y' in struct 'Point'
```

---

## 30.7 `tests/tools/typecheck/negative/unknown_enum_member.nv`

```nova
enum Color {
    Red;
}

fn main() : void {
    let c : Color = Color.Blue;
    return;
}
```

## `tests/tools/typecheck/negative/unknown_enum_member.err`

```text
CheckerError at 6:27: unknown enum member 'Blue' in enum 'Color'
```

---

## 30.8 `tests/tools/typecheck/negative/vec_new_without_expected_type.nv`

```nova
fn main() : void {
    vec_new();
    return;
}
```

## `tests/tools/typecheck/negative/vec_new_without_expected_type.err`

```text
CheckerError at 2:5: cannot infer type for vec_new
```

---

## 30.9 `tests/tools/typecheck/negative/vec_push_type_mismatch.nv`

```nova
fn main() : void {
    let xs : vec<int> = vec_new();
    vec_push(xs, "hello");
    return;
}
```

## `tests/tools/typecheck/negative/vec_push_type_mismatch.err`

```text
CheckerError at 3:18: type error in vec_push value: expected int, got str
```

---

## 30.10 `tests/tools/typecheck/negative/assignment_type_mismatch.nv`

```nova
fn main() : void {
    let x : int = 1;
    x = "hello";
    return;
}
```

## `tests/tools/typecheck/negative/assignment_type_mismatch.err`

```text
CheckerError at 3:5: type error in assignment: expected int, got str
```

---

## 30.11 `tests/tools/typecheck/negative/non_assignable_target.nv`

```nova
fn main() : void {
    1 = 2;
    return;
}
```

## `tests/tools/typecheck/negative/non_assignable_target.err`

```text
CheckerError at 2:5: invalid assignment target
```

---

## 31. CMake helper

## Positive

```cmake
function(add_nova_typecheck_positive_test name)
    add_test(
        NAME nova_tool_typecheck_positive_${name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/nova_checker.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/typecheck/positive/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/typecheck/positive/${name}.out
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/typecheck_positive_${name}
            -DCC=cc
            -P ${NOVA_TOOL_TEST_DRIVER}
    )
    set_tests_properties(nova_tool_typecheck_positive_${name} PROPERTIES LABELS "nova_tool;typecheck;positive")
endfunction()
```

## Negative

```cmake
function(add_nova_typecheck_negative_test name)
    add_test(
        NAME nova_tool_typecheck_negative_${name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/nova_checker.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/typecheck/negative/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/typecheck/negative/${name}.err
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/typecheck_negative_${name}
            -DSTRIP_RUNTIME_PREFIX=ON
            -DCC=cc
            -P ${NOVA_TOOL_NEGATIVE_TEST_DRIVER}
    )
    set_tests_properties(nova_tool_typecheck_negative_${name} PROPERTIES LABELS "nova_tool;typecheck;negative")
endfunction()
```

---

## 32. CMake 添加测试

```cmake
add_nova_typecheck_positive_test(let_return_int)
add_nova_typecheck_positive_test(struct_literal_field_access)
add_nova_typecheck_positive_test(enum_member_compare)
add_nova_typecheck_positive_test(vec_basic)
add_nova_typecheck_positive_test(while_assignment)

add_nova_typecheck_negative_test(let_type_mismatch)
add_nova_typecheck_negative_test(return_type_mismatch)
add_nova_typecheck_negative_test(if_condition_not_bool)
add_nova_typecheck_negative_test(undefined_variable)
add_nova_typecheck_negative_test(call_arg_mismatch)
add_nova_typecheck_negative_test(unknown_field)
add_nova_typecheck_negative_test(unknown_enum_member)
add_nova_typecheck_negative_test(vec_new_without_expected_type)
add_nova_typecheck_negative_test(vec_push_type_mismatch)
add_nova_typecheck_negative_test(assignment_type_mismatch)
add_nova_typecheck_negative_test(non_assignable_target)
```

---

## 33. 推荐实现顺序

1. 升级 `SymbolTables`，保存 `StructInfo / EnumInfo / FunctionInfo`。
2. 改造 `is_known_type / check_type_exists`，使用 info 查找。
3. 实现 `ScopeStack`。
4. 实现 `infer_expr_type_expected` 框架。
5. 先实现 literal / identifier。
6. 实现 unary / binary。
7. 实现 user function call。
8. 实现 runtime builtin handler。
9. 实现 vec intrinsic handler。
10. 实现 field access / enum member access。
11. 实现 struct literal。
12. 接入 let / return / expr stmt / if / while。
13. 添加测试。

---

## 34. 验收标准

### 合格

- 变量解析可用
- let initializer mismatch 能报错
- return type mismatch 能报错
- if / while condition 必须是 bool
- function call 参数能检查

### 良好

- struct field access 可用
- struct literal 检查可用
- enum member access 可用
- vec intrinsic 可用
- runtime builtin 检查可用

### 优秀

- `vec_new()` 的 expected type propagation 可用
- diagnostics 使用 expression 的 line/column
- positive / negative tests 都稳定

---

## 35. 完成标志

Step 16 完成时，Nova checker 应该能做到：

```text
ParseNode expression tree -> type inference -> type errors
```

也就是说，Nova-written frontend 已经具备：

```text
tokenizer
parser
declaration checker
expression type checker
```

这就足够进入 Step 17：frontend integration + diagnostics framework。
