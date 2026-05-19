# Nova Project — Step 14
## 题目：用 Nova 实现 Declaration-Level Checker Prototype

---

## 1. 本次作业目标

在 Step 13 中，Nova tools 已经完成了两件关键升级：

```text
source -> tokenize(source) -> vec<Token>
source -> parse_program_tree(tokens) -> ParseNode
ParseNode -> dump_parse_tree(tree)
```

同时，`Token.kind` 已经从 `str` 迁移到了 `TokenKind enum`，parser 也不再只是直接输出 trace，而是先构造一个通用 `ParseNode` tree。

Step 14 的目标是：

> **基于 Step 13 的 `ParseNode`，用 Nova 写一个 declaration-level checker prototype。**

也就是说，本阶段要实现：

```text
source
  -> tokenize
  -> parse_program_tree
  -> check_program
  -> Check OK / diagnostics
```

这一步不是完整语义分析器，也不做表达式类型检查。它的重点是：

- 遍历 `ParseNode`
- 收集顶层符号
- 检查重复声明
- 检查声明类型是否存在
- 检查 `main` 函数
- 检查 struct / enum / function signature
- 做浅层 let declaration 检查

完成后，Nova frontend 就不只是“能 parse”，而是开始能“理解程序声明结构”。

---

## 2. 本阶段定位

Step 14 是 self-hosted frontend 的第一个 semantic prototype。

你已经有：

```text
Step 10: Nova tokenizer
Step 11: Nova parser trace
Step 12: enum
Step 13: TokenKind + ParseNode
```

Step 14 要增加：

```text
ParseNode -> declaration-level semantic checks
```

它对应 C++ compiler 中 semantic analyzer 的很小一部分，但只做声明级别，不检查完整表达式。

---

## 3. 本阶段不做什么

本阶段明确不做：

- expression AST
- expression type checking
- function call checking
- return expression type checking
- assignment target checking
- struct literal field checking
- vec intrinsic type checking
- full semantic analyzer
- codegen in Nova
- replacing C++ sema
- module/import

这些留给后续 Step 15 / 16。

本阶段只做：

```text
declaration-level checker
```

---

## 4. 建议文件结构

新增：

```text
tools/
  nova_check.nv
```

测试新增：

```text
tests/tools/checker/
  positive/
  negative/
```

建议：

```text
tests/tools/checker/positive/
  minimal_function.nv
  structs_enums_functions.nv
  recursive_parse_node_shape.nv
  local_lets.nv

tests/tools/checker/negative/
  duplicate_function.nv
  duplicate_struct.nv
  duplicate_enum.nv
  struct_enum_name_conflict.nv
  duplicate_struct_field.nv
  duplicate_enum_member.nv
  unknown_field_type.nv
  void_field_type.nv
  direct_self_field.nv
  invalid_main_return.nv
  missing_main.nv
  duplicate_param.nv
  void_param.nv
  unknown_let_type.nv
  void_let_type.nv
  duplicate_let_same_block.nv
```

`nova_check.nv` 可以暂时复制 `nova_parser.nv` 中的 tokenizer/parser/ParseNode 代码。等 Step 18 做 import/module 后再拆共享库。

---

## 5. Tool 行为

建议命令：

```bash
nova_check <input.nv> <output.check>
```

成功输出：

```text
Check OK
Structs: 1
Enums: 1
Functions: 2
```

失败时 runtime error：

```text
CheckerError at 3:5: duplicate field 'x' in struct 'Point'
```

由于 Nova 当前没有异常系统，checker error 可以继续用：

```nova
nova_runtime_error(message);
```

---

## 6. Checker 输入输出 API

建议新增：

```nova
fn check_program(tree: ParseNode) : str
```

成功返回 summary：

```text
Check OK
Structs: <n>
Enums: <n>
Functions: <n>
```

失败直接 runtime error。

`main()`：

