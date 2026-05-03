# Nova Project — Step 7
## 题目：用 Nova 实现第一个编译器组件原型：Lexer / Token Dump Tool

---

## 1. 本次作业目标

在 Step 6 中，Nova 已经具备了更实用的 runtime / core 能力：

- 字符串长度与索引
- 字符串切片
- 字符串拼接与 buffer builder
- 文件读写
- string vector
- 命令式控制流与函数调用

Step 7 的目标是：

> **开始用 Nova 编写 Nova 工具本身。**

本阶段不要立刻重写整个编译器。你要先实现一个较小但真实的组件：

> **一个用 Nova 写的 lexer/token dump 工具。**

它读取一个 Nova 源文件，扫描字符，输出 token dump。

这一步的意义是验证：

- Nova 是否已经能处理真实文本输入
- runtime API 是否足够写非玩具程序
- C codegen 是否能支撑较复杂 Nova 程序
- 后续 self-hosting 的路线是否可行

---

## 2. 本阶段结束时你应该具备什么

完成 Step 7 后，你应该拥有：

- 一个用 Nova 写的 lexer 工具
- 能读取 `.nv` 源文件
- 能输出 token dump 到文件或 stdout
- 能处理核心 Nova token
- 能用现有 `nova_compile` 编译它
- 能通过端到端测试验证它的输出

完整链路应当是：

```text
nova_lexer.nv
  -> nova_compile
  -> generated C
  -> native executable
  -> run on input .nv
  -> token dump output
```

这一步是 Nova 自举路线中的第一个关键里程碑：

> **Nova 第一次开始实现自己的编译器组件。**

---

## 3. 本阶段核心任务

你要实现一个 Nova 程序，例如：

```text
tools/nova_lexer.nv
```

它执行：

```text
read source file
scan characters
emit token dump
write output file
```

建议命令形式：

```bash
nova_lexer <input.nv> <output.tok>
```

不过 Nova 当前 `main()` 没有参数，所以你需要先补一个小 runtime 能力：命令行参数访问。

---

## 4. 新增 runtime API：命令行参数

为了让 Nova 程序能写工具，必须能读取命令行参数。

建议新增 runtime API：

```nova
arg_count() : int
arg_get(index: int) : str
```

语义：

- `arg_count()` 返回 C `argc`
- `arg_get(0)` 返回程序名
- `arg_get(1)` 返回第一个用户参数
- 越界时 runtime error

---

## 5. Codegen 对 main 的调整

当前 C codegen 很可能生成：

```c
int main(void) {
    ...
}
```

为了支持 `arg_count()` / `arg_get()`，建议改成：

```c
int main(int argc, char** argv) {
    nova_runtime_init(argc, argv);
    ...
}
```

runtime 中新增：

```c
void nova_runtime_init(int argc, char** argv);
```

runtime 内部保存：

```c
static int rt_argc;
static char** rt_argv;
```

然后：

```c
int arg_count(void);
const char* arg_get(int index);
```

Nova 侧注册为：

```nova
arg_count() : int
arg_get(index: int) : str
```

注意：

- `nova_runtime_init(argc, argv)` 只需要在生成的 C `main` 开头调用。
- 普通 Nova 函数不需要处理 argc/argv。
- Nova 语言层仍然保持 `fn main() : void`，不改变语言语法。

---

## 6. 本阶段不做什么

本阶段明确不做：

- 用 Nova 重写 parser
- 用 Nova 重写 semantic analyzer
- 用 Nova 重写 codegen
- 生成 C++ lexer 的完全兼容 token 格式
- 实现完整错误恢复
- 实现 token struct / enum 语言特性
- 新增 Nova struct / array / module 语法

这一步只做：

> **Nova 写的 lexer prototype。**

---

## 7. 建议文件结构

新增：

```text
tools/
  nova_lexer.nv

tests/nova_tools/
  lexer/
    inputs/
    expected/
```

或者如果你想继续沿用 codegen 测试目录，也可以：

```text
tests/codegen/positive/
  nova_lexer_smoke.nv
```

但更清楚的做法是单独建：

```text
tests/tools/lexer/
  input_basic.nv
  input_basic.tok
  input_strings.nv
  input_strings.tok
```

---

## 8. 推荐 token dump 格式

不要一开始强求完全匹配 C++ `nova_lex` 的输出格式。Nova 版本可以先用更容易生成的格式。

