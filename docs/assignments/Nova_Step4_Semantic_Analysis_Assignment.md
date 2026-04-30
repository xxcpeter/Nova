# Nova Project — Step 4
## 题目：实现 Nova v0 的 Semantic Analysis（名字解析与类型检查）

---

## 1. 本次作业目标

在 Step 3 中，你已经完成了 parser，能够把 Nova 源码解析成 AST。
Step 4 的目标是：

> **在 AST 上进行语义分析（semantic analysis），把“语法上合法”的程序进一步筛成“语义上合法”的程序。**

这一阶段的重点是：

- 顶层函数收集与重复定义检查
- 作用域管理与名字解析（name resolution）
- 表达式类型推导
- 赋值、调用、返回语句、条件表达式的类型检查
- 对非法程序给出准确、稳定的语义错误

本阶段**不做 codegen，不做优化，不做控制流高级分析**。

---

## 2. 本阶段结束时你应该具备什么

你完成 Step 4 后，应当能做到：

- 读入 `.nv` 文件
- lexer + parser 生成 AST
- semantic analyzer 检查程序是否语义合法
- 对合法程序输出 “semantic check passed” 或 AST + checked 信息
- 对非法程序输出稳定、可测试的语义错误

换句话说，这一步会把之前 parser 测不出来的错误都拦住，例如：

- 未定义变量
- 同作用域重名
- 重复函数定义
- 赋值两边类型不一致
- 调用参数个数不匹配
- `if` / `while` 条件不是 `bool`
- `return` 与函数返回类型不匹配
- 非法赋值目标

---

## 3. 本次作业的硬性范围

Step 4 只覆盖 **Nova v0 已冻结的语言子集**，并且只做“静态语义检查”。

---

## 3.1 顶层函数

你需要支持并检查：

- 顶层函数名字唯一
- 参数名在同一个函数参数列表内不能重复
- 函数可前向调用
- 调用时参数个数必须匹配
- 调用时参数类型必须匹配

本阶段不要求：

- 重载
- 默认参数
- 可变参数
- 泛型函数

---

## 3.2 变量与作用域

你需要支持并检查：

- block 形成词法作用域
- 内层可遮蔽外层同名变量
- 同一作用域内不能重复定义同名变量
- 变量使用前必须已定义
- 赋值目标必须是当前 Nova v0 合法左值

本阶段建议只允许：

- `IdentifierExpr` 作为合法赋值目标

虽然你的 AST 已经为将来复杂左值预留了 `Expr`，但当前 Nova v0 的语义子集可以仍然只接受简单变量赋值。

---

## 3.3 类型检查

你需要检查：

- `let` 初始化表达式类型必须与声明类型一致
- 赋值两侧类型必须一致
- `if` / `while` 条件必须是 `bool`
- `return;` 只允许出现在 `void` 函数中
- `return expr;` 只允许出现在非 `void` 函数中，且表达式类型必须匹配函数返回类型
- `==` / `!=` 两侧类型必须相同
- `< <= > >=` 只允许用于 `int`
- `&& || !` 只允许用于 `bool`
- unary `-` 只允许用于 `int`
- `+ - * / %` 只允许用于 `int`
- 函数调用的实参与形参类型必须一一匹配

Nova v0 **不做隐式类型转换**。

---

## 3.4 控制流与返回

本阶段建议实现：

- 非 `void` 函数的所有控制路径必须返回一个值

这会比纯类型检查再多一点控制流分析，但在 Nova v0 这个规模下是值得做的。

你至少需要正确处理这些模式：

- block 末尾有 `return`
- `if/else` 的两边都返回
- `while` 不保证返回

例如：

```nova
fn f(x: int) : int {
    if (x > 0) {
        return 1;
    } else {
        return 2;
    }
}
```

应当通过。

而：

```nova
fn f(x: int) : int {
    if (x > 0) {
        return 1;
    }
}
```

应当报错：并非所有路径都返回。

---

## 4. 本阶段不做什么

本阶段明确不做：

- code generation
- constant folding
- dead code elimination
- dataflow analysis
- borrow checking
- definite assignment analysis
- struct / field access
- arrays / indexing
- operator overloading
- implicit conversion
- advanced control-flow graph construction

你需要记住：

> **Step 4 的目标是“语义是否合法”，不是“程序如何运行得更好”。**

---

## 5. 建议的文件结构

建议新增：

```text
include/
  sema.h

src/
  sema.cpp
  nova_sema.cpp   // 可选：单独的语义检查驱动
```

如果你想保持工具数量更少，也可以把 parser 和 sema 接到同一个 `nova_parse` / `nova_check` 驱动里，但建议单独做一个 `nova_sema`，更清楚。

---

## 6. 建议的模块职责

---

## 6.1 `sema.h`

