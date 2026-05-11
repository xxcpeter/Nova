# Nova Project — Step 12
## 题目：给 Nova 增加 payload-less `enum`，重构 TokenKind 表示

---

## 1. 本次作业目标

在 Step 10 和 Step 11 中，你已经用 Nova 写出了：

```text
source -> tokenize(source) -> vec<Token>
vec<Token> -> parser prototype -> parse trace
```

目前的问题是：`Token.kind` 仍然是 `str`。

例如：

```nova
struct Token {
    kind: str;
    lexeme: str;
    line: int;
    column: int;
}
```

parser 中判断 token 时需要：

```nova
str_eq(token.kind, "KW_FN")
```

这能跑，但不够像真正的编译器。Step 12 的目标是：

> **给 Nova 增加最小可用的 enum 类型，并用它表示 TokenKind。**

完成后，你应该能写：

```nova
enum TokenKind {
    KW_FN;
    KW_LET;
    IDENT;
    INT_LITERAL;
    STR_LITERAL;
    EOF;
}

struct Token {
    kind: TokenKind;
    lexeme: str;
    line: int;
    column: int;
}
```

并且能写：

```nova
if (token.kind == TokenKind.KW_FN) {
    ...
}
```

---

## 2. 本阶段核心原则

Step 12 只做 **payload-less enum**，也就是 C-style enum。

支持：

- enum declaration
- enum type
- enum member access
- enum equality / inequality
- enum 作为变量、字段、参数、返回值
- enum lowering 到 C enum

不做：

- enum payload
- tagged union
- pattern matching
- exhaustive checking
- enum methods
- enum-to-int cast
- int-to-enum cast
- enum value customization

也就是说，本阶段只支持：

```nova
enum Color {
    Red;
    Green;
    Blue;
}
```

不支持：

```nova
enum Expr {
    IntLiteral(value: int);
    Binary(left: Expr, right: Expr);
}
```

---

## 3. 为什么不直接做 tagged union

完整 tagged union 对编译器很有用，但现在太早。

它会立刻引入：

- payload layout
- recursive type
- variant constructors
- match / switch
- ownership / allocation
- AST representation 设计

这会把 Step 12 变成一个非常大的阶段。

所以本阶段先做：

> **enum without payload**

它足够解决当前最急的问题：`Token.kind` 不应该是字符串。

---

## 4. 新增语法

### 4.1 Enum declaration

```nova
enum TokenKind {
    KW_FN;
    KW_LET;
    IDENT;
    EOF;
}
```

规则：

- `enum` 是新关键字。
- enum 只能出现在顶层。
- enum 名称是 identifier。
- enum member 是 identifier。
- 每个 member 以 `;` 结束。
- enum declaration 本身不需要结尾 `;`。

---

### 4.2 Enum type

enum 名称可以作为类型使用：

```nova
let k : TokenKind = TokenKind.KW_FN;
```

struct field 也可以是 enum：

```nova
struct Token {
    kind: TokenKind;
    lexeme: str;
    line: int;
    column: int;
}
```

function parameter / return type 也可以是 enum：

```nova
fn is_keyword(kind: TokenKind) : bool {
    return kind == TokenKind.KW_FN;
}
```

---

### 4.3 Enum member access

语法：

```nova
EnumName.MemberName
```

示例：

```nova
TokenKind.KW_FN
TokenKind.IDENT
TokenKind.EOF
```

语义：

- `TokenKind` 必须是已定义 enum。
- `KW_FN` 必须是 `TokenKind` 的 member。
- 表达式类型是 `TokenKind`。

---

## 5. Lexer 修改

新增关键字：

```text
enum
```

`DOT` 已经在 Step 8 中加入，所以不需要新增 `.` token。

---

## 6. AST / Type 修改

### 6.1 TypeKind

当前大概已有：

```cpp
enum class TypeKind {
    Int,
    Bool,
    Str,
    Void,
    Struct,
    Vec
};
```