推荐格式：

```text
KIND|lexeme|line|column
```

例如：

```text
KW_FN|fn|1|1
IDENT|main|1|4
LPAREN|(|1|8
RPAREN|)|1|9
COLON|:|1|11
KW_VOID|void|1|13
LBRACE|{|1|18
KW_RETURN|return|2|5
SEMIC|;|2|11
RBRACE|}|3|1
EOF||4|1
```

为什么选择这个格式：

- Nova 里用 `buf_push_str` / `buf_push_int` 很容易生成
- line/column 清楚
- 不需要复杂 quoted escaping
- 测试容易 diff

注意：

- 如果 lexeme 本身是 `|`，比如 `||`，格式里会出现 `OR_OR||||1|1` 这种不太好读的行。
- 为了 Step 7 简化，可以接受这一点。
- 如果你想更稳，可以把格式改成：`KIND line column lexeme`，但有空格的 lexeme 会更麻烦。

---

## 9. Nova lexer v1 必须支持的 token

先覆盖 Nova v0 的核心 token。

### 9.1 特殊 token

- `EOF`
- `INVALID`（可选；也可以直接 runtime error）

### 9.2 标识符和字面量

- `IDENT`
- `INT_LITERAL`
- `STR_LITERAL`

### 9.3 关键字

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
- `str`
- `void`

### 9.4 标点

- `(` `)`
- `{` `}`
- `,`
- `;`
- `:`

### 9.5 运算符

- `=`
- `+` `-` `*` `/` `%`
- `==` `!=`
- `<` `<=` `>` `>=`
- `&&` `||` `!`

---

## 10. 推荐实现方式

由于 Nova v0 没有 struct，也没有 enum，建议用函数 + 局部变量组织代码。

### 10.1 全局状态问题

Nova 当前没有全局变量。为了保持语言简单，你可以把 lexer 写成一组函数，使用参数显式传递状态；但这会比较笨重。

更现实的 Step 7 做法是：

> **先写一个单函数或少量大函数版本。**

也就是在 `main()` 里维护：

```nova
let source : str = read_file(input_path);
let out : int = buf_new();
let pos : int = 0;
let line : int = 1;
let column : int = 1;
```

然后用 `while (pos < str_len(source))` 扫描。

这不是最终架构，但适合验证 Nova 能否写 lexer。

---

## 11. 需要的字符 helper

Nova 没有 `char` 类型，所以字符用 `int` 表示 ASCII 码。

建议你在 Nova 工具里写这些 helper：

```nova
fn is_digit(c: int) : bool
fn is_alpha(c: int) : bool
fn is_alnum(c: int) : bool
fn is_ident_start(c: int) : bool
fn is_ident_continue(c: int) : bool
fn is_whitespace(c: int) : bool
```

ASCII 常量可以直接写数字：

```text
'a' = 97
'z' = 122
'A' = 65
'Z' = 90
'0' = 48
'9' = 57
'_' = 95
' ' = 32
'\n' = 10
'\t' = 9
```

Nova v0 没有 char literal，所以先用数字。

---

## 12. 需要的输出 helper

建议写：

```nova
fn emit_token(out: int, kind: str, lexeme: str, line: int, column: int) : void
```

输出：

```text
kind|lexeme|line|column\n
```

示例实现思路：

```nova
buf_push_str(out, kind);
buf_push_str(out, "|");
buf_push_str(out, lexeme);
buf_push_str(out, "|");
buf_push_int(out, line);
buf_push_str(out, "|");
buf_push_int(out, column);
buf_push_str(out, "\n");
```

这会大量使用 Step 6 的 buffer API。

---

## 13. 字符扫描规则

### 13.1 Whitespace

跳过：

- 空格 `32`
- tab `9`
- newline `10`
- carriage return `13`

newline 时：

```text
line = line + 1
column = 1
```

普通字符：

```text
column = column + 1
```

---

### 13.2 Line comment

遇到 `//`：

- 跳过直到 newline 或 EOF

---

### 13.3 Block comment

遇到 `/*`：

- 跳过直到 `*/`
- 如果 EOF 未闭合，打印 runtime error 或输出 `INVALID`

Step 7 可以先选择简单粗暴：

```nova
print_str("lexer error: unterminated block comment");
return;
```

如果你希望工具退出非零，需要后续 runtime 支持 `exit(code)`。

