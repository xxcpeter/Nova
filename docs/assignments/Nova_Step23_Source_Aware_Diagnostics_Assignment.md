# Nova Project — Step 23
## 题目：Source-aware Diagnostics v1

---

## 1. 当前起点

Step 22 已经完成：

```text
stage-generated tools 可以运行代表性输入
str + str 语法糖已经接入
Nova checker 有基础 no-return / missing-return 检查
```

当前还保留的限制之一是：

```text
imported source diagnostics may use flattened source locations
```

也就是说，经过 import/source_loader 展开之后，错误位置可能指向展开后的大字符串，而不是原始 `.nv` 文件。

Step 23 的目标是做第一版 source-aware diagnostics。

---

## 2. 本次作业目标

Step 23 的目标是：

> **让 C++ compiler 和 Nova-written frontend/checker 的 diagnostics 尽量输出原始 source file 的 file:line:column。**

至少做到：

```text
main.nv import bad.nv
bad.nv 中有错误
diagnostic 指向 bad.nv 的本地 line/column
```

目标格式：

```text
path/to/file.nv:line:column: ErrorKind: message
```

例如：

```text
tests/import/negative/type_error/bad.nv:2:19: CheckerError: type error in variable declaration: expected 'int', got 'str'
```

---

## 3. 本阶段定位

Step 23 是 diagnostics 改进，不是 module system。

它应该解决：

```text
import 展开后错误位置丢失原始文件身份
tokens / parse nodes / AST locations 没有 source file
diagnostic 文案只包含 line:column
```

它不要求解决：

```text
namespace
export/private
alias import
package search path
separate compilation
完整 macro/source-map v2
multi-span diagnostics
pretty code frame
```

---

## 4. 本阶段不做什么

Step 23 不做：

```text
import system v2 path normalization
完整 module system
LSP diagnostics
diagnostic code frame
warning system
source range highlighting
fix-it suggestions
```

这些后续 step 再做。

Step 23 的范围是：

```text
source file identity + local line/column
```

---

# Part I：C++ compiler diagnostics

---

## 5. C++ 当前问题

C++ compiler 现在的 pipeline 大致是：

```text
load_source_with_imports(input_path)
  -> returns flattened source string

Lexer(flattened_source)
  -> Token{ kind, lexeme, location(line, column) }

Parser
  -> AST nodes carry location(line, column)

Sema / Codegen diagnostics
  -> print line:column
```

问题是：

```text
line/column 是 flattened source 的位置
不是原始 file 的位置
```

---

## 6. C++ v1 推荐设计

### 6.1 SourceLocation 增加 file

如果现在有类似：

```cpp
struct SourceLocation {
    int line;
    int column;
};
```

建议改成：

```cpp
struct SourceLocation {
    std::string file;
    int line = 1;
    int column = 1;
};
```

如果担心复制字符串，可以后续优化为 source id。Step 23 先用 `std::string` 更简单。

---

### 6.2 Token location 携带 file

Token 中的 location 应该变成：

```cpp
SourceLocation location;
```

lexer 产生 token 时填：

```text
file
line
column
```

---

### 6.3 AST location 自动获得 file

Parser 从 token 构造 AST node 时，直接使用 token.location。

这样：

```text
StructDecl.location
FunctionDecl.location
LetStmt.location
Expr.location
```

都会自动带 file。

---

## 7. C++ source_loader v1 方案

有两种实现方案。推荐方案 A。

---

### 方案 A：SourceLoader 返回 flattened source + line map

新增一个结构：

```cpp
struct SourceLineMapping {
    std::string file;
    int original_line;
};

struct LoadedSource {
    std::string text;
    std::vector<SourceLineMapping> line_map;
};
```

含义：

```text
LoadedSource.text 第 N 行
对应 line_map[N - 1].file
对应 line_map[N - 1].original_line
```

source_loader 展开 import 时，不仅拼接 text，也同步记录每一行来自哪个文件。

然后 lexer 仍然扫描 flattened text，但创建 token location 时：

```cpp
SourceLocation loc{
    .file = loaded.line_map[current_line - 1].file,
    .line = loaded.line_map[current_line - 1].original_line,
    .column = current_column
};
```

优点：

```text
lexer 改动较小
parser/sema 自动继承 file-aware location
source_loader 能保持 include-once/cycle detection
```

---

### 方案 B：source_loader 生成 #line 风格 marker

例如展开成：

```text
#line "file.nv" 1
...
```

不推荐，因为 Nova lexer/parser 还要特殊跳过这些 marker，会污染语言本身。

---

## 8. C++ source_loader 行为细节

source_loader 在处理每个文件时，需要按行追加：