新增：

```cpp
Enum
```

也就是：

```cpp
enum class TypeKind {
    Int,
    Bool,
    Str,
    Void,
    Struct,
    Vec,
    Enum
};
```

`Type.name` 可以同时用于 `Struct` 和 `Enum`：

```cpp
Type{ TypeKind::Struct, "Point" }
Type{ TypeKind::Enum, "TokenKind" }
```

`operator==`：

- enum type 要求 enum name 相同
- `TokenKind` 和其他 enum 不相等
- `TokenKind` 和 struct `TokenKind` 也不应同时存在

---

### 6.2 新增 EnumDecl

```cpp
struct EnumMember {
    std::string name;
    SourceLocation location;
};

struct EnumDecl : Decl {
    std::string name;
    SourceLocation name_location;
    std::vector<EnumMember> members;
};
```

---

### 6.3 Enum member access 表示

本阶段推荐**不要新建复杂 path AST**，而是复用已有 `FieldAccessExpr`，由 sema 标注它到底是：

- struct field access：`token.kind`
- enum member access：`TokenKind.KW_FN`

建议给 `FieldAccessExpr` 增加一个 sema 标注：

```cpp
enum class FieldAccessKind {
    Unknown,
    StructField,
    EnumMember
};

mutable FieldAccessKind access_kind = FieldAccessKind::Unknown;
mutable std::string enum_name;
mutable std::string enum_member_name;
```

sema 判断后：

```cpp
expr.access_kind = FieldAccessKind::EnumMember;
expr.enum_name = "TokenKind";
expr.enum_member_name = "KW_FN";
```

codegen 根据 `access_kind` 生成不同 C 代码。

---

## 7. Parser 修改

### 7.1 Top-level declaration

顶层现在可以是：

- `struct`
- `enum`
- `fn`

`parse_program()`：

```text
KW_STRUCT -> parse_struct_decl()
KW_ENUM   -> parse_enum_decl()
KW_FN     -> parse_function_decl()
```

---

### 7.2 parse_enum_decl

解析：

```nova
enum TokenKind {
    KW_FN;
    IDENT;
    EOF;
}
```

流程：

1. consume `enum`
2. expect enum name
3. expect `{`
4. while not `}`:
   - expect member name
   - expect `;`
5. expect `}`

---

### 7.3 parse_type

这里有一个设计选择。

你现在的 parser 看到 user-defined type 时，大概率直接生成：

```cpp
Type{ TypeKind::Struct, name }
```

Step 12 后，`IDENT` 类型既可能是 struct，也可能是 enum。

最干净的方案是引入：

```cpp
TypeKind::Named
```

parser 看到 `IDENT` type 时返回：

```cpp
Type{ TypeKind::Named, name }
```

然后 sema canonicalize：

```text
Named("Point")     -> Struct("Point")
Named("TokenKind") -> Enum("TokenKind")
```

如果你想少改，也可以先让 parser 仍然返回 `Struct(name)`，然后 sema 在 `validate_type` 时如果发现名字属于 enum，就把它当成 enum。但这个方案会让类型系统名字不够准确。

### 推荐

Step 12 最好做 `Named -> canonical type`，因为后面类型系统还会继续扩展。

---

## 8. Sema 修改

### 8.1 Enum table

新增：

```cpp
struct EnumInfo {
    std::string name;
    SourceLocation location;
    std::vector<std::string> members;
    std::unordered_map<std::string, size_t> member_index;
};
```

新增成员：

```cpp
std::unordered_map<std::string, EnumInfo> enums_;
```

---

### 8.2 收集 enum declarations

分析顺序建议：

```text
install_builtin_functions()
collect_struct_declarations()
collect_enum_declarations()
collect_function_signatures()
analyze functions
```

更稳的是：

```text
collect type names(structs + enums)
validate struct fields
validate enum members
collect functions
analyze functions
```

