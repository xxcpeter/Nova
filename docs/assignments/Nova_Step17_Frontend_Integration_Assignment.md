# Nova Project — Step 17
## 题目：整合 Nova Frontend Driver，并统一 diagnostics / smoke tests

---

## 1. 当前进度

到 Step 16 结束时，Nova tools 已经具备：

```text
tokenize(source) -> vec<Token>
parse_program_tree(tokens) -> ParseNode
check_program(tree) -> declaration + expression type checking
```

现在已经有：

```text
tools/nova_lexer.nv
tools/nova_tokenizer.nv
tools/nova_parser.nv
tools/nova_expr_parser.nv
tools/nova_checker.nv
```

Step 17 的目标不是再加一个新的语言特性，而是把这些工具能力整合成一个更像 compiler frontend 的入口。

---

## 2. 本次作业目标

新增：

```text
tools/nova_frontend.nv
```

让它支持不同 frontend mode：

```text
tokens
parse
check
```

建议命令形式：

```bash
nova_frontend <mode> <input.nv> <output>
```

例如：

```bash
nova_frontend tokens tests/foo.nv foo.tok
nova_frontend parse tests/foo.nv foo.out
nova_frontend check tests/foo.nv foo.check
```

对应行为：

```text
tokens:
  source -> tokenize -> dump_tokens

parse:
  source -> tokenize -> parse_program_tree -> dump_parse_tree

check:
  source -> tokenize -> parse_program_tree -> check_program
```

---

## 3. 本阶段不做什么

Step 17 不做：

- module/import
- codegen
- 新语言特性
- LSP / VS Code extension
- 替换 C++ compiler
- 大规模重构 runtime

这些留给后续：

```text
Step 18: import/module or include system
Step 19: Nova codegen prototype
Step 20: bootstrap milestone
```

---

## 4. 文件策略

现在还没有 import/module，所以 `nova_frontend.nv` 可以先复制：

- tokenizer
- parser
- checker

也就是短期允许重复。

等 Step 18 再拆成：

```text
lib/token.nv
lib/tokenizer.nv
lib/parser.nv
lib/checker.nv
lib/diagnostics.nv

tools/nova_frontend.nv
```

当前不要为了去重阻塞 Step 17。

---

## 5. main 函数建议

```nova
fn main() : void {
    if (arg_count() != 4) {
        print_str("Usage: nova_frontend <tokens|parse|check> <input.nv> <output>");
        return;
    }

    let mode : str = arg_get(1);
    let input_path : str = arg_get(2);
    let output_path : str = arg_get(3);

    let source : str = read_file(input_path);

    if (str_eq(mode, "tokens")) {
        let tokens : vec<Token> = tokenize(source);
        let output : str = dump_tokens(tokens);
        write_file(output_path, output);
        return;
    }

    if (str_eq(mode, "parse")) {
        let tokens : vec<Token> = tokenize(source);
        let tree : ParseNode = parse_program_tree(tokens);
        let output : str = dump_parse_tree(tree);
        write_file(output_path, output);
        return;
    }

    if (str_eq(mode, "check")) {
        let tokens : vec<Token> = tokenize(source);
        let tree : ParseNode = parse_program_tree(tokens);
        let output : str = check_program(tree);
        write_file(output_path, output);
        return;
    }

    nova_runtime_error(str_concat("unknown frontend mode '", str_concat(mode, "'")));
}
```

---

## 6. Diagnostics 目标

Step 17 不要求重新设计错误系统，但要统一 frontend 的使用方式。

推荐保持现有格式：

```text
ParseError at line:column: message
CheckerError at line:column: message
CheckerError: message
```

runtime stderr 中仍然可能带：

```text
Nova runtime error: ...
```

测试里继续使用：

```cmake
-DSTRIP_RUNTIME_PREFIX=ON
```

这样 `.err` 文件只写真实 frontend error。

---

## 7. 新增 CMake helper

### 7.1 Positive frontend tests

```cmake
function(add_nova_frontend_test mode name ext)
    add_test(
        NAME nova_tool_frontend_${mode}_${name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/nova_frontend.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/frontend/${mode}/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/frontend/${mode}/${name}.${ext}
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/frontend_${mode}_${name}
            -DCC=cc
            -DTOOL_ARGS=${mode}
            -P ${NOVA_TOOL_TEST_DRIVER}
    )

    set_tests_properties(
        nova_tool_frontend_${mode}_${name}
        PROPERTIES LABELS "nova_tool;frontend;${mode}"
    )
endfunction()
```

这要求 `RunNovaToolTest.cmake` 支持可选 `TOOL_ARGS`。

如果你的 runner 现在只支持：

```cmake
COMMAND "${tool_exe}" "${INPUT}" "${actual_output}"
```

可以改成：

