# Nova Project — Step 2
## 题目：实现 Nova 的词法分析器（Lexer）

## 1. 本次作业目标

实现一个独立、稳定、可测试的 Nova lexer，将 `.nv` 源码文本转换成 token 序列，为后续 parser 提供输入。

本阶段结束时，你应当拥有：

- 一个可运行的 lexer
- 明确的 token 类型定义
- 能输出 token dump 的测试工具
- 一组覆盖正负例的 lexer 测试
- 清晰的错误报告格式

本阶段**不做 AST**，也**不做语义分析**。只负责把字符流切成 token。

---

## 2. Step 2 的范围

Nova lexer 必须支持 Step 1 冻结下来的全部词法元素。

### 2.1 关键字

必须识别以下关键字：

- `fn`
- `let`
- `if`
- `else`
- `while`
- `return`
- `true`
- `false`
- `int`
- `bool`
- `void`
- `str`

### 2.2 标识符

规则建议：

- 首字符：字母或下划线
- 后续字符：字母、数字、下划线

示例：

- `x`
- `_tmp`
- `repeat_str`

非法示例：

- `1abc`

### 2.3 字面量

必须支持：

- 整数字面量：`0`, `1`, `42`
- 字符串字面量：`"hello"`
- 布尔字面量：`true`, `false`（作为关键字处理）

### 2.4 运算符

必须识别：

- `=`
- `+` `-` `*` `/` `%`
- `==` `!=` `<` `<=` `>` `>=`
- `&&` `||` `!`

### 2.5 标点符号

必须识别：

- `(` `)`
- `{` `}`
- `,`
- `;`
- `:`

### 2.6 注释

必须支持：

- 行注释：`// ...`
- 块注释：`/* ... */`

规则：

- 块注释不嵌套
- 注释内容不产出 token
- 注释内部允许出现看起来像 token 的字符，但应整体跳过

### 2.7 空白字符

空白字符应被忽略，但你仍需正确维护：

- 行号
- 列号

---

## 3. 建议的 Token 设计

你可以自由命名，但建议至少有如下几类：

### 3.1 特殊 token

- `EOF`
- `INVALID`（可选）

### 3.2 标识与字面量

- `IDENT`
- `INT_LITERAL`
- `STRING_LITERAL`

### 3.3 关键字 token

为每个关键字单独建 token 类型，推荐这样做，因为 parser 会更简单。

例如：

- `KW_FN`
- `KW_LET`
- `KW_IF`
- `KW_ELSE`
- `KW_WHILE`
- `KW_RETURN`
- `KW_TRUE`
- `KW_FALSE`
- `KW_INT`
- `KW_BOOL`
- `KW_VOID`
- `KW_STR`

### 3.4 运算符与标点 token

建议也分别建类型，例如：

- `PLUS`, `MINUS`, `STAR`, `SLASH`, `PERCENT`
- `ASSIGN`, `EQ_EQ`, `BANG_EQ`
- `LT`, `LE`, `GT`, `GE`
- `AND_AND`, `OR_OR`, `BANG`
- `LPAREN`, `RPAREN`
- `LBRACE`, `RBRACE`
- `COMMA`, `SEMICOLON`, `COLON`

---

## 4. 你需要实现什么

## 4.1 输入

lexer 的输入应为完整源文件文本。

建议同时保存：

- 原始源码字符串
- 当前下标
- 当前行号
- 当前列号

## 4.2 输出

输出为 token 序列。每个 token 至少应包含：

- token kind
- token text / lexeme
- 起始行号
- 起始列号

可选但推荐：

- 结束行号/列号
- 字面量解析值（如整数值）

## 4.3 行列信息

后面 parser 和 sema 都会依赖位置信息，所以从 lexer 开始就把位置做好。

建议规则：

- 行号从 1 开始
- 列号从 1 开始
- token 的位置取其第一个字符的位置

---

## 5. 推荐的实现策略

建议按下面顺序做。

### Task A：先定义 `TokenKind`

先把 token 枚举和 token 结构体定义好，不要一边扫一边临时发明 token。

### Task B：实现最小字符游标

至少支持：

- `peek()`：看当前字符
- `peek_next()`：看下一个字符
- `advance()`：吃掉当前字符并更新位置
- `match(expected)`：条件性消费一个字符

### Task C：实现跳过空白与注释

优先把：