需要检查：

- enum 重名
- enum 不能和 struct 重名
- enum 不能和 builtin type 重名
- member 不能重复
- enum 至少有一个 member（建议要求）

---

### 8.3 Type validation

新增规则：

- enum type 必须存在
- enum 可以作为变量、字段、参数、返回值、vec element
- `vec<TokenKind>` 合法
- `void` 仍然不能作为变量/字段/参数/vec element

如果你引入了 `Named`，则 sema 应该把所有声明类型都 canonicalize 成 `Struct` 或 `Enum`。例如：

```nova
let k : TokenKind = TokenKind.KW_FN;
```

声明类型从 parser 的：

```text
Named("TokenKind")
```

变成 sema 后的：

```text
Enum("TokenKind")
```

---

### 8.4 Enum member access checking

对于：

```nova
TokenKind.KW_FN
```

如果 object 是 `IdentifierExpr("TokenKind")`，并且：

- 当前变量作用域没有名为 `TokenKind` 的变量
- `TokenKind` 是 enum name
- field 是 enum member

则这是 enum member access。

规则：

- enum name 必须存在
- member 必须存在
- expression type 是 `TokenKind`

为了简单，建议禁止变量名和 type name 冲突。这样 `TokenKind.KW_FN` 不会和变量 field access 混淆。

---

### 8.5 Equality / inequality

允许：

```nova
a == b
a != b
```

如果 `a` 和 `b` 是同一个 enum type。

你的 equality 规则如果已经是“左右类型相同即可”，enum 会自然工作。

不允许：

```nova
TokenKind.KW_FN == OtherKind.X
TokenKind.KW_FN == 1
```

---

## 9. Codegen 修改

### 9.1 Enum declaration -> C enum

Nova：

```nova
enum TokenKind {
    KW_FN;
    IDENT;
    EOF;
}
```

生成 C：

```c
typedef enum TokenKind {
    TokenKind_KW_FN,
    TokenKind_IDENT,
    TokenKind_EOF
} TokenKind;
```

注意 enum member 名字加 enum 前缀，避免不同 enum 之间 member 重名。

---

### 9.2 Enum member access

Nova：

```nova
TokenKind.KW_FN
```

生成：

```c
TokenKind_KW_FN
```

如果 `FieldAccessExpr` 被 sema 标记为 enum member access，codegen 生成 enum constant。

否则普通 field access 继续生成：

```c
object.field
```

---

### 9.3 c_type

```cpp
case TypeKind::Enum:
    return type.name;
```

---

### 9.4 输出顺序

C 代码生成顺序建议：

```text
#include "nova_runtime.h"

enum declarations
struct declarations
vector helpers
functions
```

如果 struct field 使用 enum type，enum 必须先生成。

---

## 10. Runtime 修改

Step 12 不需要 runtime 修改。

enum 是编译期类型，直接 lowering 到 C enum。

---

## 11. Nova tools 迁移策略：避免反复整份重写

你担心后面反复重写 `nova_tokenizer.nv` / `nova_parser.nv`，这个担心很对。Step 12 开始要改策略。

### 11.1 稳定外部 API

尽量长期保持：

```nova
fn tokenize(source: str) : vec<Token>
fn dump_tokens(tokens: vec<Token>) : str
fn parse_program(tokens: vec<Token>) : str
```

即使内部 `Token.kind` 改了，外部接口也不变。

---

### 11.2 使用 compatibility layer

内部改成：

```nova
struct Token {
    kind: TokenKind;
    lexeme: str;
    line: int;
    column: int;
}
```

但增加：

```nova
fn token_kind_name(kind: TokenKind) : str
```

dump 仍然：

```nova
buf_push_str(out, token_kind_name(t.kind));
```

这样 `.tok` golden output 可以保持不变。

---

### 11.3 Parser check 小步迁移

旧：

