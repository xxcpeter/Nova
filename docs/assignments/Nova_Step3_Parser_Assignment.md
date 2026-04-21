# Nova Project — Step 3
## 题目：实现 Nova v0 的 Parser 与 AST

---

## 1. 本次作业目标

在 Step 2 中，你已经完成了 lexer，能够把源代码切分成 token 流。
Step 3 的目标是：

> **把 token 流解析成抽象语法树（AST），并正确反映 Nova v0 的语法结构与表达式优先级。**

这一步的重点是：

- 语法结构是否正确
- 表达式优先级是否正确
- AST 设计是否清楚
- 语法错误能否定位

本阶段**不做语义分析**。
也就是说，像下面这些问题先不处理：

- 变量是否已定义
- 函数是否存在
- 参数个数是否匹配
- `if (1)` 是否类型错误

这些属于后面的 semantic analysis。

---

## 2. 本阶段结束时你应该具备什么

你完成 Step 3 后，应当能做到：

- 读取 `.nv` 文件
- 得到 token 流
- 将 token 流解析成 AST
- 把 AST 以稳定格式打印出来
- 对语法错误给出基本诊断信息

你最终应该至少能正确解析这些结构：

- 顶层函数定义
- 参数列表
- 返回类型
- block
- `let`
- 赋值
- `if / else`
- `while`
- `return`
- 表达式语句
- 函数调用
- 字面量
- 一元表达式
- 二元表达式

---

## 3. 本次作业的硬性范围

本阶段 parser 必须支持 Nova v0 里已经冻结的语法子集。

### 3.1 顶层结构

程序由若干个顶层函数组成：

```nova
fn add(a: int, b: int) : int {
    return a + b;
}

fn main() : void {
    return;
}
```

你需要支持：

- 多个函数
- 空参数列表与非空参数列表
- 显式返回类型

### 3.2 语句

必须支持这些 statement：

- `let` 声明
- 赋值语句
- `if / else`
- `while`
- `return`
- 表达式语句
- block

例如：

```nova
let x : int = 1;
x = x + 1;

if (x > 0) {
    return;
} else {
    return;
}

while (x > 0) {
    x = x - 1;
}
```

### 3.3 表达式

必须支持这些表达式类别：

- 整数字面量
- 字符串字面量
- `true / false`
- 标识符
- 函数调用
- 括号表达式
- 一元表达式：`!expr`、`-expr`
- 二元表达式
- 赋值表达式（如果你选择将赋值纳入 expression grammar）

### 3.4 运算符优先级

parser 必须正确实现 Nova v0 的优先级与结合性：

1. unary：`!`, unary `-`  
   - 右结合

2. multiplicative：`* / %`  
   - 左结合

3. additive：`+ -`  
   - 左结合

4. comparison：`< <= > >=`  
   - 不允许链式比较

5. equality：`== !=`  
   - 左结合

6. logical AND：`&&`  
   - 左结合

7. logical OR：`||`  
   - 左结合

8. assignment：`=`  
   - 右结合

注意：
这一步只要求**语法层面**正确建树，不要求做类型合法性检查。

---

## 4. 本阶段不做什么

这一步先明确不做：

- 语义分析
- 类型检查
- 名字解析
- 未定义变量检查
- 函数签名匹配检查
- 控制流完整性检查
- 常量折叠
- 优化
- codegen

你要牢牢记住：

> **Step 3 只负责“源码长得对不对、树长得对不对”。**

---

## 5. 建议的文件结构

如果你现在已经有：

```text
include/
src/
tests/
```

那么建议你新增：

```text
include/
  ast.h
  parser.h

src/
  ast.cpp        // 可选，如果你有打印辅助函数
  parser.cpp
  nova_parse.cpp // 或者先接到现有驱动
```

如果 AST 很简单，`ast.cpp` 可以不需要，全部放在头文件里也行。

---

## 6. 文件级说明

### 6.1 `ast.h`

这个文件负责定义 AST 节点和相关的枚举、别名、打印接口。它不负责解析逻辑。

#### 你应该在这个文件里定义什么

##### 1. 基本类型枚举
建议定义一个 Nova 的类型枚举，例如：

- `enum class TypeKind { Int, Bool, Str, Void };`

这可以避免在 AST 里到处直接存字符串类型名。

##### 2. 运算符枚举
建议定义：

- `enum class UnaryOp`
- `enum class BinaryOp`

例如：
- Unary: `Negate`, `LogicalNot`
- Binary: `Add`, `Sub`, `Mul`, `Div`, `Mod`, `Less`, `LessEqual`, `Greater`, `GreaterEqual`, `EqualEqual`, `BangEqual`, `LogicalAnd`, `LogicalOr`, `Assign`

