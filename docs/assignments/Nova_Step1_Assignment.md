# Nova Project — Step 1
## 题目：定义 Nova 的最小可自举子集

## 1. 本次作业目标

本阶段不写编译器代码，唯一目标是**冻结 Nova v0 的语言边界**，使后续的 lexer、parser、sema、codegen 都可以基于一份明确、稳定、可执行的规格推进。

Step 1 完成后，你应当拥有：

- 一份清晰的 `spec.md`
- 一组覆盖核心语法与语义边界的样例程序
- 一套明确的非目标清单
- 一份足够让别人直接开始实现 lexer/parser 的语言说明

Nova v0 的设计目标不是通用语言，而是服务于一个**编译器 bootstrap project** 的最小可行语言子集。

---

## 2. 设计原则

### 原则 A：够小
Nova 不是“未来可能很强大的语言”，而是“当前能支撑 bootstrap 实验的最小语言”。

### 原则 B：够规整
语法尽量保持 C-like imperative style，避免在 parser 阶段浪费精力在花哨语法上。

### 原则 C：够表达编译器本身
未来需要用 Nova 重写编译器的一部分，因此语言至少要能表达：

- 字符串处理
- 控制流
- 函数调用
- 局部变量
- runtime 调用
- 文件读写（通过 runtime）

---

## 3. Nova v0 的冻结范围

### 3.1 内建类型

Nova v0 只支持四种内建类型：

- `int`
- `bool`
- `str`
- `void`

暂不支持：

- `float`
- `char`
- 内建数组
- `struct`
- `enum`
- 指针
- 泛型

### 3.2 程序结构

程序由若干个顶层函数组成。

示例：

```nova
fn helper(x: int) : int {
    return x + 1;
}

fn main() : void {
    return;
}
```

暂不支持：

- 全局变量
- 嵌套函数
- 类
- 模块/import system

### 3.3 语句

必须支持：

- 变量声明：`let name : Type = expr;`
- 赋值：`name = expr;`
- `if / else`
- `while`
- `return`
- 表达式语句
- block

暂不支持：

- `for`
- `break`
- `continue`
- `match`
- `defer`
- `+=`, `-=`, `++`, `--`

### 3.4 表达式

必须支持：

- 整数字面量
- 字符串字面量
- 布尔字面量
- 变量引用
- 函数调用
- 括号表达式
- 一元运算
- 二元运算

运算符集合：

- 一元：`!`, unary `-`
- 算术：`+ - * / %`
- 比较：`== != < <= > >=`
- 逻辑：`&& ||`
- 赋值：`=`

### 3.5 作用域与名称规则

- `{}` 引入一个新的词法作用域
- 允许内层变量遮蔽外层变量
- 同一作用域内不允许重名
- 函数参数属于函数体作用域
- 顶层函数名全局唯一
- 允许前向调用：函数可以在定义前被调用，只要最终在同一程序中定义

### 3.6 类型规则

- `if` 和 `while` 条件必须是 `bool`
- 赋值左右两边类型必须完全一致
- 函数调用参数个数和类型必须完全匹配
- `==` / `!=` 只允许比较相同类型
- `< <= > >=` 只允许用于 `int`
- `&& || !` 只允许用于 `bool`
- unary `-` 只允许用于 `int`
- `+ - * / %` 只允许用于 `int`
- 不做隐式类型转换
- 不允许链式比较

### 3.7 返回规则

- `void` 函数只允许 `return;`
- 非 `void` 函数只允许 `return expr;`
- 非 `void` 函数要求所有控制路径都返回
- `main` 的签名固定为：

```nova
fn main() : void {
    return;
}
```

---

## 4. 词法与语法层要求

### 4.1 词法元素

需要在 `spec.md` 中定义：

- 标识符规则
- 整数字面量规则
- 字符串字面量规则
- 注释形式
- 空白字符处理方式
- 关键字列表
- 运算符和标点列表

### 4.2 文法

必须写出一份足够具体的 grammar 草案，至少覆盖：

- `program`
- `function`
- `params`
- `type`
- `block`
- `statement`
- `expression`

### 4.3 运算符优先级与结合性

应明确列出：

1. unary `!`, unary `-` — 右结合
2. `* / %` — 左结合
3. `+ -` — 左结合
4. `< <= > >=` — 非结合，不允许链式比较
5. `== !=` — 左结合
6. `&&` — 左结合
7. `||` — 左结合
8. `=` — 右结合

---

## 5. Runtime 边界

Nova v0 允许依赖一个外部 runtime。语言前端只检查这些函数的签名；其具体实现由 C runtime 提供。

推荐冻结如下 API：

```nova
fn print_int(x: int) : void
fn print_str(s: str) : void
fn str_eq(a: str, b: str) : bool
fn str_concat(a: str, b: str) : str
fn read_file(path: str) : str
fn write_file(path: str, content: str) : void
```

这一步的重点是把语言和 runtime 的职责分开：

- 语言负责语法、类型、控制流和函数调用
- runtime 负责输出、字符串实现和文件 IO

---

## 6. 你需要提交什么

### 6.1 `docs/spec.md`

至少应包含以下内容：

1. Language Goal
2. Lexical Rules
3. Grammar
4. Operator Precedence and Associativity
5. Types and Static Semantics
6. Runtime Interface
7. Error Model
8. Non-goals
9. Design Decisions Summary

### 6.2 `examples/`

至少提交一组正例和负例，覆盖以下内容：

#### 正例建议覆盖：

- 表达式优先级
- 函数调用
- 前向调用
- 递归
- 局部变量与遮蔽
- `if / else`
- `while`
- runtime 函数调用

#### 负例建议覆盖：

- 条件表达式不是 `bool`
- 参数个数不匹配
- 返回类型不匹配
- 未定义变量读取
- 未定义变量赋值
- 同作用域重名

---

## 7. 验收标准

Step 1 完成的标准不是“文档写得漂亮”，而是：

- 另一个人读完 `spec.md` 后，可以直接开始写 Nova 的 lexer 和 parser
- examples 中不再包含任何“spec 里未定义”的语法
- examples 已覆盖 Nova v0 的核心语言规则
- 语言本体和 runtime 边界清楚

---

## 8. 已通过的 Step 1 最终检查要点

在当前 Nova v0 冻结版中，以下内容已确定：

- 返回类型语法统一使用 `:`
- 字符串类型名使用 `str`
- 允许顶层函数前向调用
- runtime 通过普通函数调用暴露
- 变量声明后可通过 `=` 重新赋值
- 作用域由 `{}` 定义
- 允许跨作用域遮蔽，不允许同作用域重名
- 比较链式写法被禁止
- `void` / 非 `void` return 规则严格对应
- 非 `void` 函数所有路径必须返回

---

## 9. 本阶段最容易犯的错误

- 过早加入 `match`、`for`、数组、struct 等新特性
- runtime 边界不清，导致后续前端和 runtime 职责混乱
- 文法写得过于模糊，导致 parser 阶段重新发明语言
- examples 使用尚未定义的语法，例如复合赋值或隐式重载

---

## 10. Step 1 结论

Step 1 的目标是冻结 Nova v0 的最小可自举子集。当前版本已经具备进入 Step 2（lexer）的条件。