```nova
fn check(tokens: vec<Token>, pos: int, kind: str) : bool {
    return str_eq(vec_get(tokens, pos).kind, kind);
}
```

新：

```nova
fn check(tokens: vec<Token>, pos: int, kind: TokenKind) : bool {
    return vec_get(tokens, pos).kind == kind;
}
```

调用点从：

```nova
check(tokens, pos, "KW_FN")
```

变成：

```nova
check(tokens, pos, TokenKind.KW_FN)
```

这是局部替换，不是重写 parser。

---

### 11.4 以后每一步都保留 wrapper

Step 13 如果把 parse trace 升级成 parse tree，也保留：

```nova
fn parse_program_trace(tokens: vec<Token>) : str
```

或者保留原 `parse_program` 的 trace 版本，再新增：

```nova
fn parse_program_tree(tokens: vec<Token>) : ParseTree
```

这样旧测试不用全部推倒。

---

## 12. 推荐测试

### 12.1 Codegen positive tests

### `tests/codegen/positive/enum_basic.nv`

```nova
enum Color {
    Red;
    Green;
    Blue;
}

fn main() : void {
    let c : Color = Color.Green;

    if (c == Color.Green) {
        print_str("green");
    } else {
        print_str("other");
    }

    return;
}
```

### `tests/codegen/positive/enum_basic.out`

```text
green
```

---

### `tests/codegen/positive/enum_struct_field.nv`

```nova
enum TokenKind {
    KW_FN;
    IDENT;
    EOF;
}

struct Token {
    kind: TokenKind;
    lexeme: str;
}

fn main() : void {
    let t : Token = Token {
        kind: TokenKind.KW_FN,
        lexeme: "fn"
    };

    if (t.kind == TokenKind.KW_FN) {
        print_str(t.lexeme);
    } else {
        print_str("bad");
    }

    return;
}
```

### `tests/codegen/positive/enum_struct_field.out`

```text
fn
```

---

### `tests/codegen/positive/enum_vec.nv`

```nova
enum TokenKind {
    KW_FN;
    IDENT;
    EOF;
}

fn main() : void {
    let kinds : vec<TokenKind> = vec_new();

    vec_push(kinds, TokenKind.KW_FN);
    vec_push(kinds, TokenKind.IDENT);
    vec_push(kinds, TokenKind.EOF);

    print_int(vec_len(kinds));

    if (vec_get(kinds, 1) == TokenKind.IDENT) {
        print_str("ident");
    } else {
        print_str("bad");
    }

    return;
}
```

### `tests/codegen/positive/enum_vec.out`

```text
3
ident
```

---

### 12.2 Sema negative tests

### `tests/sema/negative/duplicate_enum.nv`

```nova
enum Color {
    Red;
}

enum Color {
    Blue;
}

fn main() : void {
    return;
}
```

### `tests/sema/negative/duplicate_enum.err`

```text
SemanticError at 5:1: duplicate enum 'Color'
```

---

### `tests/sema/negative/duplicate_enum_member.nv`

```nova
enum Color {
    Red;
    Red;
}

fn main() : void {
    return;
}
```

### `tests/sema/negative/duplicate_enum_member.err`

```text
SemanticError at 3:5: duplicate enum member 'Red' in enum 'Color'
```

---

### `tests/sema/negative/unknown_enum_member.nv`

```nova
enum Color {
    Red;
}

fn main() : void {
    let c : Color = Color.Blue;
    return;
}
```

### `tests/sema/negative/unknown_enum_member.err`

```text
SemanticError at 6:27: unknown enum member 'Blue' in enum 'Color'
```

---

### `tests/sema/negative/enum_type_mismatch.nv`

```nova
enum Color {
    Red;
}

enum Shape {
    Circle;
}

fn main() : void {
    let c : Color = Shape.Circle;
    return;
}
```

### `tests/sema/negative/enum_type_mismatch.err`

```text
SemanticError at 10:21: type error in variable initializer: expected Color, got Shape
```

