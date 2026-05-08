# Nova Project — Step 9
## 题目：给 Nova 增加 typed vector / dynamic array 能力

## 1. 本次作业目标

在 Step 8 中，Nova 已经支持了 `struct`，可以表达结构化数据，例如 `Token`、`SourceLocation`、`Point` 等。

但是继续往自举方向推进时，还缺一个关键能力：

> 能保存“一串”结构化对象。

例如后续用 Nova 写 tokenizer / parser 时，需要表示：

- `vec<Token>`
- `vec<Stmt>`
- `vec<Expr>`
- `vec<str>`
- `vec<int>`

Step 9 的目标是：

> **给 Nova 增加最小可用的 typed vector / dynamic array 能力。**

完成后，你应该能写：

```nova
struct Token {
    kind: int;
    lexeme: str;
    line: int;
    column: int;
}

fn main() : void {
    let tokens : vec<Token> = vec_new();

    vec_push(tokens, Token { kind: 1, lexeme: "fn", line: 1, column: 1 });
    vec_push(tokens, Token { kind: 2, lexeme: "main", line: 1, column: 4 });

    print_int(vec_len(tokens));
    print_str(vec_get(tokens, 0).lexeme);
    print_str(vec_get(tokens, 1).lexeme);
    return;
}
```

---

## 2. 本阶段核心原则

Step 9 只实现 **最小 typed vector**。

支持：

- `vec<T>` 类型
- `vec_new()`
- `vec_len(v)`
- `vec_push(v, value)`
- `vec_get(v, index)`
- `vec_set(v, index, value)`
- vector 作为变量、参数、返回值
- vector 元素可以是 builtin type 或 struct type

不做：

- vector literal
- slicing
- iteration syntax
- generics for user-defined functions
- array indexing syntax `v[i]`
- ownership / destructor
- nested vector `vec<vec<int>>`（可选，初版可以禁止）
- vector equality
- vector copy semantics 的深拷贝

本阶段的 vector 可以是 **reference-like handle semantics**：

```nova
let a : vec<int> = vec_new();
let b : vec<int> = a;
vec_push(b, 1);
print_int(vec_len(a)); // 可以输出 1
```

也就是说，vector 变量复制的是底层 vector handle，而不是复制整个数组。

---

## 3. 新增类型语法

新增类型：

```nova
vec<T>
```

示例：

```nova
let xs : vec<int> = vec_new();
let names : vec<str> = vec_new();
let tokens : vec<Token> = vec_new();
```

推荐 lexer 新增关键字：

```text
vec
```

也可以在 type context 中把 `vec` 当作普通 identifier 特判，但作为关键字更清楚。

---

## 4. 新增内建 vector intrinsic

这些不是普通 runtime builtin 函数，而是 **typed intrinsic**。

```nova
vec_new() : vec<T>
vec_len(v: vec<T>) : int
vec_push(v: vec<T>, value: T) : void
vec_get(v: vec<T>, index: int) : T
vec_set(v: vec<T>, index: int, value: T) : void
```

注意：

- `T` 不是 Nova 用户层面的泛型。
- sema 根据 `vec<T>` 参数或上下文推导 `T`。
- codegen 根据 `T` 生成或调用对应的 C helper。

---

## 5. 为什么 `vec_new()` 需要上下文类型

`vec_new()` 本身没有参数，无法从参数推断元素类型。

所以：

```nova
let xs : vec<int> = vec_new();
```

合法，因为 let 声明给出了 expected type。

但：

```nova
vec_new();
```

应报错，因为无法推断 `T`。

建议 sema 支持：

```cpp
Type infer_expr_type(const Expr& expr, const Type* expected_type = nullptr);
```

然后：

- `LetStmt` 检查 initializer 时，把 declared type 作为 expected type 传入。
- `ReturnStmt` 检查 return expr 时，把 current return type 作为 expected type 传入。
- struct literal field initializer 检查时，把 field type 作为 expected type 传入。
- 普通表达式没有 expected type。

`vec_new()` 规则：

- 如果 expected type 是 `vec<T>`，返回该 expected type。
- 否则报 `cannot infer element type for vec_new`。

---