##### 3. AST 基类
建议至少定义：

- `struct ASTNode`
- `struct Expr : ASTNode`
- `struct Stmt : ASTNode`
- `struct Decl : ASTNode`

`ASTNode` 建议至少包含：

- `SourceLocation location`
- 虚析构函数

##### 4. Program 节点
建议定义：

- `struct Program : ASTNode`

字段建议：
- `std::vector<std::unique_ptr<FunctionDecl>> functions`

##### 5. Declaration 节点
建议至少定义：

- `struct ParamDecl : ASTNode`
- `struct FunctionDecl : Decl`

`ParamDecl` 字段建议：
- `std::string name`
- `TypeKind type`

`FunctionDecl` 字段建议：
- `std::string name`
- `std::vector<ParamDecl> params` 或 `std::vector<std::unique_ptr<ParamDecl>>`
- `TypeKind return_type`
- `std::unique_ptr<BlockStmt> body`

##### 6. Statement 节点
建议定义：

- `struct BlockStmt : Stmt`
- `struct LetStmt : Stmt`
- `struct IfStmt : Stmt`
- `struct WhileStmt : Stmt`
- `struct ReturnStmt : Stmt`
- `struct ExprStmt : Stmt`

字段建议：

`BlockStmt`
- `std::vector<std::unique_ptr<Stmt>> statements`

`LetStmt`
- `std::string name`
- `TypeKind declared_type`
- `std::unique_ptr<Expr> initializer`

`IfStmt`
- `std::unique_ptr<Expr> condition`
- `std::unique_ptr<Stmt> then_branch`
- `std::unique_ptr<Stmt> else_branch`（可为空）

`WhileStmt`
- `std::unique_ptr<Expr> condition`
- `std::unique_ptr<Stmt> body`

`ReturnStmt`
- `std::unique_ptr<Expr> value`（可为空）

`ExprStmt`
- `std::unique_ptr<Expr> expr`

##### 7. Expression 节点
建议定义：

- `struct AssignExpr : Expr`
- `struct BinaryExpr : Expr`
- `struct UnaryExpr : Expr`
- `struct CallExpr : Expr`
- `struct IdentifierExpr : Expr`
- `struct IntLiteralExpr : Expr`
- `struct StringLiteralExpr : Expr`
- `struct BoolLiteralExpr : Expr`
- 可选：`struct GroupingExpr : Expr`

字段建议：

`AssignExpr`
- `std::string target`
- `std::unique_ptr<Expr> value`

`BinaryExpr`
- `BinaryOp op`
- `std::unique_ptr<Expr> left`
- `std::unique_ptr<Expr> right`

`UnaryExpr`
- `UnaryOp op`
- `std::unique_ptr<Expr> operand`

`CallExpr`
- `std::string callee`
- `std::vector<std::unique_ptr<Expr>> arguments`

`IdentifierExpr`
- `std::string name`

`IntLiteralExpr`
- `std::string lexeme` 或 `long long value`

`StringLiteralExpr`
- `std::string lexeme`

`BoolLiteralExpr`
- `bool value`

##### 8. AST 打印接口
建议至少声明：

- `std::ostream& operator<<(std::ostream&, const Program&)`

或者：

- `void dump_ast(std::ostream&, const Program&, int indent = 0);`

如果你喜欢 visitor，也可以做，但 Step 3 不强制。

#### `ast.h` 不应该做什么

- 不负责 token 消费
- 不负责语法错误处理
- 不负责 lexer

---

### 6.2 `parser.h`

这个文件负责定义 parser 的接口、基础游标函数和错误类型。

#### 你应该在这个文件里定义什么

##### 1. `class ParseError`
建议定义成继承 `std::runtime_error` 的异常类。

它至少应支持：
- 错误消息
- 报错位置

推荐格式类似：
- `ParseError at 3:15: expected ';' after expression`

##### 2. `class Parser`

建议字段包括：
- `const std::vector<Token>& tokens_`
- `std::string_view source_name_`
- `size_t current_ = 0`

##### 3. 公有接口
建议至少提供：

- `Parser(const std::vector<Token>& tokens, std::string_view source_name);`
- `std::unique_ptr<Program> parse_program();`

##### 4. 私有辅助函数：游标管理
建议声明：

- `bool is_at_end() const;`
- `const Token& peek() const;`
- `const Token& previous() const;`
- `const Token& advance();`
- `bool check(TokenType type) const;`
- `bool match(TokenType type);`
- `const Token& expect(TokenType type, std::string_view message);`

你也可以再加：

- `bool match_any(std::initializer_list<TokenType> types);`

##### 5. 私有辅助函数：解析顶层结构
建议声明：