```nova
fn main() : void {
    if (arg_count() != 3) {
        print_str("Usage: nova_check <input.nv> <output.check>");
        return;
    }

    let input_path : str = arg_get(1);
    let output_path : str = arg_get(2);

    let source : str = read_file(input_path);
    let tokens : vec<Token> = tokenize(source);
    let tree : ParseNode = parse_program_tree(tokens);
    let result : str = check_program(tree);

    write_file(output_path, result);
    return;
}
```

---

## 7. 为什么基于 ParseNode 而不是重新 parse

Step 14 不应该重新扫描 token。

正确链路是：

```text
tokens -> ParseNode -> checker
```

这样后续 Step 15/16 可以继续复用同一棵 parse tree。

checker 只看：

```nova
struct ParseNode {
    kind: ParseNodeKind;
    name: str;
    value: str;
    children: vec<ParseNode>;
}
```

例如：

```text
Struct name=Token
  Field name=kind type=TokenKind

Function name=main return=void
  Block
    ReturnStmt
```

---

## 8. 建议新增 checker 数据结构

### 8.1 SymbolTables

Nova 当前没有 map，所以先用 `vec<str>` 保存名称集合。

```nova
struct SymbolTables {
    structs: vec<str>;
    enums: vec<str>;
    functions: vec<str>;
}
```

构造：

```nova
fn make_symbol_tables() : SymbolTables {
    return SymbolTables {
        structs: vec_new(),
        enums: vec_new(),
        functions: vec_new()
    };
}
```

---

### 8.2 CheckerStats

用于成功输出：

```nova
struct CheckerStats {
    struct_count: int;
    enum_count: int;
    function_count: int;
}
```

也可以直接用 `vec_len(symbols.structs)` 等，不一定需要单独 struct。

---

### 8.3 Scope

如果你要做浅层 let 检查，可以定义：

```nova
struct Scope {
    names: vec<str>;
}
```

因为没有 stack/map，可以先让 `check_block` 每次创建一个本地 `vec<str>`。

---

## 9. 必备 helper

### 9.1 vec_str_contains

```nova
fn vec_str_contains(xs: vec<str>, name: str) : bool {
    let i : int = 0;
    while (i < vec_len(xs)) {
        if (str_eq(vec_get(xs, i), name)) {
            return true;
        }
        i = i + 1;
    }
    return false;
}
```

---

### 9.2 add_unique_name

```nova
fn add_unique_name(xs: vec<str>, name: str, error_message: str) : void {
    if (vec_str_contains(xs, name)) {
        nova_runtime_error(error_message);
    }
    vec_push(xs, name);
}
```

更好的版本可以传 line/column，但 ParseNode 当前没有 location，所以 Step 14 可以先不带精确位置。

如果你想要更精确 diagnostics，Step 14 也可以给 `ParseNode` 增加：

```nova
line: int;
column: int;
```

这会让 checker error 更像真正 compiler。可选。

---

### 9.3 is_builtin_type

```nova
fn is_builtin_type(name: str) : bool {
    return str_eq(name, "int") ||
           str_eq(name, "bool") ||
           str_eq(name, "str") ||
           str_eq(name, "void");
}
```

---

### 9.4 is_keyword_name

用于检查 symbol name：

```nova
fn is_keyword_name(name: str) : bool {
    return str_eq(name, "fn") ||
           str_eq(name, "let") ||
           str_eq(name, "if") ||
           str_eq(name, "else") ||
           str_eq(name, "while") ||
           str_eq(name, "return") ||
           str_eq(name, "struct") ||
           str_eq(name, "enum") ||
           str_eq(name, "vec") ||
           str_eq(name, "int") ||
           str_eq(name, "bool") ||
           str_eq(name, "str") ||
           str_eq(name, "void") ||
           str_eq(name, "true") ||
           str_eq(name, "false");
}
```

---

## 10. Type text checking

因为 ParseNode 里 type 是字符串：

```text
int
Token
TokenKind
vec<Token>
```

Step 14 需要一个浅层 type validator：