```cmake
if(DEFINED TOOL_ARGS)
    separate_arguments(extra_args UNIX_COMMAND "${TOOL_ARGS}")
else()
    set(extra_args "")
endif()

execute_process(
    COMMAND "${tool_exe}" ${extra_args} "${INPUT}" "${actual_output}"
    ...
)
```

---

### 7.2 Negative frontend tests

```cmake
function(add_nova_frontend_negative_test mode name)
    add_test(
        NAME nova_tool_frontend_negative_${mode}_${name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/nova_frontend.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/frontend/negative/${mode}/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/frontend/negative/${mode}/${name}.err
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/frontend_negative_${mode}_${name}
            -DSTRIP_RUNTIME_PREFIX=ON
            -DCC=cc
            -DTOOL_ARGS=${mode}
            -P ${NOVA_TOOL_NEGATIVE_TEST_DRIVER}
    )

    set_tests_properties(
        nova_tool_frontend_negative_${mode}_${name}
        PROPERTIES LABELS "nova_tool;frontend;negative;${mode}"
    )
endfunction()
```

---

## 8. Runner 修改建议

### 8.1 `RunNovaToolTest.cmake`

在运行 generated tool 前增加：

```cmake
if(DEFINED TOOL_ARGS)
    separate_arguments(extra_args UNIX_COMMAND "${TOOL_ARGS}")
else()
    set(extra_args "")
endif()
```

然后运行：

```cmake
execute_process(
    COMMAND "${tool_exe}" ${extra_args} "${INPUT}" "${actual_output}"
    RESULT_VARIABLE run_rv
    OUTPUT_VARIABLE run_out
    ERROR_VARIABLE run_err
)
```

### 8.2 `RunNovaToolNegativeTest.cmake`

同样增加 `TOOL_ARGS` 支持：

```cmake
if(DEFINED TOOL_ARGS)
    separate_arguments(extra_args UNIX_COMMAND "${TOOL_ARGS}")
else()
    set(extra_args "")
endif()

execute_process(
    COMMAND "${tool_exe}" ${extra_args} "${INPUT}" "${actual_output}"
    RESULT_VARIABLE run_rv
    OUTPUT_VARIABLE run_out
    ERROR_VARIABLE run_err
)
```

这样所有 mode test 都可以复用同一套 runner。

---

## 9. Tests — tokens mode

### 9.1 `tests/tools/frontend/tokens/basic.nv`

```nova
fn main() : void {
    return;
}
```

### `tests/tools/frontend/tokens/basic.tok`

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

### 9.2 `tests/tools/frontend/tokens/enum_expr.nv`

```nova
enum Color {
    Red;
}

fn main() : void {
    let c : Color = Color.Red;
    return;
}
```

### `tests/tools/frontend/tokens/enum_expr.tok`

```text
KW_ENUM|enum|1|1
IDENT|Color|1|6
LBRACE|{|1|12
IDENT|Red|2|5
SEMIC|;|2|8
RBRACE|}|3|1
KW_FN|fn|5|1
IDENT|main|5|4
LPAREN|(|5|8
RPAREN|)|5|9
COLON|:|5|11
KW_VOID|void|5|13
LBRACE|{|5|18
KW_LET|let|6|5
IDENT|c|6|9
COLON|:|6|11
IDENT|Color|6|13
ASSIGN|=|6|19
IDENT|Color|6|21
DOT|.|6|26
IDENT|Red|6|27
SEMIC|;|6|30
KW_RETURN|return|7|5
SEMIC|;|7|11
RBRACE|}|8|1
EOF||9|1
```

---

## 10. Tests — parse mode

### 10.1 `tests/tools/frontend/parse/expression.nv`

```nova
fn main() : void {
    let x : int = 1 + 2 * 3;
    return;
}
```

### `tests/tools/frontend/parse/expression.out`

```text
Program
  Function name=main return=void
    Block
      LetStmt name=x type=int
        BinaryExpr op=+
          IntLiteralExpr value=1
          BinaryExpr op=*
            IntLiteralExpr value=2
            IntLiteralExpr value=3
      ReturnStmt
```

---

### 10.2 `tests/tools/frontend/parse/condition.nv`

```nova
fn main() : void {
    if (x > 0) {
        return;
    }
    return;
}
```

### `tests/tools/frontend/parse/condition.out`

```text
Program
  Function name=main return=void
    Block
      IfStmt
        Condition
          BinaryExpr op=>
            IdentifierExpr name=x
            IntLiteralExpr value=0
        Then
          Block
            ReturnStmt
      ReturnStmt
```

---

## 11. Tests — check mode positive

### 11.1 `tests/tools/frontend/check/let_return_int.nv`

```nova
fn id(x: int) : int {
    let y : int = x;
    return y;
}

fn main() : void {
    print_int(id(1));
    return;
}
```

### `tests/tools/frontend/check/let_return_int.check`

```text
Check OK
Structs: 0
Enums: 0
Functions: 2
```

---

