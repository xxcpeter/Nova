# Nova Project — Step 8
## 题目：给 Nova 增加 `struct` / record 能力，为自举数据结构做准备

---

## 1. 本次作业目标

在 Step 7 中，你已经用 Nova 写出了第一个编译器相关工具：Nova lexer / token dump tool。

这说明当前 Nova 已经可以：

- 读取文件
- 扫描字符串
- 构造输出
- 生成可运行的 native program
- 写一些非玩具工具

但是，如果继续往自举方向推进，很快会遇到一个硬问题：

> Nova 目前没有结构化数据类型。

写编译器时，你需要表示：

- Token
- SourceLocation
- Parser 状态
- FunctionSignature
- VariableInfo
- AST 节点的简化模型

只靠 `int / bool / str` 和 runtime handle 会越来越别扭。

所以 Step 8 的目标是：

> **给 Nova 增加最小可用的 `struct` 能力。**

完成后，你应该能写：

```nova
struct Token {
    kind: int;
    lexeme: str;
    line: int;
    column: int;
}

fn main() : void {
    let t : Token = Token { kind: 1, lexeme: "fn", line: 1, column: 1 };
    print_str(t.lexeme);
    print_int(t.line);
    return;
}
```

---

## 2. 本阶段核心原则

Step 8 不是要一次性实现完整类型系统。

只实现 **value-style struct**：

- struct 声明
- struct 类型名
- struct 字面量初始化
- field access
- field assignment
- struct 作为变量、参数、返回值
- struct 之间按值复制

不做：

- methods
- pointers
- references
- inheritance
- generics
- optional fields
- default values
- recursive struct by value
- heap allocation syntax
- destructor / ownership

---

## 3. 新增语法

---

## 3.1 Struct declaration

建议语法：

```nova
struct Name {
    field1: type;
    field2: type;
}
```

示例：

```nova
struct Point {
    x: int;
    y: int;
}
```

规则：

- `struct` 是新的关键字。
- struct 只能出现在顶层。
- struct 名称使用 `Identifier`。
- field 名称使用 `Identifier`。
- field 必须显式声明类型。
- field 声明以 `;` 结束。
- struct 声明本身不需要结尾 `;`。

---

## 3.2 Struct type

struct 名称可以作为类型使用：

```nova
let p : Point = Point { x: 1, y: 2 };
```

函数参数和返回值也可以使用 struct 类型：

```nova
fn make_point(x: int, y: int) : Point {
    return Point { x: x, y: y };
}
```

---

## 3.3 Struct literal

建议语法：

```nova
TypeName { field1: expr, field2: expr }
```

示例：

```nova
let t : Token = Token {
    kind: 1,
    lexeme: "fn",
    line: 1,
    column: 1
};
```

规则：

- 字段名必须存在。
- 每个字段必须出现一次。
- 不允许重复字段。
- 不允许未知字段。
- 字段顺序可以不同于声明顺序。
- 字段表达式类型必须匹配声明类型。

---

## 3.4 Field access

语法：

```nova
expr.field
```

示例：

```nova
print_int(p.x);
print_str(t.lexeme);
```

规则：

- `expr` 的类型必须是 struct。
- `field` 必须存在于该 struct。
- field access 的类型是该字段类型。

---

## 3.5 Field assignment

你已经把 `AssignExpr.target` 设计成 `Expr`，所以 Step 8 可以自然支持：

```nova
p.x = 10;
t.lexeme = "let";
```

语义规则：

- assignment target 可以是：
  - `IdentifierExpr`
  - `FieldAccessExpr`
- 右侧类型必须等于左侧字段类型。

---

## 4. Lexer 修改

新增关键字：

```text
struct
```

新增 token：

```text
DOT
```

对应字符：

```text
.
```

注意：

- `.` 当前只用于 field access。
- 不做 float literal，所以 `1.2` 应该报错或在 parser 阶段失败。

---

## 5. AST 修改建议

---

## 5.1 Type 表示需要升级

现在如果你的类型还是：

```cpp
enum class TypeKind { Int, Bool, Str, Void };
```

它已经不够了，因为需要表示用户自定义 struct 类型。

建议改成：

```cpp
enum class TypeKind {
    Int,
    Bool,
    Str,
    Void,
    Struct
};

struct Type {
    TypeKind kind;
    std::string name; // 仅当 kind == Struct 时使用
};
```

辅助函数：

```cpp
bool operator==(const Type& a, const Type& b);
std::string type_to_string(const Type& type);
```

示例：

- `int` -> `{ Int, "" }`
- `str` -> `{ Str, "" }`
- `Point` -> `{ Struct, "Point" }`