## 6. AST / Type 修改

## 6.1 TypeKind

当前 Step 8 后大概已有：

```cpp
enum class TypeKind {
    Int,
    Bool,
    Str,
    Void,
    Struct
};
```

Step 9 建议改成：

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

## 6.2 Type 结构

因为 `vec<T>` 是递归类型，需要让 `Type` 能保存 element type。

推荐：

```cpp
struct Type {
    TypeKind kind;
    std::string name;                  // Struct 时使用
    std::shared_ptr<Type> element_type; // Vec 时使用
};
```

辅助构造函数：

```cpp
Type int_type();
Type bool_type();
Type str_type();
Type void_type();
Type struct_type(std::string name);
Type vec_type(Type element);
```

比较规则：

- builtin type 按 kind 比较
- struct type 要求 name 相同
- vec type 要求 element type 相同

字符串化：

```text
int
bool
str
void
Point
vec<int>
vec<Token>
```

---

## 7. Parser 修改

## 7.1 Lexer

新增关键字：

```text
vec
```

如果你将 `vec` 作为 keyword，则 token 可叫：

```text
KW_VEC
```

`<` 和 `>` 已经作为比较运算符存在，不需要新增 token。

---

## 7.2 parse_type()

支持：

```nova
vec<int>
vec<str>
vec<Token>
```

伪逻辑：

```text
if type is int/bool/str/void:
    return builtin type

if type is IDENT:
    return struct type

if type is vec:
    expect '<'
    elem = parse_type()
    expect '>'
    return vec<elem>
```

建议初版禁止：

```nova
vec<void>
vec<vec<int>>
```

也可以让 sema 报错，不一定 parser 报。

---

## 8. Sema 修改

## 8.1 Type validation

新增规则：

- `vec<void>` 非法
- `vec<UnknownStruct>` 非法
- `vec<T>` 的 element type 也需要递归验证
- 初版可以禁止 nested vector：`vec<vec<int>>`

需要检查的位置：

- let declared type
- function parameter type
- function return type
- struct field type
- vector intrinsic 调用

---

## 8.2 Expression type annotation（强烈建议）

Step 9 后，codegen 需要知道：

- `vec_push(tokens, token)` 中 tokens 的元素类型
- `vec_get(tokens, 0)` 返回什么 C 类型
- `vec_new()` 生成哪个 typed vector helper

因此建议在 `Expr` 上加一个可选 resolved type：

```cpp
struct Expr : ASTNode {
    mutable std::optional<Type> resolved_type;
};
```

或者如果 sema visitor 不是 const，也可以不用 `mutable`。

在 `infer_expr_type()` 结束时，写入：

```cpp
expr.resolved_type = result;
```

这样 codegen 不需要重新做类型推导。

---

## 8.3 Vector intrinsic 检查规则

### `vec_new()`

合法场景：

```nova
let xs : vec<int> = vec_new();
```

检查：

- 参数数量必须是 0
- expected type 必须是 `vec<T>`
- 返回 expected type

错误：

```nova
vec_new();
```

报：

```text
cannot infer element type for vec_new
```

---

### `vec_len(v)`

检查：

- 参数数量为 1
- 参数类型必须是 `vec<T>`
- 返回 `int`

---

### `vec_push(v, value)`

检查：

- 参数数量为 2
- 第一个参数必须是 `vec<T>`
- 第二个参数类型必须是 `T`
- 返回 `void`

---

### `vec_get(v, index)`

检查：

- 参数数量为 2
- 第一个参数必须是 `vec<T>`
- 第二个参数必须是 `int`
- 返回 `T`

---

### `vec_set(v, index, value)`

检查：

- 参数数量为 3
- 第一个参数必须是 `vec<T>`
- 第二个参数必须是 `int`
- 第三个参数类型必须是 `T`
- 返回 `void`

---

## 9. Codegen 修改

## 9.1 C 类型映射

Nova type 到 C type：

| Nova | C |
|---|---|
| `int` | `int` |
| `bool` | `bool` |
| `str` | `const char*` |
| `Point` | `Point` |
| `vec<int>` | `NovaVec_int*` |
| `vec<str>` | `NovaVec_str*` |
| `vec<Point>` | `NovaVec_Point*` |