```text
append line text
append SourceLineMapping{file, original_line}
```

对于 import 行本身，有两种选择：

### 选择 A：import 行不进入 flattened source

这是当前 include 语义最常见做法。

```nova
import "lib.nv";
fn main() : void { ... }
```

展开后 import 行被 imported file 内容替代。

这种情况下：

```text
imported content 的 line_map 指向 imported file
main 后续内容的 line_map 指向 main file 原始 line
```

### 选择 B：import 行保留为空行

也可以保留一个空行来维持 main 文件行数连续。

例如：

```text
<imported content>
<empty line mapped to main.nv line import_line>
```

Step 23 建议选择 A 或保持现有行为，不强制改变 import 展开语义。

关键是：

```text
每一行 flattened text 都必须有 line_map
```

---

## 9. C++ diagnostics formatting

当前错误可能是：

```text
SemanticError at 2:19: ...
```

Step 23 之后建议变成：

```text
SemanticError at path/to/file.nv:2:19: ...
```

或者更常见：

```text
path/to/file.nv:2:19: SemanticError: ...
```

二选一即可，建议统一项目风格。

推荐：

```text
file:line:column: ErrorKind: message
```

例如：

```text
tests/sema/negative/bad.nv:2:19: SemanticError: type error in variable initializer: expected int, got str
```

如果为了减少 golden diff，也可以暂时保留：

```text
SemanticError at file:line:column: message
```

关键是加入 file。

---

## 10. C++ entry point 需要更新

所有现代 C++ entry point 都应该使用新的 `LoadedSource`：

```text
nova_lex
nova_parse
nova_sema
nova_compile
```

以前：

```cpp
std::string source = load_source_with_imports(path);
Lexer lexer(source);
```

改成类似：

```cpp
LoadedSource source = load_source_with_imports(path);
Lexer lexer(source.text, source.line_map);
```

或者：

```cpp
Lexer lexer(source);
```

看你想怎么封装。

---

# Part II：Nova-written frontend diagnostics

---

## 11. Nova 当前问题

Nova-written frontend 当前大致是：

```text
load_source_with_imports(input_path) -> str
tokenize(source) -> vec<Token>
parse_program_tree(tokens) -> ParseNode
check_program(tree)
```

Token / ParseNode 现在通常只有：

```text
line
column
```

没有：

```text
file
```

所以 Nova-side diagnostics 也无法指向原始 imported file。

---

## 12. Nova v1 推荐设计

为了不把 Step 23 做太大，Nova side 可以采用一个轻量结构：

```nova
struct SourceLineInfo {
    file: str;
    line: int;
}

struct LoadedSource {
    text: str;
    lines: vec<SourceLineInfo>;
}
```

然后新增 API：

```nova
fn load_source_with_imports_info(path: str) : LoadedSource
```

保留旧 API：

```nova
fn load_source_with_imports(path: str) : str
```

旧 API 可以包装新 API：

```nova
fn load_source_with_imports(path: str) : str {
    return load_source_with_imports_info(path).text;
}
```

这样已有工具可以逐步迁移，不需要一次改完所有调用点。

---

## 13. Nova Token 增加 file

当前 `Token` 可能类似：

```nova
struct Token {
    kind: TokenKind;
    lexeme: str;
    line: int;
    column: int;
}
```

建议改成：

```nova
struct Token {
    kind: TokenKind;
    lexeme: str;
    file: str;
    line: int;
    column: int;
}
```

然后 tokenizer 需要知道 line mapping。

可以新增：

```nova
fn tokenize_loaded(source: LoadedSource) : vec<Token>
```

保留旧 API：

```nova
fn tokenize(source: str) : vec<Token>
```

旧 API 可以给 file 填空字符串或 `"<source>"`。

---

## 14. Nova ParseNode 增加 file

当前 ParseNode 可能类似：

```nova
struct ParseNode {
    kind: ParseNodeKind;
    name: str;
    value: str;
    children: vec<ParseNode>;
    line: int;
    column: int;
}
```

改成：

```nova
struct ParseNode {
    kind: ParseNodeKind;
    name: str;
    value: str;
    children: vec<ParseNode>;
    file: str;
    line: int;
    column: int;
}
```

Parser 构造 ParseNode 时，从 token 拿：

```text
token.file
token.line
token.column
```

---

## 15. Nova diagnostics helper

`diagnostics.nv` 当前可能有：

```nova
fn checker_error_at(line: int, column: int, message: str) : void
```

Step 23 建议新增：

```nova
fn checker_error_at_source(file: str, line: int, column: int, message: str) : void
fn parse_error_at_source(file: str, line: int, column: int, message: str) : void
fn codegen_error_at_source(file: str, line: int, column: int, message: str) : void
```