```nova
fn is_known_type(type_text: str, symbols: SymbolTables) : bool
```

规则：

- `int`, `bool`, `str`, `void` 是 builtin
- struct name 合法
- enum name 合法
- `vec<T>` 合法，当且仅当 T 是合法且不是 `void`
- 先不支持 nested vec：`vec<vec<int>>` 非法

---

## 11. parse type text helper

由于 type 已经是字符串，你可以写一些字符串 helper：

```nova
fn starts_with_vec(type_text: str) : bool
fn vec_element_type(type_text: str) : str
```

例如：

```nova
fn is_vec_type(type_text: str) : bool {
    return str_starts_with(type_text, "vec<");
}
```

如果你的 runtime 有 `str_len`, `str_get`, `str_slice`，则：

```nova
fn vec_element_type(type_text: str) : str {
    // "vec<Token>" -> "Token"
    return str_slice(type_text, 4, str_len(type_text) - 1);
}
```

注意 `str_slice` 的 end 是不是 exclusive，按你 runtime 约定调整。

---

## 12. Type validation rules

### 12.1 General type

```nova
fn check_type_exists(type_text: str, symbols: SymbolTables, context: str) : void
```

规则：

- builtin type OK
- struct / enum name OK
- `vec<T>` 递归检查 T
- unknown name 报错

---

### 12.2 Type cannot be void

某些位置不允许 void：

- struct field
- function parameter
- let variable
- vec element

但 function return type 可以是 void。

建议：

```nova
fn check_non_void_type(type_text: str, symbols: SymbolTables, context: str) : void
```

---

### 12.3 Direct self field

非法：

```nova
struct Node {
    next: Node;
}
```

合法：

```nova
struct Node {
    children: vec<Node>;
}
```

在检查 struct fields 时：

```nova
if (str_eq(field.value, struct_name)) {
    error
}
```

不要禁止：

```nova
vec<Node>
```

---

## 13. Pass 1：Collect top-level symbols

函数：

```nova
fn collect_top_level_symbols(program: ParseNode) : SymbolTables
```

遍历 `program.children`：

- `StructDecl` -> add to `symbols.structs`
- `EnumDecl` -> add to `symbols.enums`
- `FunctionDecl` -> add to `symbols.functions`

检查：

- duplicate struct
- duplicate enum
- duplicate function
- struct 和 enum 不能同名
- function 和 type 名可以先允许，也可以禁止；建议禁止更简单
- name 不能是 keyword

推荐规则：

```text
struct / enum / function 全部共享一个顶层名字空间
```

这样 checker 简单。

---

## 14. Pass 2：Validate top-level declarations

函数：

```nova
fn validate_declarations(program: ParseNode, symbols: SymbolTables) : void
```

对每个 child：

- StructDecl -> `check_struct_decl`
- EnumDecl -> `check_enum_decl`
- FunctionDecl -> `check_function_decl`

---

## 15. Struct declaration checks

输入节点：

```text
Struct name=Point
  Field name=x type=int
  Field name=y type=int
```

检查：

1. field name 不能重复
2. field name 不能是 keyword
3. field type exists
4. field type cannot be void
5. direct self field illegal
6. `vec<Self>` allowed

函数：

```nova
fn check_struct_decl(node: ParseNode, symbols: SymbolTables) : void
```

---

## 16. Enum declaration checks

输入节点：

```text
Enum name=Color
  Member name=Red
  Member name=Green
```

检查：

1. enum 至少一个 member
2. member name 不能重复
3. member name 不能是 keyword

函数：

```nova
fn check_enum_decl(node: ParseNode) : void
```

---

## 17. Function declaration checks

输入：

```text
Function name=main return=void
  Param name=x type=int
  Block
```

检查：

1. return type exists
2. parameter type exists
3. parameter type cannot be void
4. duplicate parameter
5. parameter name cannot be keyword
6. main signature:
   - main 必须存在
   - main return must be void
   - main parameter count must be 0

函数：