### 11.2 `tests/tools/frontend/check/struct_enum_vec.nv`

```nova
enum Color {
    Red;
    Green;
}

struct Point {
    x: int;
    y: int;
}

fn main() : void {
    let c : Color = Color.Red;
    let p : Point = Point { x: 1, y: 2 };
    let xs : vec<int> = vec_new();
    vec_push(xs, p.x);
    if (c == Color.Red) {
        print_int(vec_get(xs, 0));
    }
    return;
}
```

### `tests/tools/frontend/check/struct_enum_vec.check`

```text
Check OK
Structs: 1
Enums: 1
Functions: 1
```

---

## 12. Tests — check mode negative

### 12.1 `tests/tools/frontend/negative/check/type_mismatch.nv`

```nova
fn main() : void {
    let x : int = "hello";
    return;
}
```

### `tests/tools/frontend/negative/check/type_mismatch.err`

```text
CheckerError at 2:19: type error in variable declaration: expected 'int', got 'str'
```

---

### 12.2 `tests/tools/frontend/negative/check/undefined_variable.nv`

```nova
fn main() : void {
    print_int(x);
    return;
}
```

### `tests/tools/frontend/negative/check/undefined_variable.err`

```text
CheckerError at 2:15: undefined variable 'x'
```

---

## 13. Tests — parse mode negative

### 13.1 `tests/tools/frontend/negative/parse/missing_expr.nv`

```nova
fn main() : void {
    let x : int = ;
    return;
}
```

### `tests/tools/frontend/negative/parse/missing_expr.err`

```text
ParseError at 2:19: expected expression
```

---

## 14. CMake 添加测试

```cmake
add_nova_frontend_test(tokens basic tok)
add_nova_frontend_test(tokens enum_expr tok)

add_nova_frontend_test(parse expression out)
add_nova_frontend_test(parse condition out)

add_nova_frontend_test(check let_return_int check)
add_nova_frontend_test(check struct_enum_vec check)

add_nova_frontend_negative_test(check type_mismatch)
add_nova_frontend_negative_test(check undefined_variable)
add_nova_frontend_negative_test(parse missing_expr)
```

---

## 15. Smoke tests

Step 17 除了小单元测试，还应该加入 smoke tests。

目标：用 `nova_frontend check` 检查当前工具源码。

建议先从最小的开始：

```text
tools/nova_lexer.nv
tools/nova_tokenizer.nv
tools/nova_parser.nv
tools/nova_expr_parser.nv
tools/nova_checker.nv
```

由于当前还没有 import/module，这些文件体积大，可能暴露 checker 缺失的 runtime builtin 或 parser 语法支持。可以逐个打开，不要求一次全过。

建议 CMake 里先添加：

```cmake
add_nova_frontend_smoke_check(nova_tokenizer tools/nova_tokenizer.nv)
add_nova_frontend_smoke_check(nova_expr_parser tools/nova_expr_parser.nv)
add_nova_frontend_smoke_check(nova_checker tools/nova_checker.nv)
```

输出统一期望：

```text
Check OK
```

但你当前 `check_program()` 输出带 counts，所以 smoke expected 可以直接只用当前实际完整输出，或者另写一个 `--quiet` mode。为了简单，Step 17 先用完整输出。

---

## 16. 建议的 smoke helper

```cmake
function(add_nova_frontend_smoke_check name source_path)
    add_test(
        NAME nova_tool_frontend_smoke_${name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/nova_frontend.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/${source_path}
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/frontend/smoke/${name}.check
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/frontend_smoke_${name}
            -DCC=cc
            -DTOOL_ARGS=check
            -P ${NOVA_TOOL_TEST_DRIVER}
    )

    set_tests_properties(
        nova_tool_frontend_smoke_${name}
        PROPERTIES LABELS "nova_tool;frontend;smoke"
    )
endfunction()
```

---

## 17. Step 17 验收标准

### 合格

- 新增 `tools/nova_frontend.nv`
- 支持 `tokens` / `parse` / `check` 三种 mode
- CMake runner 支持 `TOOL_ARGS`
- frontend positive tests 通过

### 良好

- frontend negative tests 通过
- diagnostics 输出稳定
- check mode 能覆盖 Step 16 的 type checker

### 优秀

- 能 smoke check 当前一个或多个 Nova tool 源码
- frontend driver 成为后续 Step 18 import/module 的统一入口
- 所有 mode 的输出文件后缀清晰：`.tok` / `.out` / `.check`

---

## 18. 完成标志

Step 17 完成时，应该能运行：

```bash
nova_frontend tokens input.nv output.tok
nova_frontend parse input.nv output.out
nova_frontend check input.nv output.check
```

并且 CTest 中有对应的 positive / negative / smoke tests。

这一步完成后，Nova-written frontend 已经有统一入口。  
下一步 Step 18 就可以开始做 import/module，把重复工具代码拆成共享库。