保留旧函数：

```nova
fn checker_error_at(line: int, column: int, message: str) : void {
    checker_error_at_source("", line, column, message);
}
```

格式 helper：

```nova
fn format_source_location(file: str, line: int, column: int) : str {
    if (str_eq(file, "")) {
        return int_to_str(line) + ":" + int_to_str(column);
    }
    return file + ":" + int_to_str(line) + ":" + int_to_str(column);
}
```

---

## 16. Nova checker / parser 调用迁移

原来：

```nova
checker_error_at(node.line, node.column, "message");
```

迁移为：

```nova
checker_error_at_source(node.file, node.line, node.column, "message");
```

为了减少 diff，也可以新增：

```nova
fn checker_error_at_node(node: ParseNode, message: str) : void {
    checker_error_at_source(node.file, node.line, node.column, message);
}
```

但注意 `diagnostics.nv` 不应该依赖 `ParseNode`，所以这个 helper 应该放在：

```text
checker.nv
codegen_c.nv
parser.nv
```

而不是放进 `diagnostics.nv`。

---

## 17. Nova-side 是否必须一步完成？

Step 23 的核心目标是 source-aware diagnostics。  
如果一次改 C++ 和 Nova side 太大，可以分两段做：

```text
Step 23.1:
  C++ compiler diagnostics source-aware

Step 23.2:
  Nova-written frontend/checker diagnostics source-aware
```

但仍然属于 Step 23。

最低验收建议：

```text
C++ compiler import diagnostics source-aware 必须完成。
Nova frontend source-aware diagnostics 至少完成 Token / ParseNode file propagation 或建立 LoadedSource API。
```

---

# Part III：测试设计

---

## 18. C++ import diagnostic test

新增测试：

```text
tests/import/negative/source_location/
  main.nv
  bad.nv
  main.err
```

### `main.nv`

```nova
import "bad.nv";

fn main() : void {
    return;
}
```

### `bad.nv`

```nova
fn bad() : void {
    let x : int = "hello";
    return;
}
```

### `main.err`

推荐格式：

```text
tests/import/negative/source_location/bad.nv:2:19: SemanticError: type error in variable initializer: expected int, got str
```

如果你的错误文案不同，按实际项目风格调整。

---

## 19. C++ parse error imported file test

新增：

```text
tests/import/negative/source_location_parse/
  main.nv
  bad.nv
  main.err
```

### `main.nv`

```nova
import "bad.nv";

fn main() : void {
    return;
}
```

### `bad.nv`

```nova
fn bad() : void {
    let x : int = ;
}
```

期望错误指向：

```text
bad.nv:2:<column>
```

---

## 20. Nova frontend imported diagnostic test

如果 Nova side 完成 file propagation，新增：

```text
tests/tools/frontend_import/negative/source_location/
  main.nv
  bad.nv
  main.err
```

### `main.nv`

```nova
import "bad.nv";

fn main() : void {
    return;
}
```

### `bad.nv`

```nova
fn bad() : void {
    let x : int = "hello";
    return;
}
```

期望：

```text
bad.nv:2:19: CheckerError: ...
```

如果 Step 23 先只完成 C++ side，可以把这个测试留到 Nova side 完成后再接入。

---

## 21. No import normal diagnostic regression

确保普通无 import 文件仍然输出可接受位置。

例如：

```text
tests/sema/negative/source_location_basic.nv
```

```nova
fn main() : void {
    let x : int = "hello";
    return;
}
```

期望可以是：

```text
tests/sema/negative/source_location_basic.nv:2:19: SemanticError: ...
```

---

# Part IV：兼容与迁移策略

---

## 22. 是否一次修改所有 golden

加入 file 后，很多 `.err` golden 会变化。  
为了降低 diff，可以采用渐进策略。

### 策略 A：只给 import diagnostics 加 file

普通单文件错误继续保留旧格式：

```text
SemanticError at 2:19: ...
```

imported file 错误输出：

```text
SemanticError at bad.nv:2:19: ...
```

优点：

```text
golden diff 少
```

缺点：

```text
格式不完全统一
```

### 策略 B：所有 diagnostics 都输出 file

统一格式：

```text
file:line:column: ErrorKind: message
```

优点：

```text
格式清晰统一
长期更好
```

缺点：

```text
需要更新更多 golden
```

推荐 Step 23 采用策略 B。  
这是一次性痛苦，但后面更干净。

---

## 23. Source path 使用绝对还是相对？

建议输出相对路径，避免不同机器测试路径不同。

例如：

```text
tests/import/negative/source_location/bad.nv
```