```nova
fn check_function_decl(node: ParseNode, symbols: SymbolTables) : void
```

---

## 18. Block shallow checks

虽然 Step 14 不做 expression type checking，但可以检查 let 声明类型。

输入：

```text
Block
  LetStmt name=x type=int
  LetStmt name=y type=Missing
  IfStmt
    Then
      Block
        LetStmt ...
```

检查：

1. let type exists
2. let type cannot be void
3. duplicate let in same block
4. nested block 有新作用域
5. if/while body 递归检查 block

函数：

```nova
fn check_block(node: ParseNode, symbols: SymbolTables) : void
```

建议同一个 block 内：

```nova
let x : int = 1;
let x : int = 2;
```

报错。

不同 nested block 内允许 shadow：

```nova
let x : int = 1;
{
    let x : int = 2;
}
```

---

## 19. Branch / while checks

`IfStmt` tree 大概：

```text
IfStmt
  Then
    Block
  Else
    Block
```

检查：

- ThenBranch 必须有 Block child
- ElseBranch 如果存在，必须有 Block child
- 递归检查 block

`WhileStmt`：

```text
WhileStmt
  Block
```

检查：

- 必须有 Block child
- 递归检查 block

---

## 20. check_program flow

```nova
fn check_program(tree: ParseNode) : str {
    let symbols : SymbolTables = collect_top_level_symbols(tree);

    validate_declarations(tree, symbols);
    check_main(tree, symbols);

    let out : int = buf_new();
    buf_push_str(out, "Check OK\n");
    buf_push_str(out, "Structs: ");
    buf_push_int(out, vec_len(symbols.structs));
    buf_push_str(out, "\n");
    buf_push_str(out, "Enums: ");
    buf_push_int(out, vec_len(symbols.enums));
    buf_push_str(out, "\n");
    buf_push_str(out, "Functions: ");
    buf_push_int(out, vec_len(symbols.functions));
    buf_push_str(out, "\n");
    return buf_to_str(out);
}
```

---

## 21. Positive tests

### 21.1 `tests/tools/checker/positive/minimal_function.nv`

```nova
fn main() : void {
    return;
}
```

### `tests/tools/checker/positive/minimal_function.out`

```text
Check OK
Structs: 0
Enums: 0
Functions: 1
```

---

### 21.2 `tests/tools/checker/positive/structs_enums_functions.nv`

```nova
enum TokenKind {
    KW_FN;
    IDENT;
    EOF;
}

struct Token {
    kind: TokenKind;
    lexeme: str;
    line: int;
    column: int;
}

fn make_token(kind: TokenKind, lexeme: str, line: int, column: int) : Token {
    return Token { kind: kind, lexeme: lexeme, line: line, column: column };
}

fn main() : void {
    return;
}
```

### `tests/tools/checker/positive/structs_enums_functions.out`

```text
Check OK
Structs: 1
Enums: 1
Functions: 2
```

---

### 21.3 `tests/tools/checker/positive/recursive_parse_node_shape.nv`

```nova
enum ParseNodeKind {
    Program;
    FunctionDecl;
}

struct ParseNode {
    kind: ParseNodeKind;
    name: str;
    children: vec<ParseNode>;
}

fn main() : void {
    return;
}
```

### `tests/tools/checker/positive/recursive_parse_node_shape.out`

```text
Check OK
Structs: 1
Enums: 1
Functions: 1
```

---

### 21.4 `tests/tools/checker/positive/local_shadow_nested_block.nv`

```nova
fn main() : void {
    let x : int = 1;
    {
        let x : int = 2;
    }
    return;
}
```

### `tests/tools/checker/positive/local_shadow_nested_block.out`

```text
Check OK
Structs: 0
Enums: 0
Functions: 1
```

---

## 22. Negative tests

### 22.1 `tests/tools/checker/negative/duplicate_function.nv`

```nova
fn main() : void {
    return;
}

fn main() : void {
    return;
}
```

### `tests/tools/checker/negative/duplicate_function.err`