---

### `tests/sema/negative/enum_member_on_unknown_enum.nv`

```nova
fn main() : void {
    let x : int = Missing.Value;
    return;
}
```

### `tests/sema/negative/enum_member_on_unknown_enum.err`

```text
SemanticError at 2:19: undefined variable or enum 'Missing'
```

---

## 13. CMake additions

```cmake
add_codegen_positive_test(enum_basic)
add_codegen_positive_test(enum_struct_field)
add_codegen_positive_test(enum_vec)

add_sema_negative_test(duplicate_enum)
add_sema_negative_test(duplicate_enum_member)
add_sema_negative_test(unknown_enum_member)
add_sema_negative_test(enum_type_mismatch)
add_sema_negative_test(enum_member_on_unknown_enum)
```

---

## 14. 推荐实现顺序

### Step A：Lexer

新增：

```text
enum
```

### Step B：AST

新增：

- `TypeKind::Enum`
- 可选 `TypeKind::Named`
- `EnumDecl`
- `Program.enums`

### Step C：Parser

支持 top-level enum declaration。

### Step D：Sema enum table

实现：

- enum collection
- duplicate enum check
- duplicate member check
- enum type validation

### Step E：Enum member access

先让：

```nova
Color.Red
```

通过 sema。

### Step F：Codegen enum declaration

生成 C enum。

### Step G：Codegen enum member access

生成：

```c
Color_Red
```

### Step H：小步迁移 tools

1. `Token.kind: str` -> `Token.kind: TokenKind`
2. 新增 `token_kind_name(kind: TokenKind) : str`
3. `dump_tokens` 使用 `token_kind_name`
4. parser `check` 改成 enum 参数
5. tool tests 保持输出不变

---

## 15. 本阶段最容易犯的错误

### 错误 1：把 enum member access 当普通 struct field access

`Color.Red` 不是访问变量字段，而是访问 enum constant。

### 错误 2：parser 里过早区分 enum 和 field

Parser 不知道符号表，最好让 sema 判断。

### 错误 3：enum member C 名字冲突

不同 enum 可能都有 `Unknown`，C 里要生成：

```c
Color_Unknown
TokenKind_Unknown
```

### 错误 4：工具输出格式改动太大

内部改成 enum，不代表 golden dump 也要重写。

### 错误 5：enum type 和 struct type 名称冲突

建议禁止：

```nova
struct TokenKind { ... }
enum TokenKind { ... }
```

---

## 16. 验收标准

### 合格

- 能声明 enum
- 能声明 enum 变量
- 能使用 enum member
- 能比较 enum value
- codegen positive tests 通过

### 良好

- enum 可以作为 struct field
- enum 可以作为 vec element
- enum sema negative tests 覆盖常见错误
- generated C enum 名称稳定

### 优秀

- tokenizer/parser 小步迁移到 `TokenKind`
- 旧 tool tests 输出不变
- enum member diagnostics 位置准确

---

## 17. 关于后续会不会反复重写 Nova tools

从 Step 12 开始，后续采用 **稳定接口 + 小步迁移** 策略。

推荐保持这些接口长期稳定：

```nova
fn tokenize(source: str) : vec<Token>
fn dump_tokens(tokens: vec<Token>) : str
fn parse_program(tokens: vec<Token>) : str
```

后续内部变化不应该影响外部测试。

规划如下：

```text
Step 12: 内部 TokenKind enum 化，外部 dump 不变
Step 13: parse trace -> parse tree，但保留 parse trace wrapper
Step 14: 增加 basic semantic checks / simplified lowering，但 tokenizer/parser API 不变
Step 15: bootstrap milestone，对照 C++ compiler 处理 Nova 子集
```

这意味着后面不应该反复整份重写 `nova_tokenizer.nv` / `nova_parser.nv`，而是围绕稳定 API 小步演进。只有当语言能力发生重大变化时，才安排明确的 migration step。 