---

## 5.2 新增 AST 节点

### `StructDecl`

```cpp
struct StructField {
    std::string name;
    SourceLocation name_location;
    Type type;
};

struct StructDecl : Decl {
    std::string name;
    SourceLocation name_location;
    std::vector<StructField> fields;
};
```

### `StructLiteralExpr`

```cpp
struct StructLiteralField {
    std::string name;
    SourceLocation name_location;
    std::unique_ptr<Expr> value;
};

struct StructLiteralExpr : Expr {
    std::string type_name;
    SourceLocation type_location;
    std::vector<StructLiteralField> fields;
};
```

### `FieldAccessExpr`

```cpp
struct FieldAccessExpr : Expr {
    std::unique_ptr<Expr> object;
    std::string field_name;
    SourceLocation field_location;
};
```

---

## 5.3 Program 节点

`Program` 现在应包含：

```cpp
std::vector<std::unique_ptr<StructDecl>> structs;
std::vector<std::unique_ptr<FunctionDecl>> functions;
```

或者如果你有统一 `Decl`：

```cpp
std::vector<std::unique_ptr<Decl>> declarations;
```

对当前项目，推荐先分开存：

- `structs`
- `functions`

这样 sema/codegen 更简单。

---

## 6. Parser 修改

---

## 6.1 Top-level parsing

顶层现在可以是：

```text
struct declaration
function declaration
```

`parse_program()` 中根据当前 token 分派：

- `KW_STRUCT` -> `parse_struct_decl()`
- `KW_FN` -> `parse_function()`

否则报错。

---

## 6.2 `parse_struct_decl()`

解析：

```nova
struct Point {
    x: int;
    y: int;
}
```

流程：

1. consume `struct`
2. expect struct name
3. expect `{`
4. 循环解析 field：
   - field name
   - `:`
   - type
   - `;`
5. expect `}`

---

## 6.3 `parse_type()`

现在类型可以是：

- `int`
- `bool`
- `str`
- `void`
- `Identifier` 作为 struct type

注意：

- parser 不需要确认 struct 是否存在。
- struct 是否存在交给 sema。

---

## 6.4 Field access / postfix parsing

建议新增 `parse_postfix()`：

```text
postfix -> primary ( call | field_access )*
field_access -> '.' IDENT
call -> '(' arguments? ')'
```

这样以后可以自然支持：

```nova
p.x
make_point(1, 2).x
```

当前函数调用可以仍然限制 callee 是 identifier，也可以把 `CallExpr.callee` 继续保持 string。为了少改动，Step 8 可以先支持：

- `IDENT(...)` 函数调用
- `expr.field` field access

不必支持 `expr(...)` 调用。

---

## 6.5 Struct literal parsing

struct literal 形如：

```nova
Point { x: 1, y: 2 }
```

它和普通 identifier 有歧义：看到 `IDENT` 时，需要看后面是不是 `{`。

建议在 `parse_primary()` 或 `parse_postfix()` 中处理：

- 如果当前是 `IDENT`，且下一个 token 是 `{`，解析 struct literal。
- 否则如果后面是 `(`，解析 call。
- 否则是 identifier expression。

---

## 7. Sema 修改

---

## 7.1 Struct table

新增：

```cpp
std::unordered_map<std::string, StructInfo> structs_;
```

`StructInfo` 建议包含：

```cpp
struct FieldInfo {
    std::string name;
    Type type;
    SourceLocation location;
};

struct StructInfo {
    std::string name;
    SourceLocation location;
    std::vector<FieldInfo> fields;
    std::unordered_map<std::string, size_t> field_index;
};
```

---

## 7.2 第一遍收集 struct 声明

在分析函数前，先收集所有 struct：

```text
collect_struct_declarations(program)
collect_function_signatures(program)
analyze functions
```

检查：

- struct 重名
- struct 和 builtin type 重名：不允许 `struct int { ... }`
- 同一个 struct 内 field 重名

---

## 7.3 Type validation

所有声明类型都要检查：

- builtin type 合法
- struct type 必须存在
- `void` 不能作为变量类型、字段类型、参数类型
- `void` 只能作为函数返回类型

需要检查的位置：

- struct field type
- function parameter type
- function return type
- let declared type

---

## 7.4 Struct literal type checking

检查：

```nova
Point { x: 1, y: 2 }
```

规则：

- `Point` 必须是已定义 struct。
- 每个字段名必须存在。
- 不允许重复字段。
- 必须初始化所有字段。
- 字段表达式类型必须匹配字段声明类型。

表达式类型返回：

```text
Type { Struct, "Point" }
```

---

## 7.5 Field access type checking