需要一个 type mangling helper：

```cpp
std::string mangle_type(const Type& type);
```

示例：

```text
int        -> int
bool       -> bool
str        -> str
Point      -> Point
vec<int>   -> vec_int
vec<Point> -> vec_Point
```

C vector type name：

```cpp
NovaVec_ + mangle_type(element_type)
```

---

## 9.2 生成 specialized vector helpers

对每个用到的 `vec<T>`，codegen 生成一套 C helper。

例如 `vec<Token>`：

```c
typedef struct NovaVec_Token {
    Token* data;
    int len;
    int cap;
} NovaVec_Token;

static NovaVec_Token* nova_vec_new_Token(void) {
    NovaVec_Token* v = malloc(sizeof(NovaVec_Token));
    if (!v) nova_runtime_error("out of memory");
    v->data = NULL;
    v->len = 0;
    v->cap = 0;
    return v;
}

static int nova_vec_len_Token(NovaVec_Token* v) {
    return v->len;
}

static void nova_vec_push_Token(NovaVec_Token* v, Token value) {
    if (v->len == v->cap) {
        int new_cap = v->cap == 0 ? 4 : v->cap * 2;
        Token* new_data = realloc(v->data, sizeof(Token) * new_cap);
        if (!new_data) nova_runtime_error("out of memory");
        v->data = new_data;
        v->cap = new_cap;
    }
    v->data[v->len] = value;
    v->len = v->len + 1;
}

static Token nova_vec_get_Token(NovaVec_Token* v, int index) {
    if (index < 0 || index >= v->len) {
        nova_runtime_error("vector index out of bounds");
    }
    return v->data[index];
}

static void nova_vec_set_Token(NovaVec_Token* v, int index, Token value) {
    if (index < 0 || index >= v->len) {
        nova_runtime_error("vector index out of bounds");
    }
    v->data[index] = value;
}
```

需要 runtime 暴露：

```c
void nova_runtime_error(const char* message);
```

否则 generated C helper 不能调用 runtime error。

---

## 9.3 何时生成 vector helpers

最简单做法：

1. codegen 前遍历 AST，收集所有出现的 `vec<T>` 类型：
   - let declared type
   - function parameter type
   - function return type
   - struct field type
   - expression resolved type
2. 对每个唯一 element type 生成 helper。

初版也可以在 codegen 遇到 `vec<T>` 时 lazily 注册，然后在输出函数前统一 emit helper。

输出顺序建议：

```text
#include "nova_runtime.h"

struct declarations

vector helper declarations/definitions

function definitions
```

---

## 9.4 Vector intrinsic codegen

根据 `CallExpr.resolved_type` 或参数类型生成对应 helper call。

### `vec_new()`

如果 call resolved type 是 `vec<Token>`：

```c
nova_vec_new_Token()
```

### `vec_len(tokens)`

如果第一个参数类型是 `vec<Token>`：

```c
nova_vec_len_Token(tokens)
```

### `vec_push(tokens, t)`

```c
nova_vec_push_Token(tokens, t)
```

### `vec_get(tokens, 0)`

```c
nova_vec_get_Token(tokens, 0)
```

### `vec_set(tokens, 0, t)`

```c
nova_vec_set_Token(tokens, 0, t)
```

---

## 10. Runtime 修改

Step 9 只需要新增一个 public runtime error function：

```c
void nova_runtime_error(const char* message);
```

实现可以直接调用你已有的内部 `runtime_error`：

```c
void nova_runtime_error(const char* message) {
    runtime_error(message);
}
```

vector helper 本身由 codegen 生成，不放在 runtime.c 中。

---

## 11. 测试建议

主要测试放在：

```text
tests/codegen/positive/
tests/sema/negative/
```

---

## 11.1 Codegen positive tests

### `tests/codegen/positive/vec_int_basic.nv`

```nova
fn main() : void {
    let xs : vec<int> = vec_new();

    vec_push(xs, 10);
    vec_push(xs, 20);
    vec_push(xs, 30);

    print_int(vec_len(xs));
    print_int(vec_get(xs, 0));
    print_int(vec_get(xs, 1));
    print_int(vec_get(xs, 2));

    return;
}
```