```text
CheckerError: duplicate top-level name 'main'
```

---

### 22.2 `tests/tools/checker/negative/duplicate_struct_field.nv`

```nova
struct Point {
    x: int;
    x: int;
}

fn main() : void {
    return;
}
```

### `tests/tools/checker/negative/duplicate_struct_field.err`

```text
CheckerError: duplicate field 'x' in struct 'Point'
```

---

### 22.3 `tests/tools/checker/negative/duplicate_enum_member.nv`

```nova
enum Color {
    Red;
    Red;
}

fn main() : void {
    return;
}
```

### `tests/tools/checker/negative/duplicate_enum_member.err`

```text
CheckerError: duplicate member 'Red' in enum 'Color'
```

---

### 22.4 `tests/tools/checker/negative/unknown_field_type.nv`

```nova
struct Box {
    value: Missing;
}

fn main() : void {
    return;
}
```

### `tests/tools/checker/negative/unknown_field_type.err`

```text
CheckerError: unknown type 'Missing'
```

---

### 22.5 `tests/tools/checker/negative/void_field_type.nv`

```nova
struct Bad {
    value: void;
}

fn main() : void {
    return;
}
```

### `tests/tools/checker/negative/void_field_type.err`

```text
CheckerError: field 'value' in struct 'Bad' cannot have void type
```

---

### 22.6 `tests/tools/checker/negative/direct_self_field.nv`

```nova
struct Node {
    next: Node;
}

fn main() : void {
    return;
}
```

### `tests/tools/checker/negative/direct_self_field.err`

```text
CheckerError: struct 'Node' cannot contain field 'next' of its own type
```

---

### 22.7 `tests/tools/checker/negative/missing_main.nv`

```nova
fn helper() : void {
    return;
}
```

### `tests/tools/checker/negative/missing_main.err`

```text
CheckerError: missing main function
```

---

### 22.8 `tests/tools/checker/negative/invalid_main_return.nv`

```nova
fn main() : int {
    return 1;
}
```

### `tests/tools/checker/negative/invalid_main_return.err`

```text
CheckerError: main function must return void
```

---

### 22.9 `tests/tools/checker/negative/duplicate_param.nv`

```nova
fn main(x: int, x: int) : void {
    return;
}
```

### `tests/tools/checker/negative/duplicate_param.err`

```text
CheckerError: duplicate parameter 'x' in function 'main'
```

---

### 22.10 `tests/tools/checker/negative/void_param.nv`

```nova
fn main(x: void) : void {
    return;
}
```

### `tests/tools/checker/negative/void_param.err`

```text
CheckerError: parameter 'x' in function 'main' cannot have void type
```

---

### 22.11 `tests/tools/checker/negative/unknown_let_type.nv`

```nova
fn main() : void {
    let x : Missing = 1;
    return;
}
```

### `tests/tools/checker/negative/unknown_let_type.err`

```text
CheckerError: unknown type 'Missing'
```

---

### 22.12 `tests/tools/checker/negative/void_let_type.nv`

```nova
fn main() : void {
    let x : void = 1;
    return;
}
```

### `tests/tools/checker/negative/void_let_type.err`

```text
CheckerError: variable 'x' cannot have void type
```

---

### 22.13 `tests/tools/checker/negative/duplicate_let_same_block.nv`

```nova
fn main() : void {
    let x : int = 1;
    let x : int = 2;
    return;
}
```

### `tests/tools/checker/negative/duplicate_let_same_block.err`

```text
CheckerError: duplicate local variable 'x'
```

---

## 23. CMake hook

如果已有 tool test helper，可以新增：

```cmake
function(add_nova_checker_positive_test name)
    add_test(
        NAME nova_tool_checker_positive_${name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/nova_check.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/checker/positive/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/checker/positive/${name}.out
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/checker_positive_${name}
            -DCC=cc
            -P ${NOVA_TOOL_TEST_DRIVER}
    )
    set_tests_properties(nova_tool_checker_positive_${name} PROPERTIES LABELS "nova_tool;checker;positive")
endfunction()
```