检查：

```nova
p.x
```

规则：

- `p` 的类型必须是 struct。
- `x` 必须是该 struct 的字段。
- 返回字段类型。

错误例子：

```nova
let x : int = 1;
print_int(x.y);
```

应报：

```text
cannot access field 'y' on non-struct type int
```

---

## 7.6 Assignment target

现在合法 lvalue：

- identifier
- field access

非法：

```nova
1 = 2;
make_point(1, 2) = p;
```

语义规则：

- 如果 target 是 `FieldAccessExpr`，先 infer 其类型。
- value 类型必须匹配 target 类型。

---

## 8. Codegen 修改

---

## 8.1 Struct declaration -> C typedef struct

Nova：

```nova
struct Point {
    x: int;
    y: int;
}
```

C：

```c
typedef struct Point {
    int x;
    int y;
} Point;
```

生成顺序：

1. include runtime
2. struct declarations
3. function declarations/definitions

---

## 8.2 Struct literal -> C compound literal

Nova：

```nova
Point { x: 1, y: 2 }
```

C：

```c
((Point){ .x = 1, .y = 2 })
```

建议始终加外层括号。

---

## 8.3 Field access -> C field access

Nova：

```nova
p.x
```

C：

```c
p.x
```

如果 object 表达式复杂，建议加括号：

```c
(make_point(1, 2)).x
```

---

## 8.4 Function parameters / return values

Nova：

```nova
fn make_point(x: int, y: int) : Point {
    return Point { x: x, y: y };
}
```

C：

```c
Point make_point(int x, int y) {
    return ((Point){ .x = x, .y = y });
}
```

---

## 9. Runtime 修改

Step 8 不需要新增 runtime API。

struct 是编译器语言特性，直接 lowering 到 C struct，不需要 runtime 支持。

---

## 10. 测试建议

新增测试可以放在：

```text
tests/parser/positive/
tests/parser/negative/
tests/sema/positive/
tests/sema/negative/
tests/codegen/positive/
```

建议重点放在 sema 和 codegen。

---

## 10.1 Codegen positive tests

### `tests/codegen/positive/struct_basic.nv`

```nova
struct Point {
    x: int;
    y: int;
}

fn main() : void {
    let p : Point = Point { x: 3, y: 4 };
    print_int(p.x);
    print_int(p.y);
    return;
}
```

`.out`:

```text
3
4
```

### `tests/codegen/positive/struct_assignment.nv`

```nova
struct Point {
    x: int;
    y: int;
}

fn main() : void {
    let p : Point = Point { x: 1, y: 2 };
    p.x = 10;
    p.y = p.x + 5;
    print_int(p.x);
    print_int(p.y);
    return;
}
```

`.out`:

```text
10
15
```

### `tests/codegen/positive/struct_function_return.nv`

```nova
struct Token {
    kind: int;
    lexeme: str;
    line: int;
    column: int;
}

fn make_token(kind: int, lexeme: str, line: int, column: int) : Token {
    return Token { kind: kind, lexeme: lexeme, line: line, column: column };
}

fn main() : void {
    let t : Token = make_token(1, "fn", 2, 3);
    print_int(t.kind);
    print_str(t.lexeme);
    print_int(t.line);
    print_int(t.column);
    return;
}
```

`.out`:

```text
1
fn
2
3
```

### `tests/codegen/positive/struct_nested.nv`

```nova
struct Point {
    x: int;
    y: int;
}

struct Line {
    start: Point;
    end: Point;
}

fn main() : void {
    let l : Line = Line {
        start: Point { x: 1, y: 2 },
        end: Point { x: 3, y: 4 }
    };

    print_int(l.start.x);
    print_int(l.start.y);
    print_int(l.end.x);
    print_int(l.end.y);
    return;
}
```

`.out`:

```text
1
2
3
4
```

---

## 10.2 Sema negative tests

### `tests/sema/negative/unknown_struct_type.nv`

```nova
fn main() : void {
    let p : Point = Point { x: 1 };
    return;
}
```

Expected error should mention unknown struct type `Point`.

### `tests/sema/negative/duplicate_struct.nv`

```nova
struct Point {
    x: int;
}

struct Point {
    y: int;
}

fn main() : void {
    return;
}
```

Expected error should mention duplicate struct `Point`.

### `tests/sema/negative/duplicate_struct_field.nv`

```nova
struct Point {
    x: int;
    x: int;
}

fn main() : void {
    return;
}
```

Expected error should mention duplicate field `x`.

### `tests/sema/negative/missing_struct_field.nv`