### `tests/codegen/positive/vec_int_basic.out`

```text
3
10
20
30
```

---

### `tests/codegen/positive/vec_struct_token.nv`

```nova
struct Token {
    kind: int;
    lexeme: str;
    line: int;
    column: int;
}

fn main() : void {
    let tokens : vec<Token> = vec_new();

    vec_push(tokens, Token { kind: 1, lexeme: "fn", line: 1, column: 1 });
    vec_push(tokens, Token { kind: 2, lexeme: "main", line: 1, column: 4 });

    print_int(vec_len(tokens));
    print_str(vec_get(tokens, 0).lexeme);
    print_str(vec_get(tokens, 1).lexeme);

    return;
}
```

### `tests/codegen/positive/vec_struct_token.out`

```text
2
fn
main
```

---

### `tests/codegen/positive/vec_set.nv`

```nova
fn main() : void {
    let xs : vec<int> = vec_new();

    vec_push(xs, 1);
    vec_push(xs, 2);
    vec_push(xs, 3);

    vec_set(xs, 1, 42);

    print_int(vec_get(xs, 0));
    print_int(vec_get(xs, 1));
    print_int(vec_get(xs, 2));

    return;
}
```

### `tests/codegen/positive/vec_set.out`

```text
1
42
3
```

---

### `tests/codegen/positive/vec_function_param.nv`

```nova
fn sum(xs: vec<int>) : int {
    let i : int = 0;
    let total : int = 0;

    while (i < vec_len(xs)) {
        total = total + vec_get(xs, i);
        i = i + 1;
    }

    return total;
}

fn main() : void {
    let xs : vec<int> = vec_new();

    vec_push(xs, 5);
    vec_push(xs, 7);
    vec_push(xs, 9);

    print_int(sum(xs));
    return;
}
```

### `tests/codegen/positive/vec_function_param.out`

```text
21
```

---

### `tests/codegen/positive/vec_function_return.nv`

```nova
fn make_numbers() : vec<int> {
    let xs : vec<int> = vec_new();

    vec_push(xs, 3);
    vec_push(xs, 4);
    vec_push(xs, 5);

    return xs;
}

fn main() : void {
    let xs : vec<int> = make_numbers();

    print_int(vec_len(xs));
    print_int(vec_get(xs, 0));
    print_int(vec_get(xs, 1));
    print_int(vec_get(xs, 2));

    return;
}
```

### `tests/codegen/positive/vec_function_return.out`

```text
3
3
4
5
```

---

### `tests/codegen/positive/vec_of_str.nv`

```nova
fn main() : void {
    let parts : vec<str> = vec_new();

    vec_push(parts, "hello");
    vec_push(parts, " ");
    vec_push(parts, "nova");

    let b : int = buf_new();
    let i : int = 0;

    while (i < vec_len(parts)) {
        buf_push_str(b, vec_get(parts, i));
        i = i + 1;
    }

    print_str(buf_to_str(b));
    return;
}
```

### `tests/codegen/positive/vec_of_str.out`

```text
hello nova
```

---

## 11.2 Sema negative tests

### `tests/sema/negative/vec_new_without_context.nv`

```nova
fn main() : void {
    vec_new();
    return;
}
```

### `tests/sema/negative/vec_new_without_context.err`

```text
SemanticError at 2:5: cannot infer element type for vec_new
```

---

### `tests/sema/negative/vec_push_type_mismatch.nv`

```nova
fn main() : void {
    let xs : vec<int> = vec_new();
    vec_push(xs, "bad");
    return;
}
```

### `tests/sema/negative/vec_push_type_mismatch.err`

```text
SemanticError at 3:18: type error in vec_push value: expected int, got str
```

---

### `tests/sema/negative/vec_get_index_not_int.nv`

```nova
fn main() : void {
    let xs : vec<int> = vec_new();
    let x : int = vec_get(xs, true);
    return;
}
```

### `tests/sema/negative/vec_get_index_not_int.err`

```text
SemanticError at 3:29: type error in vec_get index: expected int, got bool
```

---