- `std::unique_ptr<FunctionDecl> parse_function();`
- `std::vector<ParamDecl> parse_param_list();`
- `TypeKind parse_type();`

##### 6. 私有辅助函数：解析语句
建议声明：

- `std::unique_ptr<Stmt> parse_statement();`
- `std::unique_ptr<BlockStmt> parse_block();`
- `std::unique_ptr<Stmt> parse_let_stmt();`
- `std::unique_ptr<Stmt> parse_if_stmt();`
- `std::unique_ptr<Stmt> parse_while_stmt();`
- `std::unique_ptr<Stmt> parse_return_stmt();`
- `std::unique_ptr<Stmt> parse_expr_stmt();`

##### 7. 私有辅助函数：解析表达式
建议严格按优先级拆成：

- `std::unique_ptr<Expr> parse_expression();`
- `std::unique_ptr<Expr> parse_assignment();`
- `std::unique_ptr<Expr> parse_logical_or();`
- `std::unique_ptr<Expr> parse_logical_and();`
- `std::unique_ptr<Expr> parse_equality();`
- `std::unique_ptr<Expr> parse_comparison();`
- `std::unique_ptr<Expr> parse_additive();`
- `std::unique_ptr<Expr> parse_multiplicative();`
- `std::unique_ptr<Expr> parse_unary();`
- `std::unique_ptr<Expr> parse_primary();`

如果你选择把调用表达式独立成 postfix 层，也可以再加：

- `std::unique_ptr<Expr> parse_postfix();`

#### `parser.h` 不应该做什么

- 不负责读取文件
- 不负责运行 lexer
- 不负责 AST 输出格式实现（可以只声明）

---

### 6.3 `parser.cpp`

这个文件负责实现全部解析逻辑。

#### 你应该在这里实现什么

##### 1. 基础游标函数
这几个函数最好优先写好，因为后面所有 parse 函数都会依赖它们。

###### `is_at_end()`
判断当前 token 是否是 `EndOfFile`。

###### `peek()`
查看当前 token，不前进。

###### `previous()`
返回上一个已消费 token。

###### `advance()`
消费当前 token，并返回被消费的 token。

###### `check(TokenType)`
判断当前 token 是否是某类型，但不消费。

###### `match(TokenType)`
如果当前 token 匹配，则消费并返回 `true`，否则返回 `false`。

###### `expect(TokenType, message)`
如果当前 token 是目标类型，消费并返回；否则直接抛 `ParseError`。

##### 2. `parse_program()`

职责：
- 循环解析顶层函数，直到 `EndOfFile`
- 构造 `Program` 节点

##### 3. `parse_function()`

职责：
- 解析 `fn`
- 解析函数名
- 解析参数列表
- 解析 `:`
- 解析返回类型
- 解析函数体 block

输入大致长这样：

```nova
fn main() : void {
    return;
}
```

##### 4. `parse_param_list()`

职责：
- 解析 `a: int, b: str` 这种列表
- 空参数列表也要支持

##### 5. `parse_type()`

职责：
- 识别 `int / bool / str / void`
- 返回 `TypeKind`

##### 6. `parse_statement()`

职责：
- 根据当前 token 决定进入哪种 statement 解析函数

一般分派逻辑：
- `{` -> `parse_block()`
- `let` -> `parse_let_stmt()`
- `if` -> `parse_if_stmt()`
- `while` -> `parse_while_stmt()`
- `return` -> `parse_return_stmt()`
- 否则 -> `parse_expr_stmt()`

##### 7. `parse_block()`

职责：
- 解析 `{ ... }`
- 收集内部 statement 列表
- 直到遇到 `}`

##### 8. `parse_let_stmt()`

职责：
解析：

```nova
let x : int = expr;
```

应解析：
- 变量名
- `:`
- 类型
- `=`
- 初始化表达式
- `;`

##### 9. `parse_if_stmt()`

职责：
解析：

```nova
if (cond) stmt
if (cond) stmt else stmt
```

##### 10. `parse_while_stmt()`

职责：
解析：

```nova
while (cond) stmt
```

##### 11. `parse_return_stmt()`

职责：
解析：

```nova
return;
return expr;
```

这一步只负责语法，不检查函数返回类型是否匹配。

##### 12. `parse_expr_stmt()`

职责：
解析：

```nova
expr;
```

##### 13. 表达式优先级函数

建议严格按层实现。

###### `parse_expression()`
直接调用 `parse_assignment()`。

###### `parse_assignment()`
建议语法：
- `IDENT '=' assignment`
- 否则回落到 `logical_or`

注意：
这一步只在语法层面构造 `AssignExpr`，不检查左边是不是合法左值以外的复杂情况。

###### `parse_logical_or()`
循环解析 `||`