不要输出：

```text
/home/peter/Nova/tests/import/negative/source_location/bad.nv
```

实现方式：

```text
source_loader 内部可以 canonical absolute path 用于 visited/cycle
diagnostic file name 使用 display path
display path 尽量相对 project root 或相对 input file
```

如果项目暂时没有 project root 概念，可以先输出：

```text
bad.nv
```

但更推荐测试中稳定使用相对路径。

---

## 24. visited key 和 display file 分离

source_loader 最好区分：

```text
canonical_path:
  用于 include-once 和 cycle detection

display_path:
  用于 diagnostics
```

例如：

```cpp
struct SourceFile {
    std::filesystem::path canonical_path;
    std::string display_path;
};
```

Step 23 不要求完整 import v2，但这个分离会避免后面重构。

---

# Part V：推荐实现顺序

---

## 25. C++ side 推荐顺序

### Step A：SourceLocation 增加 file

修改：

```text
include/token.h 或 source location 定义处
相关 AST location 使用处
```

---

### Step B：LoadedSource + line_map

修改：

```text
include/source_loader.h
src/source_loader.cpp
```

新增：

```cpp
LoadedSource load_source_with_imports(...);
```

如果已有函数签名被大量使用，可以：

```cpp
LoadedSource load_source_with_imports_info(...);
std::string load_source_with_imports(...);
```

逐步迁移。

---

### Step C：Lexer 接收 line_map

修改 lexer 构造：

```cpp
Lexer(std::string source, std::vector<SourceLineMapping> line_map)
```

或者：

```cpp
Lexer(const LoadedSource& source)
```

token 生成时映射 current flattened line 到 original file/line。

---

### Step D：Diagnostic formatting

修改：

```text
LexerError
ParserError
SemaError
CodegenError
```

或者它们的统一 formatting 函数。

---

### Step E：入口迁移

更新：

```text
nova_lex
nova_parse
nova_sema
nova_compile
tests drivers if needed
```

---

### Step F：更新 tests

先加 import source location tests，再更新必要 golden。

---

## 26. Nova side 推荐顺序

### Step A：LoadedSource struct

在 `source_loader.nv` 或单独文件定义：

```nova
struct SourceLineInfo {
    file: str;
    line: int;
}

struct LoadedSource {
    text: str;
    lines: vec<SourceLineInfo>;
}
```

---

### Step B：新增 load_source_with_imports_info

保留旧 API。

---

### Step C：Token 增加 file

修改：

```text
lib/token.nv
lib/tokenizer.nv
```

新增：

```nova
fn tokenize_loaded(source: LoadedSource) : vec<Token>
```

---

### Step D：ParseNode 增加 file

修改：

```text
lib/parse_tree.nv
lib/parser.nv
```

---

### Step E：diagnostics.nv 增加 source-aware helper

保持 diagnostics 不依赖 ParseNode。

---

### Step F：frontend/checker/codegen 逐步迁移

更新：

```text
tools/nova_frontend.nv
tools/nova_checker.nv
tools/nova_codegen.nv
lib/checker.nv
lib/codegen_c.nv
```

---

# Part VI：验收标准

---

## 27. 合格

```text
C++ compiler diagnostics 包含 source file
imported file 中的 sema error 指向 imported file
imported file 中的 parse error 指向 imported file
全量 CTest 通过
```

---

## 28. 良好

```text
nova_lex / nova_parse / nova_sema / nova_compile 都使用 source-aware locations
普通单文件 diagnostics 也统一输出 file:line:column
source_loader 区分 canonical path 和 display path
```

---

## 29. 优秀

```text
Nova-written frontend/checker diagnostics 也包含 source file
Token 和 ParseNode 都携带 file
frontend_import negative tests 覆盖 imported file diagnostics
stage-generated frontend/checker 仍可运行
bootstrap_stage1 仍通过
```

---

## 30. 完成标志

Step 23 完成时，应该可以说：

```text
Nova diagnostics are source-aware enough for multi-file imported programs.
```

至少下面场景成立：

```text
main.nv imports bad.nv
bad.nv has parse/sema/check error
diagnostic points to bad.nv local line/column
```

同时：

```bash
cmake --build build --parallel
ctest --test-dir build -j8 --output-on-failure
scripts/bootstrap_stage1.sh
```

都通过。

---

## 31. 后续关联

Step 23 完成后，Step 24 可以继续做：

```text
Import system v2
  path normalization
  better include-once
  better cycle error
  C++ and Nova source_loader behavior alignment
```

Step 23 只负责：

```text
file-aware diagnostics
```

Step 24 再负责：

```text
import path robustness
```