```nova
struct Point {
    x: int;
    y: int;
}

fn main() : void {
    let p : Point = Point { x: 1 };
    return;
}
```

Expected error should mention missing field `y`.

### `tests/sema/negative/unknown_struct_field.nv`

```nova
struct Point {
    x: int;
}

fn main() : void {
    let p : Point = Point { x: 1, y: 2 };
    return;
}
```

Expected error should mention unknown field `y`.

### `tests/sema/negative/field_type_mismatch.nv`

```nova
struct Token {
    kind: int;
    lexeme: str;
}

fn main() : void {
    let t : Token = Token { kind: "fn", lexeme: "fn" };
    return;
}
```

Expected error should mention field type mismatch.

### `tests/sema/negative/access_field_on_non_struct.nv`

```nova
fn main() : void {
    let x : int = 1;
    print_int(x.y);
    return;
}
```

Expected error should mention field access on non-struct type.

### `tests/sema/negative/unknown_field_access.nv`

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

Expected error should mention unknown field `y`.

---

## 11. CMake 测试添加

如果继续使用已有测试 helper，只需要添加：

```cmake
add_codegen_positive_test(struct_basic)
add_codegen_positive_test(struct_assignment)
add_codegen_positive_test(struct_function_return)
add_codegen_positive_test(struct_nested)

add_sema_negative_test(unknown_struct_type)
add_sema_negative_test(duplicate_struct)
add_sema_negative_test(duplicate_struct_field)
add_sema_negative_test(missing_struct_field)
add_sema_negative_test(unknown_struct_field)
add_sema_negative_test(field_type_mismatch)
add_sema_negative_test(access_field_on_non_struct)
add_sema_negative_test(unknown_field_access)
```

---

## 12. 推荐实现顺序

### Step A：Lexer

添加：

- `struct` keyword
- `.` token

确保 token dump 正确。

### Step B：Type system refactor

把 `TypeKind` 升级为 `Type`：

- builtin types
- struct types

这一步最容易影响 sema 和 codegen，建议先做小范围测试。

### Step C：Parser struct declaration

支持：

```nova
struct Point { x: int; y: int; }
```

先只打印 AST dump，确认结构正确。

### Step D：Parser field access

支持：

```nova
p.x
p.x.y
```

### Step E：Parser struct literal

支持：

```nova
Point { x: 1, y: 2 }
```

### Step F：Sema struct table

实现：

- struct collection
- field lookup
- type validation

### Step G：Sema expression rules

支持：

- struct literal
- field access
- field assignment

### Step H：Codegen

支持：

- C struct declarations
- compound literals
- field access

### Step I：Tests

接入 parser / sema / codegen tests。

---

## 13. 本阶段最容易犯的错误

### 错误 1：只改 AST，不升级类型系统

如果类型仍然只是 enum，struct type 会非常难处理。

### 错误 2：把 struct literal 当函数调用

`Point { x: 1 }` 和 `Point(...)` 是不同语法。

### 错误 3：field access 没有做 postfix 层

如果直接塞进 primary，后续 `p.x.y` 会很难支持。

### 错误 4：codegen 忘记先生成 struct declarations

C 中使用 `Point` 前必须先声明。

### 错误 5：没有检查 missing field / duplicate field

struct literal 的字段完整性是本阶段 sema 的重点。

### 错误 6：struct 字段允许 void

`void` 不应作为字段类型。

---

## 14. 验收标准

### 合格

- 能声明 struct
- 能创建 struct literal
- 能访问字段
- 能生成可运行 C 代码

### 良好

- 支持 struct 参数和返回值
- 支持 field assignment
- sema 能检查常见错误
- codegen tests 通过

### 优秀

- 支持 nested struct
- field order 可任意
- 错误信息位置准确
- 类型系统代码清晰稳定

---

## 15. 完成标志

Step 8 完成的标志是：

> Nova 可以直接表示结构化数据，并能把 struct 程序编译到 C struct。

也就是说，你能写：

```nova
struct Token {
    kind: int;
    lexeme: str;
    line: int;
    column: int;
}
```

并让它通过完整链路：

```text
Nova struct source
  -> lexer
  -> parser
  -> sema
  -> C codegen
  -> native executable
```

---

## 16. 下一步预告

Step 9 可以考虑两条路线：

1. **实现 struct-aware Nova tokenizer library**：用 `Token` struct 重写 Step 7 的 lexer，让它不只是 dump token，而是真的构造 token 数据。
2. **实现 minimal array/vector language feature**：让 Nova 原生支持 `TokenVec` 这类结构，而不是完全依赖 runtime handle。

建议 Step 8 完成后再决定，因为 struct 会明显改变 Nova 写工具的方式。