- 空格
- tab
- 换行
- `// ...`
- `/* ... */`

统一收敛到一个 `skip_whitespace_and_comments()` 里。

### Task D：实现单 token 扫描

写一个类似 `next_token()` 的函数，每次返回一个 token。

建议优先顺序：

1. EOF
2. identifier / keyword
3. integer literal
4. string literal
5. 双字符运算符
6. 单字符运算符与标点
7. 报错

### Task E：实现整文件 token dump

做一个小 driver：

```bash
nova_lex file.nv
```

输出类似：

```text
1:1   KW_FN        "fn"
1:4   IDENT        "main"
1:8   LPAREN       "("
...
```

这会极大地方便你调试。

---

## 6. 字符串字面量要求

这是本阶段最容易反复修改的部分，所以建议先冻结一个很小的范围。

### 最小要求

- 支持双引号字符串：`"hello"`
- 支持空字符串：`""`
- 支持普通 ASCII 文本

### 建议先不做或只做最少支持

- 复杂转义
- Unicode escape
- 多行字符串

你可以选两种策略之一：

### 策略 A：v0 先不支持任何转义
这样最简单。

### 策略 B：只支持少量常见转义
例如：

- `\n`
- `\t`
- `\"`
- `\\`

如果你不想在 lexer 阶段引入太多复杂性，建议先选 **策略 A**，等后面再加。

无论选哪个，都要写进实现说明或注释里。

---

## 7. 错误处理要求

lexer 至少要能检测并报告以下错误：

- 非法字符
- 未闭合字符串
- 未闭合块注释

推荐错误格式：

- 错误类别：Lexical error
- 行号
- 列号
- 简短说明

例如：

- `Lexical error at line 3, column 15: unexpected character '@'`
- `Lexical error at line 8, column 5: unterminated string literal`
- `Lexical error at line 12, column 1: unterminated block comment`

建议：

- lexer 遇到严重错误时可以直接停止
- 这一阶段不必做复杂恢复

---

## 8. 你需要提交什么

### 8.1 代码

建议至少包含：

- `token.h / token.cpp`
- `lexer.h / lexer.cpp`
- 一个简单的 `main.cpp` 或 `nova_lex.cpp`

### 8.2 测试

至少应包含：

#### 正例

- 能正确 dump Step 1 中所有 positive examples 的 token
- 能正确跳过空白与注释
- 能正确识别关键字与普通标识符
- 能正确识别双字符运算符：`== != <= >= && ||`

#### 负例

至少补两类词法负例：

- 未闭合字符串
- 未闭合块注释

推荐再补：

- 非法字符，例如 `@`

### 8.3 文档

简单记录：

- 你支持了哪些字符串规则
- token 设计
- 错误处理策略

---

## 9. 验收标准

如果这是课程作业，我会这样验收。

### 合格

- 能把合法 Nova 程序切成正确 token 序列
- 能区分关键字与标识符
- 能处理注释与空白
- 能报告基本词法错误

### 良好

- token 带稳定位置
- dump 工具清晰可读
- 测试覆盖双字符运算符与边界情况
- 字符串与注释处理稳定

### 优秀

- 代码结构干净
- 接口设计已经方便后续 parser 复用
- 错误信息清楚
- 测试集比较完整

---

## 10. 本阶段最容易踩的坑

### 坑一：先写 parser 风格代码，后补 token
不要让 lexer 和 parser 逻辑混在一起。现在只做 tokenization。

### 坑二：双字符运算符处理顺序错误
例如先吃掉 `=`，就会把 `==` 拆错。

### 坑三：注释和除号冲突
你要清楚地区分：

- `/`
- `//`
- `/* ... */`

### 坑四：行列号更新混乱
如果一开始位置不准，后面所有报错都会痛苦。

### 坑五：字符串规则想得太大
v0 先做最小闭环，不要一开始就做完整字符串系统。

---

## 11. 推荐的开发顺序

1. 定义 token 结构
2. 写字符游标
3. 先支持标点和简单运算符
4. 再支持关键字与标识符
5. 再支持整数
6. 再支持字符串
7. 最后补注释和错误处理
8. 做 token dump 和测试

---

## 12. 本阶段完成标志

Step 2 完成的标准是：

- 你可以对任意 `.nv` 文件输出稳定的 token 序列
- 词法错误有明确位置
- 当前 lexer 已足够作为 Step 3 parser 的输入层