这个文件负责定义语义分析器接口，以及你在语义分析阶段需要的环境数据结构。

建议至少定义：

- `class SemanticError`
- `class SemanticAnalyzer`

以及必要的辅助结构，例如：

- `struct FunctionSignature`
- `struct Scope`

### `FunctionSignature`

建议至少包含：

- `std::string name`
- `std::vector<TypeKind> param_types`
- `TypeKind return_type`
- `SourceLocation location`

### `Scope`

建议至少包含一个变量表：

- `std::unordered_map<std::string, TypeKind> variables`

如果你愿意，也可以让 `SemanticAnalyzer` 直接维护一个 `std::vector<Scope>` 作为作用域栈。

---

## 6.2 `sema.cpp`

这个文件负责实现：

- 顶层函数签名收集
- 语句检查
- 表达式类型推导
- 作用域 push / pop
- 错误报告

---

## 6.3 `nova_sema.cpp`

这是一个很薄的驱动，职责是：

- 读文件
- lexer -> parser -> sema
- 成功则打印通过信息
- 失败则打印 `LexError` / `ParseError` / `SemanticError`

不要把具体语义检查逻辑写在这里。

---

## 7. 建议的 `SemanticAnalyzer` 接口

### 公有接口

建议至少提供：

- `SemanticAnalyzer(std::string_view source_name);`
- `void analyze(const Program& program);`

其中 `analyze()` 应完成整个语义检查流程。

---

## 7.1 私有辅助函数建议

### 顶层函数相关

- `void collect_function_signatures(const Program& program);`
- `const FunctionSignature& lookup_function(std::string_view name, SourceLocation loc) const;`

### 作用域相关

- `void push_scope();`
- `void pop_scope();`
- `void declare_variable(std::string_view name, TypeKind type, SourceLocation loc);`
- `TypeKind lookup_variable(std::string_view name, SourceLocation loc) const;`
- `bool is_declared_in_current_scope(std::string_view name) const;`

### 语句检查

- `void check_function(const FunctionDecl& fn);`
- `void check_block(const BlockStmt& block);`
- `void check_stmt(const Stmt& stmt);`
- `void check_let_stmt(const LetStmt& stmt);`
- `void check_if_stmt(const IfStmt& stmt);`
- `void check_while_stmt(const WhileStmt& stmt);`
- `void check_return_stmt(const ReturnStmt& stmt);`
- `void check_expr_stmt(const ExprStmt& stmt);`

### 表达式检查 / 类型推导

- `TypeKind infer_expr_type(const Expr& expr);`
- `TypeKind infer_assign_expr(const AssignExpr& expr);`
- `TypeKind infer_binary_expr(const BinaryExpr& expr);`
- `TypeKind infer_unary_expr(const UnaryExpr& expr);`
- `TypeKind infer_call_expr(const CallExpr& expr);`
- `TypeKind infer_identifier_expr(const IdentifierExpr& expr);`
- `TypeKind infer_int_literal_expr(const IntLiteralExpr& expr);`
- `TypeKind infer_string_literal_expr(const StringLiteralExpr& expr);`
- `TypeKind infer_bool_literal_expr(const BoolLiteralExpr& expr);`

### 返回路径分析（建议）

- `bool stmt_guarantees_return(const Stmt& stmt) const;`
- `bool block_guarantees_return(const BlockStmt& block) const;`

---

## 8. 推荐的实现流程

---

## 第一轮：先实现函数签名收集

在 `analyze()` 开始时，先扫一遍 `Program.functions`，建立函数表。

你需要检查：

- 函数名重复定义
- 参数列表中是否有重复参数名

这一步的目的，是后面任何函数体里的调用都能查到签名。

---

## 第二轮：实现作用域栈

推荐用：

- `std::vector<std::unordered_map<std::string, TypeKind>> scopes_`

进入 block：
- `push_scope()`

离开 block：
- `pop_scope()`

变量声明时：
- 只检查当前 scope 是否重名

变量查找时：
- 从内层 scope 向外层 scope 查找

---

## 第三轮：先做表达式类型推导

建议优先把这些节点打通：

- `IdentifierExpr`
- `IntLiteralExpr`
- `StringLiteralExpr`
- `BoolLiteralExpr`
- `UnaryExpr`
- `BinaryExpr`
- `CallExpr`
- `AssignExpr`

### 关键约定

`infer_expr_type(expr)` 应返回这个表达式的类型；如果表达式非法，应抛出 `SemanticError`。

---

## 第四轮：再做 statement 检查

顺序建议：

1. `LetStmt`
2. `ExprStmt`
3. `ReturnStmt`
4. `IfStmt`
5. `WhileStmt`
6. `BlockStmt`

### `LetStmt`

检查：

- 名字在当前作用域不重复
- initializer 类型与声明类型一致
- 然后把变量加入当前作用域

### `ExprStmt`

