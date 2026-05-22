# Nova Project — Step 18
## 题目：实现最小 import/include system，并把 Nova tools 拆成共享库

---

## 1. 当前进度

到 Step 17 结束时，Nova 已经有统一 frontend driver：

```text
nova_frontend tokens input.nv output.tok
nova_frontend parse  input.nv output.out
nova_frontend check  input.nv output.check
```

内部链路已经完整：

```text
source
  -> tokenize
  -> parse_program_tree
  -> check_program
```

现在最大的问题不是功能缺失，而是工程结构：

```text
tools/nova_tokenizer.nv
tools/nova_expr_parser.nv
tools/nova_checker.nv
tools/nova_frontend.nv
```

这些文件里复制了大量 tokenizer / parser / checker 代码。

Step 18 的目标是：

> **实现最小 import/include system，让 Nova tools 可以共享 lib/*.nv 代码。**

---

## 2. 本次作业目标

本阶段做两件事：

```text
Part A: C++ compiler 支持 import directive
Part B: 把 Nova tools 拆成 lib/*.nv + tools/*.nv
```

目标效果：

```nova
import "../lib/tokenizer.nv";
import "../lib/parser.nv";
import "../lib/checker.nv";

fn main() : void {
    ...
}
```

然后：

```bash
nova_compile tools/nova_frontend.nv nova_frontend.c
```

可以自动把 imports 展开后再编译。

---

## 3. 重要定位：这是 include system，不是完整 module system

Step 18 不做复杂 module。

本阶段的 `import` 只是 source-level include：

```text
import path -> read file -> recursively include -> concatenate source
```

不做：

- namespace
- export / private
- alias
- package manager
- circular module semantics
- separate compilation
- incremental build
- import symbol visibility

所有 imported declarations 进入同一个全局 namespace。

这和 C 的 `#include` 更接近，不是 Python/Rust/TypeScript 那种 module。

---

## 4. import 语法

建议语法：

```nova
import "relative/path.nv";
```

例如：

```nova
import "../lib/token.nv";
import "../lib/tokenizer.nv";
```

规则：

1. 只能出现在 top-level。
2. 推荐放在文件最前面。
3. path 必须是 string literal。
4. path 相对当前文件所在目录解析。
5. 同一个 canonical file 只 include 一次。
6. cyclic import 报错。
7. missing import 报错。
8. import directive 本身不进入 lexer/parser。

---

## 5. 为什么 import directive 不进入 lexer/parser

为了让 Step 18 改动最小，建议把 import 当成 pre-lex directive。

也就是说：

```text
source file
  -> import expander
  -> flattened source
  -> lexer
  -> parser
```

所以 C++ parser 暂时不需要新增 `ImportDecl` AST。

Nova tokenizer 也不需要马上新增 `KW_IMPORT`。

以后如果要做真正 module system，再把 import 纳入 lexer/parser/AST。

---

## 6. Part A：C++ compiler 支持 import expansion

### 6.1 新增 SourceLoader / ImportExpander

建议新增：

```text
include/source_loader.h
src/source_loader.cpp
```

提供：

```cpp
std::string load_source_with_imports(const std::filesystem::path& input_path);
```

内部做：

```text
read file
scan import lines
resolve relative path
detect duplicate include
detect cycle
concatenate imported source before current file body
```

---

### 6.2 import line 识别

先用简单规则即可：

```text
optional whitespace
import
whitespace
"relative/path.nv"
optional whitespace
;
optional whitespace
```

匹配示例：

```nova
import "../lib/token.nv";
```

不要求支持：

```nova
import "../lib/" + name;
import foo;
```

---

### 6.3 import expander 伪代码

```cpp
std::string expand_file(path file) {
    path canonical = weakly_canonical(file);

    if (active_stack contains canonical) {
        throw ImportError("cyclic import involving '" + file + "'");
    }

    if (visited contains canonical) {
        return "";
    }

    visited.insert(canonical);
    active_stack.push(canonical);

    std::string result;

    for each line in file:
        if line is import directive:
            path imported = canonical.parent_path() / import_path;
            result += expand_file(imported);
            result += "\n";
        else:
            result += line;
            result += "\n";

    active_stack.pop();
    return result;
}
```

---

### 6.4 error 类型

可以先用 `std::runtime_error`，但错误文案建议稳定：

```text
ImportError: cannot open import '../lib/missing.nv'
ImportError: cyclic import involving 'a.nv'
ImportError: malformed import directive
```

如果你的 command-line tools 统一 catch exception，可以输出到 stderr。

---

### 6.5 哪些 C++ tools 要使用 SourceLoader

建议这些都改：

```text
nova_lex
nova_parse
nova_sema
nova_compile
```

也就是说 main 里从：

```cpp
std::string source = read_file(input_path);
```

改成：

```cpp
std::string source = load_source_with_imports(input_path);
```

这样 import 支持在所有 compiler 阶段一致。

---

## 7. Part B：Nova frontend 也支持 import expansion

如果只改 C++ compiler，那么：

```bash
nova_compile tools/nova_frontend.nv nova_frontend.c
```

可以编译带 import 的 Nova tool。

但是 `nova_frontend check some_file.nv out.check` 如果 `some_file.nv` 里面也有 imports，Nova frontend 自己也需要处理 imports。

所以 Step 18 建议在 Nova 侧也实现一个简单版本：

```nova
fn load_source_with_imports(path: str) : str
```

如果当前 Nova runtime 没有路径 dirname/join/canonical 相关 helper，可以先做简化版本：

```text
只支持相对当前工作目录的 import path
```

也就是：

```nova
import "lib/token.nv";
```

先不支持相对当前文件目录。

不过更推荐在 runtime 里加最小路径 helper：

```nova
path_dirname(path: str) : str
path_join(base: str, child: str) : str
```

这样 Nova frontend 行为能和 C++ compiler 一致。

---

## 8. 最小可接受方案

如果你不想 Step 18 一次做太大，可以分两级。

### 合格方案

只实现 C++ compiler import expansion。

这样可以把 tools 拆成：

```text
lib/*.nv
tools/*.nv
```

并且 `nova_compile tools/nova_frontend.nv` 能工作。

### 良好方案

Nova frontend 的 `tokens / parse / check` mode 也使用 `load_source_with_imports`。

这样：

```bash
nova_frontend check examples/foo_with_imports.nv out.check
```

也能工作。

### 优秀方案

C++ compiler 和 Nova frontend 都支持：

- relative path import
- duplicate import once
- cyclic import detection
- missing import diagnostics

---

## 9. 建议 lib 拆分结构

新增：

```text
lib/
  token.nv
  tokenizer.nv
  parse_tree.nv
  parser.nv
  checker.nv
  diagnostics.nv
  source_loader.nv
```

建议职责：

```text
lib/token.nv
  TokenKind
  Token
  make_token
  token_kind_name

lib/tokenizer.nv
  tokenize
  dump_tokens
  character/string scanning helpers

lib/parse_tree.nv
  ParseNodeKind
  ParseNode
  ParseResult
  ExprParseResult
  make_node
  add_child
  dump_parse_tree
  parse_node_header

lib/parser.nv
  parse_program_tree
  parse declarations
  parse statements
  parse expressions

lib/checker.nv
  SymbolTables
  collect_top_level_symbols
  validate_declarations
  validate_function_bodies
  check_program
  infer_expr_type

lib/diagnostics.nv
  parse_error
  checker_error
  checker_error_at

lib/source_loader.nv
  load_source_with_imports
```

可以根据实际代码体积调整，不需要一次拆得特别细。

---

## 10. import order 建议

因为没有 namespace/export，import 顺序会影响“文本展开顺序”，但 sema/codegen 已经支持 top-level 两阶段收集和 function prototypes，所以声明顺序问题应该不大。

建议入口文件这样写：

```nova
import "../lib/diagnostics.nv";
import "../lib/token.nv";
import "../lib/tokenizer.nv";
import "../lib/parse_tree.nv";
import "../lib/parser.nv";
import "../lib/checker.nv";

fn main() : void {
    ...
}
```

如果 `parser.nv` 依赖 `token.nv` 和 `parse_tree.nv`，它自己也可以 import：

```nova
import "token.nv";
import "parse_tree.nv";
```

因为 import expander 会 include once，所以重复 import 没问题。

---

## 11. 工具文件改造

### 11.1 `tools/nova_tokenizer.nv`

改成：

```nova
import "../lib/source_loader.nv";
import "../lib/tokenizer.nv";

fn main() : void {
    if (arg_count() != 3) {
        print_str("Usage: nova_tokenizer <input.nv> <output.tok>");
        return;
    }

    let input_path : str = arg_get(1);
    let output_path : str = arg_get(2);

    let source : str = load_source_with_imports(input_path);
    let tokens : vec<Token> = tokenize(source);
    let output : str = dump_tokens(tokens);

    write_file(output_path, output);
    return;
}
```

如果暂时不想让 tokenizer tool 展开 imports，可以继续用 `read_file(input_path)`。  
但 frontend tokens mode 建议使用 source loader。

---

### 11.2 `tools/nova_expr_parser.nv`

```nova
import "../lib/source_loader.nv";
import "../lib/tokenizer.nv";
import "../lib/parser.nv";

fn main() : void {
    ...
    let source : str = load_source_with_imports(input_path);
    let tokens : vec<Token> = tokenize(source);
    let tree : ParseNode = parse_program_tree(tokens);
    write_file(output_path, dump_parse_tree(tree));
}
```

---

### 11.3 `tools/nova_checker.nv`

```nova
import "../lib/source_loader.nv";
import "../lib/tokenizer.nv";
import "../lib/parser.nv";
import "../lib/checker.nv";

fn main() : void {
    ...
    let source : str = load_source_with_imports(input_path);
    let tokens : vec<Token> = tokenize(source);
    let tree : ParseNode = parse_program_tree(tokens);
    let output : str = check_program(tree);
    write_file(output_path, output);
}
```

---

### 11.4 `tools/nova_frontend.nv`

```nova
import "../lib/source_loader.nv";
import "../lib/tokenizer.nv";
import "../lib/parser.nv";
import "../lib/checker.nv";

fn main() : void {
    ...
}
```

---

## 12. Source location 注意事项

Textual import expansion 会改变 line/column。

例如：

```nova
import "lib.nv";

fn main() : void {
    let x : int = "hello";
}
```

如果展开后 lib 在前面，错误位置可能对应 flattened source，而不是原始 main 文件。

Step 18 暂时可以接受这个限制。

后续如果要改进，需要：

```text
token location = file + line + column
```

但这会影响 Token / ParseNode / diagnostics，适合以后单独做，不放 Step 18。

---

## 13. Import tests — C++ compiler positive

新增目录：

```text
tests/import/positive/simple/
```

### `tests/import/positive/simple/math.nv`

```nova
fn add(x: int, y: int) : int {
    return x + y;
}
```

### `tests/import/positive/simple/main.nv`

```nova
import "math.nv";

fn main() : void {
    print_int(add(1, 2));
    return;
}
```

### `tests/import/positive/simple/main.out`

```text
3
```

---

## 14. Import tests — C++ compiler duplicate include

新增：

```text
tests/import/positive/diamond/
```

### `tests/import/positive/diamond/common.nv`

```nova
fn value() : int {
    return 42;
}
```

### `tests/import/positive/diamond/a.nv`

```nova
import "common.nv";
```

### `tests/import/positive/diamond/b.nv`

```nova
import "common.nv";
```

### `tests/import/positive/diamond/main.nv`

```nova
import "a.nv";
import "b.nv";

fn main() : void {
    print_int(value());
    return;
}
```

### `tests/import/positive/diamond/main.out`

```text
42
```

这个测试确认：

```text
common.nv 只 include 一次
```

否则会 duplicate function。

---

## 15. Import tests — C++ compiler negative

新增：

```text
tests/import/negative/
```

### 15.1 `tests/import/negative/missing/main.nv`

```nova
import "missing.nv";

fn main() : void {
    return;
}
```

### `tests/import/negative/missing/main.err`

```text
ImportError: cannot open import 'missing.nv'
```

---

### 15.2 `tests/import/negative/cyclic/a.nv`

```nova
import "b.nv";

fn main() : void {
    return;
}
```

### `tests/import/negative/cyclic/b.nv`

```nova
import "a.nv";
```

### `tests/import/negative/cyclic/main.err`

```text
ImportError: cyclic import involving
```

这里 expected 可以只比较完整稳定文案，或者在 test runner 里做 contains check。  
如果你的 text compare 必须 exact match，建议输出固定：

```text
ImportError: cyclic import involving 'a.nv'
```

---

## 16. Import tests — Nova frontend check mode

新增：

```text
tests/tools/frontend_import/check/simple/
```

### `tests/tools/frontend_import/check/simple/math.nv`

```nova
fn add(x: int, y: int) : int {
    return x + y;
}
```

### `tests/tools/frontend_import/check/simple/main.nv`

```nova
import "math.nv";

fn main() : void {
    print_int(add(1, 2));
    return;
}
```

### `tests/tools/frontend_import/check/simple/main.check`

```text
Check OK
Structs: 0
Enums: 0
Functions: 2
```

---

## 17. Import tests — Nova frontend negative

### `tests/tools/frontend_import/negative/check/type_mismatch/lib.nv`

```nova
fn make_str() : str {
    return "hello";
}
```

### `tests/tools/frontend_import/negative/check/type_mismatch/main.nv`

```nova
import "lib.nv";

fn main() : void {
    let x : int = make_str();
    return;
}
```

### `tests/tools/frontend_import/negative/check/type_mismatch/main.err`

```text
CheckerError at 4:19: type error in variable declaration: expected 'int', got 'str'
```

Line number may differ if diagnostics use flattened source. If so, update expected to actual flattened-source location for Step 18.

---

## 18. CMake helpers

### 18.1 Import codegen positive

```cmake
function(add_import_codegen_positive_test name)
    add_test(
        NAME import_codegen_positive_${name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/import/positive/${name}/main.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/import/positive/${name}/main.out
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DCC=cc
            -P ${CODEGEN_TEST_DRIVER}
    )
    set_tests_properties(import_codegen_positive_${name} PROPERTIES LABELS "import;codegen;positive")
endfunction()
```

Use:

```cmake
add_import_codegen_positive_test(simple)
add_import_codegen_positive_test(diamond)
```

---

### 18.2 Import parser negative

If `nova_parse` uses `load_source_with_imports`, you can use text compare:

```cmake
function(add_import_negative_test name)
    add_test(
        NAME import_negative_${name}
        COMMAND ${CMAKE_COMMAND}
            -DTOOL=$<TARGET_FILE:nova_parse>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/import/negative/${name}/main.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/import/negative/${name}/main.err
            -DMODE=negative
            -DLABEL=stderr
            -P ${TEXT_TEST_DRIVER}
    )
    set_tests_properties(import_negative_${name} PROPERTIES LABELS "import;negative")
endfunction()
```

Use:

```cmake
add_import_negative_test(missing)
add_import_negative_test(cyclic)
```

---

### 18.3 Frontend import positive

```cmake
function(add_nova_frontend_import_check_test name)
    add_test(
        NAME nova_tool_frontend_import_check_${name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/nova_frontend.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/frontend_import/check/${name}/main.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/frontend_import/check/${name}/main.check
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/frontend_import_check_${name}
            -DCC=cc
            -DTOOL_ARGS=check
            -P ${NOVA_TOOL_TEST_DRIVER}
    )
    set_tests_properties(nova_tool_frontend_import_check_${name} PROPERTIES LABELS "nova_tool;frontend;import;check")
endfunction()
```

Use:

```cmake
add_nova_frontend_import_check_test(simple)
```

---

## 19. 推荐实现顺序

### Step A：实现 C++ SourceLoader

新增 `source_loader.cpp/h`，只支持：

```nova
import "path";
```

先让 `nova_compile` 支持 import。

### Step B：让所有 C++ tools 使用 SourceLoader

更新：

```text
nova_lex
nova_parse
nova_sema
nova_compile
```

### Step C：加 import codegen positive tests

先跑：

```text
simple
diamond
```

### Step D：加 import negative tests

测试 missing / cyclic。

### Step E：拆 lib/*.nv

先拆最小集合：

```text
lib/token.nv
lib/tokenizer.nv
lib/parse_tree.nv
lib/parser.nv
lib/checker.nv
```

### Step F：重写 tools/*.nv

让：

```text
nova_tokenizer.nv
nova_expr_parser.nv
nova_checker.nv
nova_frontend.nv
```

变成 import-based thin wrapper。

### Step G：Nova frontend 支持 imported input source

新增或移植：

```nova
load_source_with_imports
```

然后 `nova_frontend check` 能检查带 import 的 input。

### Step H：frontend import tests + smoke

确认 import-based tools 自己也能通过 smoke。

---

## 20. 常见错误

### 错误 1：没有 include-once

如果 diamond import 重复 include，会报 duplicate function。

### 错误 2：循环 import 没检测

会无限递归或 stack overflow。

### 错误 3：relative path 相对 cwd，而不是当前文件

推荐相对当前 importing file 的目录。

### 错误 4：imported source 没有补换行

两个文件拼接时如果缺少换行，可能把 token 拼到一起。

### 错误 5：拆 lib 后 duplicate definition

确保每个 lib 文件职责清楚，不要两个 lib 都定义同一个 struct / function。

---

## 21. 验收标准

### 合格

- C++ `nova_compile` 支持 `import "path.nv";`
- simple import codegen test 通过
- missing import 能报错
- tools 至少有一个改成 import-based wrapper

### 良好

- 所有 C++ entry tools 使用 SourceLoader
- duplicate import include once
- cyclic import detection
- `nova_frontend.nv` 改成 import-based wrapper
- frontend import check test 通过

### 优秀

- tokenizer / parser / checker / frontend 全部拆成 lib + thin tools
- Nova frontend 自己也支持 input source imports
- smoke tests 继续通过
- import diagnostics 稳定

---

## 22. 完成标志

Step 18 完成时，项目结构应该从：

```text
tools/nova_frontend.nv   // huge copied file
tools/nova_checker.nv    // huge copied file
tools/nova_expr_parser.nv
```

变成：

```text
lib/token.nv
lib/tokenizer.nv
lib/parse_tree.nv
lib/parser.nv
lib/checker.nv
lib/source_loader.nv

tools/nova_frontend.nv
tools/nova_checker.nv
tools/nova_expr_parser.nv
tools/nova_tokenizer.nv
```

并且：

```bash
nova_compile tools/nova_frontend.nv nova_frontend.c
nova_frontend check tools/nova_checker.nv out.check
```

仍然可以工作。

下一步 Step 19 就可以在这个库化结构上做 Nova codegen prototype，而不用继续复制大文件。