---

### 13.4 Identifier / keyword

规则：

```text
[A-Za-z_][A-Za-z0-9_]*
```

扫描完成后判断是否关键字。

例如：

```nova
fn keyword_kind(s: str) : str
```

返回：

- `KW_FN`
- `KW_LET`
- ...
- 否则 `IDENT`

---

### 13.5 Integer literal

规则：

```text
[0-9]+
```

如果数字后面紧跟 identifier continuation，可以先输出 `INVALID` 或 runtime error。

例如：

```nova
123abc
```

应当报：

```text
lexer error: invalid number literal
```

---

### 13.6 String literal

必须支持：

- `"hello"`
- `""`
- `"a\\n"`
- `"a\\\""`

扫描逻辑：

- 看到起始 `"`
- 遇到 `\\` 时跳过下一个字符
- 遇到未转义 `"` 结束
- 遇到 newline 或 EOF 报未闭合字符串

lexeme 建议保留原始文本，包括引号。

---

### 13.7 Operators / punctuation

双字符优先：

- `==`
- `!=`
- `<=`
- `>=`
- `&&`
- `||`

然后再处理单字符：

- `+ - * / % = < > ! ( ) { } , ; :`

---

## 14. 命令行行为

工具建议行为：

```bash
nova_lexer input.nv output.tok
```

Nova 程序中：

```nova
fn main() : void {
    if (arg_count() != 3) {
        print_str("Usage: nova_lexer <input.nv> <output.tok>");
        return;
    }

    let input : str = arg_get(1);
    let output : str = arg_get(2);
    let source : str = read_file(input);
    ...
    write_file(output, buf_to_str(out));
    return;
}
```

注意：

- `arg_get(0)` 是 executable 路径
- 用户第一个参数是 `arg_get(1)`

---

## 15. CMake / 测试方式

Step 7 需要一种新的测试：

1. 用 `nova_compile` 编译 `tools/nova_lexer.nv`
2. 用 C 编译器编译生成的 C
3. 运行生成的 `nova_lexer`，传入 input/output 参数
4. 比较 output 文件内容

建议新增：

```text
cmake/RunNovaToolTest.cmake
```

它和 codegen test 类似，但多一步：运行生成的 Nova 工具时传参数。

---

## 16. 推荐测试目录

```text
tests/tools/lexer/
  basic.nv
  basic.tok
  expressions.nv
  expressions.tok
  strings.nv
  strings.tok
  comments.nv
  comments.tok
```

这些 `.nv` 是被 Nova lexer 扫描的输入文件，不是测试程序本身。

---

## 17. 推荐测试样例

### 17.1 `tests/tools/lexer/basic.nv`

```nova
fn main() : void {
    return;
}
```

### `tests/tools/lexer/basic.tok`

```text
KW_FN|fn|1|1
IDENT|main|1|4
LPAREN|(|1|8
RPAREN|)|1|9
COLON|:|1|11
KW_VOID|void|1|13
LBRACE|{|1|18
KW_RETURN|return|2|5
SEMIC|;|2|11
RBRACE|}|3|1
EOF||4|1
```

---

### 17.2 `tests/tools/lexer/expressions.nv`

```nova
fn main() : void {
    let x : int = 1 + 2 * 3;
    x = x - 1;
    return;
}
```

### `tests/tools/lexer/expressions.tok`

```text
KW_FN|fn|1|1
IDENT|main|1|4
LPAREN|(|1|8
RPAREN|)|1|9
COLON|:|1|11
KW_VOID|void|1|13
LBRACE|{|1|18
KW_LET|let|2|5
IDENT|x|2|9
COLON|:|2|11
KW_INT|int|2|13
ASSIGN|=|2|17
INT_LITERAL|1|2|19
PLUS|+|2|21
INT_LITERAL|2|2|23
STAR|*|2|25
INT_LITERAL|3|2|27
SEMIC|;|2|28
IDENT|x|3|5
ASSIGN|=|3|7
IDENT|x|3|9
MINUS|-|3|11
INT_LITERAL|1|3|13
SEMIC|;|3|14
KW_RETURN|return|4|5
SEMIC|;|4|11
RBRACE|}|5|1
EOF||6|1
```

---

### 17.3 `tests/tools/lexer/strings.nv`

```nova
fn main() : void {
    print_str("hello");
    print_str("a\\n");
    print_str("a\\\"");
    return;
}
```