如果你的 `RunNovaToolTest.cmake` 只支持 stdout/output file success，可以先只接 positive。  
negative 可以后面单独扩 driver，或者沿用已有 text compare test driver。

---

## 24. 推荐实现顺序

### Step A：复制 parser tool 为 nova_check

从 `nova_parser.nv` 复制：

- TokenKind
- Token
- tokenize
- ParseNodeKind
- ParseNode
- ParseResult
- parse_program_tree

先让 `nova_check.nv` 输出 parse trace，确认基础无误。

### Step B：新增 SymbolTables 和 vec_str helper

实现：

- `SymbolTables`
- `make_symbol_tables`
- `vec_str_contains`
- `is_keyword_name`
- `is_builtin_type`

### Step C：collect_top_level_symbols

先收集 struct / enum / function，并输出 counts。

让 positive minimal test 过。

### Step D：validate declarations

实现：

- struct field checks
- enum member checks
- function signature checks

### Step E：main checks

实现：

- missing main
- invalid main return
- main params must be zero

### Step F：block shallow checks

实现：

- let declared type exists
- let not void
- duplicate let in same block
- nested block shadowing allowed

### Step G：tests

先 positive，再 negative。

---

## 25. 本阶段最容易犯的错误

### 错误 1：想做完整表达式检查

不要在 Step 14 做。`let x : int = "a"` 暂时不检查，因为 expression 还没 parse 成 tree。

### 错误 2：ParseNode 没有 location，错误位置不精确

这是可以接受的。Step 14 初版 diagnostics 可以不带 line/column。

如果你想改进，需要让 parser 在 ParseNode 上保存 line/column，这可以作为加分项。

### 错误 3：同名规则不统一

建议 Step 14 先采用简单规则：

```text
top-level struct / enum / function 共享名字空间
```

虽然真实语言可以更复杂，但 checker 简单很多。

### 错误 4：vec<Self> 被误判为 direct self field

只禁止：

```text
field.value == struct_name
```

不要禁止：

```text
field.value == vec<struct_name>
```

### 错误 5：nested block shadowing 误报

同一个 block 不能重复 let，但 nested block 应允许 shadow。

---

## 26. 验收标准

### 合格

- 新增 `nova_check.nv`
- 能 parse source 并输出 `Check OK`
- 能收集 struct / enum / function counts
- 能检测 duplicate top-level name

### 良好

- 检查 struct fields / enum members / function signature
- 检查 main signature
- 检查 unknown declared type
- 检查 void field / param / let
- 检查 duplicate let in same block

### 优秀

- 能跑现有 positive examples
- negative tests diagnostics 稳定
- checker code 结构清楚
- 为 Step 15 expression parser 留好接口

---

## 27. 完成标志

Step 14 完成的标志是：

> Nova 可以基于自己生成的 ParseNode tree 做声明级语义检查。

也就是说：

```text
Nova source
  -> Nova tokenizer
  -> Nova parser
  -> Nova checker
  -> Check OK / CheckerError
```

这一步完成后，Nova frontend 已经具备：

- lexical analysis
- parse tree construction
- declaration-level semantic validation

后续 Step 15 才进入 expression parser。

---

## 28. 后续路线

```text
Step 15  Expression parser in Nova
         把 skip_expr_until_semic 替换成 expression ParseNode

Step 16  Expression type checker in Nova
         基于 expression tree 做 let/return/if/while/call/field/struct/vec/enum 检查

Step 17  Frontend integration + diagnostics framework
         modes, stable diagnostics, C++ frontend success/failure comparison

Step 18  import/module or include system
         拆 lib/tokenizer.nv / parser.nv / checker.nv

Step 19  Nova codegen prototype
         ParseNode -> tiny C subset, generated executable tests

Step 20  Bootstrap milestone
         Nova tools process Nova tools, final demo and report