###### `parse_logical_and()`
循环解析 `&&`

###### `parse_equality()`
循环解析 `==`、`!=`

###### `parse_comparison()`
只允许解析**至多一个**比较运算符，避免链式比较。

###### `parse_additive()`
循环解析 `+`、`-`

###### `parse_multiplicative()`
循环解析 `*`、`/`、`%`

###### `parse_unary()`
处理：
- `!expr`
- `-expr`

###### `parse_primary()`
处理：
- 整数字面量
- 字符串字面量
- `true`
- `false`
- 标识符
- 调用表达式
- 括号表达式

如果当前 token 是 `IDENT`，你需要判断：
- 只是标识符引用
- 还是函数调用 `IDENT(...)`

你可以：
- 在 `parse_primary()` 里直接做
- 或者单独再拆一个 `parse_postfix()`

#### `parser.cpp` 的实现顺序建议

先写：
- 游标函数
- `parse_type()`
- `parse_primary()`
- 表达式优先级链
- `parse_statement()`
- `parse_function()`
- `parse_program()`

---

### 6.4 `nova_parse.cpp`

这个文件是 parser 的命令行驱动程序。

#### 它应该完成什么

至少做这些事：

1. 读取命令行参数
2. 读文件
3. 调 lexer 得到 token 列表
4. 调 parser 得到 AST
5. 打印 AST
6. 捕获并打印 `LexError` / `ParseError`

#### 你应实现哪些函数

至少需要：

- `std::string read_file(const std::string& path);`
- `int main(int argc, char* argv[]);`

#### `main()` 的逻辑建议

1. 检查参数个数
2. 读取源文件
3. `Lexer lexer(source, filename);`
4. `auto tokens = lexer.tokenize();`
5. `Parser parser(tokens, filename);`
6. `auto program = parser.parse_program();`
7. 打印 AST
8. 如果捕获到异常，打印错误并返回非零退出码

#### 这个文件不应该做什么

- 不实现 parser 细节
- 不实现 AST 节点
- 不实现 token 扫描逻辑

---

## 7. AST 打印建议

Step 3 一定要有稳定的 AST dump。

推荐最简方案：
- 在 `ast.h` / `ast.cpp` 里提供一个 `dump_ast` 函数
- 使用缩进树形输出

例如：

```text
Program
  FunctionDecl main : void
    BlockStmt
      ReturnStmt
```

如果函数里有表达式，也继续打印子节点。

例如：

```text
Program
  FunctionDecl add : int
    Param a : int
    Param b : int
    BlockStmt
      ReturnStmt
        BinaryExpr Add
          IdentifierExpr a
          IdentifierExpr b
```

这类输出最适合做 golden test。

---

## 8. 建议测试集

建议新增目录：

```text
tests/
└── parser/
    ├── positive/
    └── negative/
```

### 正例建议至少包括

- `minimal_function.nv`
- `let_statement.nv`
- `expr_precedence.nv`
- `function_call.nv`
- `if_else.nv`
- `while_loop.nv`
- `nested_blocks.nv`
- `recursion.nv`
- `assignment.nv`
- `forward_call.nv`

### 负例建议至少包括

- `missing_semicolon.nv`
- `missing_rparen.nv`
- `missing_rbrace.nv`
- `missing_colon_before_return_type.nv`
- `missing_type_in_let.nv`
- `invalid_return_expr.nv`
- `incomplete_binary_expr.nv`
- `bad_call_args.nv`

---

## 9. 验收标准

### 合格
- 能正确 parse 最小程序
- 能解析表达式优先级
- 能输出 AST
- 能报告基本语法错误

### 良好
- AST 结构清晰
- `if / else / while / let / call` 都支持
- 错误信息比较准确
- parser 模块边界清楚

### 优秀
- AST dump 稳定
- 测试覆盖较全
- 代码风格统一
- `expect / match / check` 等辅助函数设计良好

---

## 10. 本阶段最容易犯的错误

### 错误 1：把 parser 和 sema 混在一起
这一步先别检查变量定义与类型匹配。

### 错误 2：AST 设计过度复杂
先做出够用、清晰、可扩展的树。

### 错误 3：表达式优先级写乱
一定按层拆函数。

### 错误 4：没有稳定 AST 打印
没有 AST dump，很难验证 parser 是否正确。

### 错误 5：错误恢复做太多
Step 3 不要求 sophisticated recovery，先准确 fail 即可。

---

## 11. 完成标志

Step 3 完成的标准不是“parser 大概能跑”，而是：

> **你可以拿一批正例程序生成 AST dump，拿一批负例程序生成 parse error，并且这些结果稳定可比较。**

也就是说：
- parser 可用
- AST 清楚
- 测试可回归