### `tests/tools/lexer/strings.tok`

```text
KW_FN|fn|1|1
IDENT|main|1|4
LPAREN|(|1|8
RPAREN|)|1|9
COLON|:|1|11
KW_VOID|void|1|13
LBRACE|{|1|18
IDENT|print_str|2|5
LPAREN|(|2|14
STR_LITERAL|"hello"|2|15
RPAREN|)|2|22
SEMIC|;|2|23
IDENT|print_str|3|5
LPAREN|(|3|14
STR_LITERAL|"a\\n"|3|15
RPAREN|)|3|20
SEMIC|;|3|21
IDENT|print_str|4|5
LPAREN|(|4|14
STR_LITERAL|"a\\\""|4|15
RPAREN|)|4|21
SEMIC|;|4|22
KW_RETURN|return|5|5
SEMIC|;|5|11
RBRACE|}|6|1
EOF||7|1
```

---

### 17.4 `tests/tools/lexer/comments.nv`

```nova
// hello
fn main() : void {
    /* block comment */
    return;
}
```

### `tests/tools/lexer/comments.tok`

```text
KW_FN|fn|2|1
IDENT|main|2|4
LPAREN|(|2|8
RPAREN|)|2|9
COLON|:|2|11
KW_VOID|void|2|13
LBRACE|{|2|18
KW_RETURN|return|4|5
SEMIC|;|4|11
RBRACE|}|5|1
EOF||6|1
```

---

## 18. 推荐实现顺序

### Step A：支持命令行参数

新增：

- `nova_runtime_init`
- `arg_count`
- `arg_get`
- sema builtin 注册
- codegen main 调整

先写一个 Nova 程序验证：

```nova
fn main() : void {
    print_int(arg_count());
    print_str(arg_get(1));
    return;
}
```

---

### Step B：实现 token output helper

先不扫描复杂 token，只能输出固定内容也行。

验证：

- `buf_push_str`
- `buf_push_int`
- `write_file`

---

### Step C：实现 whitespace / line-column

确保 line/column 稳定。

---

### Step D：实现 identifiers / keywords / ints

先让 `basic.nv` 和 `expressions.nv` 过。

---

### Step E：实现 strings

支持 escape skipping，不解释 escape。

---

### Step F：实现 comments

支持 `//` 和 `/* */`。

---

### Step G：接 CTest

把 Nova lexer tool 纳入自动化测试。

---

## 19. 本阶段最容易犯的错误

### 错误 1：想一次性重写 C++ lexer

不要一开始追求完全等价。先实现能跑的 Nova lexer prototype。

### 错误 2：没有命令行参数支持

没有 `arg_count / arg_get`，Nova 程序很难成为工具。

### 错误 3：line/column 更新混乱

lexer 测试最容易因为列号错一位失败。

### 错误 4：字符串 escape 处理错

记住：

> `\"` 中的 `"` 不是字符串结束符。

### 错误 5：过早设计 token 数据结构

Nova v0 没有 struct，不要硬造复杂 token list。先直接输出 token dump。

---

## 20. 验收标准

### 合格

- Nova 程序能通过命令行读取 input/output path
- 能扫描 basic token
- 能输出 token dump
- 至少 `basic.nv` 测试通过

### 良好

- identifiers / keywords / ints / strings / comments 全支持
- line/column 准确
- 多个测试通过
- 测试接入 CTest

### 优秀

- 输出格式稳定
- 错误处理清楚
- 结果能和 C++ lexer 的核心 token 行为对齐
- 代码结构较清晰，有 helper 函数

---

## 21. 完成标志

Step 7 完成的标志是：

> 一个用 Nova 写的 lexer/token dump 工具，可以被当前 Nova 编译器编译，并正确扫描一批 Nova 源文件。

也就是说：

```text
Nova compiler compiles Nova lexer
Nova lexer reads Nova source
Nova lexer emits token dump
```

这就是 self-hosting 路线的第一块基石。

---

## 22. 下一步预告

Step 8 可以考虑：

- 用 Nova 写一个更结构化的 tokenizer library
- 给 Nova 添加最小 `record` / `struct` 能力
- 或者实现一个 Nova-written source formatter
- 或者开始写 Nova parser prototype

建议 Step 7 完成后再决定，因为它会暴露 Nova 当前最缺的语言能力。