只需要调用 `infer_expr_type()`，忽略返回类型。

### `ReturnStmt`

检查：

- 当前函数返回类型
- `return;` / `return expr;` 形式是否匹配
- `expr` 类型是否匹配

### `IfStmt` / `WhileStmt`

检查：

- 条件类型必须是 `bool`
- 分支 / body 内部语句递归检查

### `BlockStmt`

检查：

- push scope
- 逐条检查 statements
- pop scope

---

## 第五轮：实现“所有路径都返回”检查（建议）

你可以在 `check_function()` 末尾做：

- 如果函数返回类型不是 `void`
- 且 `body` 不保证返回
- 抛 `SemanticError`

### 最小可用规则

- `ReturnStmt` 保证返回
- `BlockStmt` 由最后一个会保证返回的 statement 决定
- `IfStmt` 只有 `then` 和 `else` 都保证返回时才保证返回
- `WhileStmt` 默认不保证返回
- 其他语句默认不保证返回

这套规则很适合 Nova v0，已经足够做课程项目。

---

## 9. 错误处理要求

你应定义 `SemanticError`，格式建议统一为：

```text
SemanticError at 3:8: undefined variable 'x'
SemanticError at 5:12: type mismatch in assignment: expected int, got bool
SemanticError at 7:5: function 'foo' expects 2 arguments, got 1
SemanticError at 10:1: non-void function 'f' does not return on all paths
```

要求至少包含：

- 错误类别
- 行号
- 列号
- 人类可读消息

---

## 10. 测试建议

建议新增：

```text
tests/
└── sema/
    ├── positive/
    └── negative/
```

---

## 10.1 正例建议覆盖

至少包括：

1. 最小 `void` 函数
2. 合法 `let` + 赋值
3. 合法 `if/else`
4. 合法 `while`
5. 合法递归函数调用
6. 前向调用
7. block 内遮蔽
8. 非 `void` 函数所有路径都返回

---

## 10.2 负例建议覆盖

至少包括：

1. 未定义变量读取
2. 未定义变量赋值
3. 同作用域变量重名
4. 函数重复定义
5. 参数个数不匹配
6. 参数类型不匹配
7. `let` 初始化类型不匹配
8. 赋值类型不匹配
9. `if` 条件不是 `bool`
10. `while` 条件不是 `bool`
11. `return;` 出现在非 `void` 函数
12. `return expr;` 出现在 `void` 函数
13. `return` 类型与函数返回类型不匹配
14. 非 `void` 函数并非所有路径返回
15. 非法赋值目标（如 `(a + b) = c`）

---

## 11. 交付物

本次作业提交物应至少包括：

### 代码
- `include/sema.h`
- `src/sema.cpp`
- `src/nova_check.cpp`

### 测试
- `tests/sema/positive/*.nv`
- `tests/sema/negative/*.nv`
- 对应 `.out` / `.err`

### 文档
- 简短说明你的作用域实现方式
- 简短说明你的返回路径检查规则

---

## 12. 本阶段验收标准

### 合格
- 能发现未定义变量、重复变量、重复函数
- 能发现基本类型不匹配
- 能发现基本函数调用错误
- 能报告稳定的 `SemanticError`

### 良好
- 作用域处理正确
- 条件、返回、赋值检查完整
- 前向调用可用
- 测试覆盖较全

### 优秀
- 所有路径返回检查实现合理
- 错误信息清楚
- 模块边界清晰
- 代码结构便于后续 codegen

---

## 13. 本阶段最容易犯的错误

### 错误 1：把函数签名收集和函数体检查混在一起
建议先建函数表，再检查函数体。

### 错误 2：作用域 push/pop 不严格
block 进入和离开必须成对，任何遗漏都会造成变量可见性错误。

### 错误 3：`AssignExpr.target` 既然是 `Expr`，就误以为所有 expr 都可赋值
当前 Nova v0 仍然应限制为简单变量赋值。

### 错误 4：错误消息太泛
比如一律报 “type mismatch”，会让调试很难。尽量指出：
- 在什么上下文
- 期望什么类型
- 实际得到什么类型

### 错误 5：返回路径分析做太复杂
Step 4 只需要做一个小而稳定的版本，不必构建完整 CFG。

---

## 14. 完成标志

Step 4 完成的标准是：

> **语法上合法的 Nova 程序，经过 sema 之后，要么被接受，要么得到稳定、可解释的语义错误。**

也就是说：

- parser 已经解决“长得像不像程序”
- sema 要解决“这程序在 Nova v0 里是否合法”

---

## 15. 下一步预告

完成 Step 4 后，你就具备进入 Step 5（lowering / codegen to C）的条件了。因为那时你已经有：

- 稳定的 AST
- 稳定的类型系统
- 稳定的语义保证

这会让后续 codegen 简单很多。