### `tests/sema/negative/vec_len_on_non_vec.nv`

```nova
fn main() : void {
    let x : int = 1;
    print_int(vec_len(x));
    return;
}
```

### `tests/sema/negative/vec_len_on_non_vec.err`

```text
SemanticError at 3:23: vec_len expects vec<T>
```

---

### `tests/sema/negative/vec_void_element.nv`

```nova
fn main() : void {
    let xs : vec<void> = vec_new();
    return;
}
```

### `tests/sema/negative/vec_void_element.err`

```text
SemanticError at 2:14: vector element type cannot be void
```

---

### `tests/sema/negative/vec_unknown_struct_element.nv`

```nova
fn main() : void {
    let xs : vec<Token> = vec_new();
    return;
}
```

### `tests/sema/negative/vec_unknown_struct_element.err`

```text
SemanticError at 2:14: unknown struct type 'Token'
```

---

## 12. CMake 测试添加

如果已有 helper，加入：

```cmake
add_codegen_positive_test(vec_int_basic)
add_codegen_positive_test(vec_struct_token)
add_codegen_positive_test(vec_set)
add_codegen_positive_test(vec_function_param)
add_codegen_positive_test(vec_function_return)
add_codegen_positive_test(vec_of_str)

add_sema_negative_test(vec_new_without_context)
add_sema_negative_test(vec_push_type_mismatch)
add_sema_negative_test(vec_get_index_not_int)
add_sema_negative_test(vec_len_on_non_vec)
add_sema_negative_test(vec_void_element)
add_sema_negative_test(vec_unknown_struct_element)
```

---

## 13. 推荐实现顺序

### Step A：Type 支持 `vec<T>`

先把 `TypeKind::Vec`、type equality、`type_to_string` 做好。

### Step B：Parser 支持 `vec<T>`

先让 AST dump 能显示：

```text
vec<int>
vec<Token>
```

### Step C：Sema 支持 type validation

先检测 `vec<void>`、`vec<UnknownStruct>`。

### Step D：Sema 支持 vector intrinsic

先做：

- `vec_new`
- `vec_len`

再做：

- `vec_push`
- `vec_get`
- `vec_set`

### Step E：表达式类型注解

把 `Expr.resolved_type` 接上，确保 codegen 能读到。

### Step F：Codegen 生成 vector helper

先支持 `vec<int>`，再支持 `vec<str>` 和 `vec<Struct>`。

### Step G：Codegen intrinsic call lowering

把：

```nova
vec_push(xs, 1)
```

lower 成：

```c
nova_vec_push_int(xs, 1)
```

### Step H：Tests

逐步接入所有 vec tests。

---

## 14. 本阶段最容易犯的错误

### 错误 1：把 `vec_new` 当普通函数注册

`vec_new()` 是 polymorphic intrinsic，没有参数，必须依赖 expected type。

### 错误 2：codegen 不知道 vector element type

这就是为什么建议 sema 给表达式写入 `resolved_type`。

### 错误 3：vector 作为值深拷贝

Step 9 先用 handle semantics，不做深拷贝。

### 错误 4：generated C helper 顺序错误

struct declarations 必须先生成，然后才能生成 `NovaVec_Token`。

### 错误 5：`vec_get` 返回 struct 时 field access 加括号不对

建议生成：

```c
(nova_vec_get_Token(tokens, 0)).lexeme
```

---

## 15. 验收标准

### 合格

- `vec<int>` 可用
- `vec_new / vec_push / vec_get / vec_len` 可用
- codegen tests 能跑

### 良好

- `vec<str>` 和 `vec<Struct>` 可用
- 支持 vector 参数和返回值
- sema negative tests 覆盖常见错误

### 优秀

- `vec_set` 可用
- 生成 helper 去重
- 表达式类型注解清晰
- 后续可用于 Nova tokenizer library

---

## 16. 完成标志

Step 9 完成的标志是：

> Nova 可以用 `struct Token` 和 `vec<Token>` 表示真正的 token list。

这意味着后续可以把 Step 7 的 token dump lexer 改造成：

```text
read source
-> produce vec<Token>
-> dump tokens
```

这是 Nova 自举路线中非常关键的一步。
