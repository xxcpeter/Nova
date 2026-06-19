# Nova Project — Step 28
## 题目：Nova Compile Driver + CLI Regression Infrastructure

---

## 1. 当前起点

Step 27 已经完成：

```text
tests/tools 目录结构统一为 positive / negative
legacy lexer/parser 测试结构已清理或迁移
CMake test drivers / helper functions 已合并和统一
Nova tool positive / negative tests 已共用同一个 runner
focused regression tests 已补齐
Nova user-facing diagnostics 基本都有 source location
C++ ImportError 已改成 source-aware
full ctest 和 self_host workflow 正常
```

Phase 2 目前已经具备：

```text
source-aware diagnostics
stable import behavior
runtime / builtin / stdlib cleanup
stage0 -> stage1 -> stage2 nova_codegen workflow
VS Code developer tooling
regression test infrastructure
```

但是还有一个明显缺口：

```text
Nova-written compiler pipeline 还没有正式 compile driver
nova_codegen.nv 仍然承担了 compiler-like wrapper 的角色
CLI bad-args behavior 缺少集中 regression 覆盖
docs 里还没有完整 tools reference
self_host workflow 还没有明确验证 stage-generated nova_compile
```

Step 28 的目标就是把 Nova-written compile-to-C 入口和 CLI regression infrastructure 做实。

---

## 2. 本次作业目标

Step 28 的目标是：

> **新增 Nova-written compile driver，并建立 CLI regression 测试基础设施。**

核心成果：

```text
1. 新增 tools/nova_compile.nv
2. 新增 tests/tools/compile/positive/
3. 新增 add_nova_compile_tool_test 或等价 CMake helper
4. 新增 CLI bad-args regression tests
5. 新增或扩展 CLI test runner
6. self_host workflow 增加 stage-generated nova_compile smoke
7. 新增 docs/tools.md
8. 更新 README / bootstrap / limitations
9. full ctest 和 scripts/self_host.sh 通过
```

这一步不是单纯文档或 usage cleanup。  
它应该新增真实工具、真实 tests、真实 self-host smoke。

---

## 3. 本阶段定位

Step 28 是 compiler driver + CLI testing infrastructure step。

它应该解决：

```text
Nova-written compile-to-C driver 缺失
nova_codegen.nv 和 official compile driver 职责不清
stage-generated compiler driver 没有 smoke coverage
bad args / usage 行为缺少自动回归测试
tools 文档缺失
```

它不解决：

```text
运行 C compiler
spawn external command from Nova
build executable
process management
filesystem management
project build system
package manager
incremental compilation
module namespace
LSP / formatter
```

这些留给 Step 29 或 Phase 3。

---

## 4. 本阶段不做什么

Step 28 不做：

```text
runtime process API
run_command(...)
file_exists / dir_exists / make_dir / remove_file
tools/nova_build.nv
full build system
package manager
project config file
automatic cc invocation from Nova
```

`tools/nova_compile.nv` 只负责：

```text
input.nv -> output.c
```

不负责：

```text
output.c -> executable
```

这个边界很重要。

---

# Part A：新增 Nova-written Compile Driver

---

## 5. 新增 tools/nova_compile.nv

新增文件：

```text
tools/nova_compile.nv
```

它是 Nova-written official compile-to-C driver。

用法：

```bash
nova_compile <input.nv> <output.c>
```

为了避免和 C++ seed compiler 混淆，文档和测试里生成的 binary 可以叫：

```text
nova_compile_nova
nova_compile_stage1
```

但源码文件仍然叫：

```text
tools/nova_compile.nv
```

---

## 6. nova_compile.nv 的职责

`nova_compile.nv` 的 pipeline 应该是：

```text
load source with imports
tokenize
parse
check
generate C
write output.c
```

也就是：

```text
input.nv
  -> LoadedSource
  -> vec<Token>
  -> ParseNode program
  -> checked program
  -> C source string
  -> output.c
```

---

## 7. 推荐实现结构

示例结构：

```nova
import "../lib/source_loader.nv";
import "../lib/tokenizer.nv";
import "../lib/parser.nv";
import "../lib/checker.nv";
import "../lib/codegen_c.nv";

fn main() : void {
    if (arg_count() != 3) {
        nova_runtime_error("Usage: nova_compile <input.nv> <output.c>");
    }

    let input_path : str = arg_get(1);
    let output_path : str = arg_get(2);

    let source : LoadedSource = load_source_with_imports(input_path);
    let tokens : vec<Token> = tokenize_loaded(source);
    let program : ParseNode = parse_program_tree(tokens);

    check_program(program);

    let c_source : str = gen_c_program(program);
    write_file(output_path, c_source);

    return;
}
```

根据当前实际 API 调整函数名。  
重点是：`nova_compile.nv` 应该使用 source-aware pipeline，而不是旧的 file/string-only API。

---

## 8. nova_compile.nv 和 nova_codegen.nv 的关系

Step 28 不要求删除 `nova_codegen.nv`。

建议明确：

```text
nova_codegen.nv:
  backend/codegen-oriented testing tool
  继续服务 tests/tools/codegen/positive

nova_compile.nv:
  official Nova-written compile-to-C driver
  面向 user-facing toolchain 和 self-host smoke
```

也就是说：

```text
nova_codegen.nv 保留
nova_compile.nv 新增
```

未来可以考虑抽出：

```text
lib/compile_pipeline.nv
```

但 Step 28 不强制，避免扩大 scope。

---

## 9. usage string

`nova_compile.nv` 的 bad args 应该输出：

```text
Usage: nova_compile <input.nv> <output.c>
```

建议通过：

```nova
nova_runtime_error("Usage: nova_compile <input.nv> <output.c>");
```

这样可以保证：

```text
stderr 有信息
exit code 非 0
CLI negative test 可覆盖
```

---

# Part B：新增 compile tool tests

---

## 10. 新增测试目录

新增：

```text
tests/tools/compile/
  positive/
```

推荐最少加入：

```text
tests/tools/compile/positive/hello.nv
tests/tools/compile/positive/hello.out

tests/tools/compile/positive/struct_vec.nv
tests/tools/compile/positive/struct_vec.out

tests/tools/compile/positive/recursion.nv
tests/tools/compile/positive/recursion.out
```

这些 tests 应该验证：

```text
tools/nova_compile.nv
  -> generated C
  -> cc
  -> executable
  -> stdout compare
```

---

## 11. hello test

`tests/tools/compile/positive/hello.nv`：

```nova
fn main() : void {
    print_str("hello");
    return;
}
```

`tests/tools/compile/positive/hello.out`：

```text
hello
```

---

## 12. struct_vec test

`tests/tools/compile/positive/struct_vec.nv`：

```nova
struct Point {
    x: int;
    y: int;
}

fn main() : void {
    let points : vec<Point> = vec_new();

    vec_push(points, Point { x: 3, y: 4 });
    let p : Point = vec_get(points, 0);

    print_int(p.x);
    print_int(p.y);
    return;
}
```

Expected:

```text
3
4
```

---

## 13. recursion test

`tests/tools/compile/positive/recursion.nv`：

```nova
fn fact(n: int) : int {
    if (n <= 1) {
        return 1;
    }

    return n * fact(n - 1);
}

fn main() : void {
    print_int(fact(5));
    return;
}
```

Expected:

```text
120
```

---

## 14. compile import test，可选

如果现有 `RunNovaCodegenToolTest.cmake` 支持 directory-style input，可以加：

```text
tests/tools/compile/positive/import_basic/
  main.nv
  helper.nv
  main.out
```

如果 runner 只支持 flat input path，先不要强行加 import compile test。

Step 28 的最低要求是 flat compile driver tests。

---

# Part C：CMake Integration

---

## 15. 新增 add_nova_compile_tool_test

可以复用当前 codegen tool runner：

```text
cmake/RunNovaCodegenToolTest.cmake
```

因为它通常已经做了：

```text
compile Nova tool to C
compile generated tool executable
run generated tool on input.nv output.c
compile output.c
run final executable
compare stdout
```

新增 helper：

```cmake
function(add_nova_compile_tool_test name)
    set(test_name nova_tool_compile_positive_${name})

    add_test(
        NAME ${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/nova_compile.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/compile/positive/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/compile/positive/${name}.out
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/compile_positive_${name}
            -DCC=cc
            -P ${NOVA_TEST_DRIVER_TOOL_CODEGEN}
    )

    set_tests_properties(
        ${test_name}
        PROPERTIES LABELS "nova_tool;compile;positive"
    )
endfunction()
```

然后注册：

```cmake
add_nova_compile_tool_test(hello)
add_nova_compile_tool_test(struct_vec)
add_nova_compile_tool_test(recursion)
```

---

## 16. 命名建议

保持和 Step 27 后的风格一致：

```text
test name:
  nova_tool_compile_positive_<name>

labels:
  nova_tool;compile;positive

work dir:
  nova_tool_tests/compile_positive_<name>
```

这样和：

```text
nova_tool_codegen_positive_<name>
nova_tool_parser_positive_<name>
nova_tool_checker_negative_<name>
```

保持一致。

---

# Part D：CLI Regression Infrastructure

---

## 17. 为什么 Step 28 要新增 CLI tests

现在 test suite 已经覆盖很多功能，但 bad args 往往容易被忽略：

```text
tool 少参数
tool 多参数
frontend mode 错误
usage string 改名后忘记更新
工具 exit 0 但其实失败
```

这些问题很适合在 Step 28 统一测试。

---

## 18. 新增 CLI negative tests 目录

建议新增：

```text
tests/tools/cli/
  negative/
```

最少覆盖：

```text
tests/tools/cli/negative/nova_frontend_bad_args.err
tests/tools/cli/negative/nova_codegen_bad_args.err
tests/tools/cli/negative/nova_compile_bad_args.err
```

更完整可以覆盖：

```text
nova_tokenizer_bad_args.err
nova_parser_bad_args.err
nova_checker_bad_args.err
nova_frontend_bad_args.err
nova_codegen_bad_args.err
nova_compile_bad_args.err
```

---

## 19. expected stderr convention

如果 test runner strip runtime prefix：

```text
Usage: nova_compile <input.nv> <output.c>
```

如果不 strip：

```text
Nova runtime error: Usage: nova_compile <input.nv> <output.c>
```

Step 28 建议沿用 Step 27 中 negative Nova tool tests 的风格：

```text
strip runtime prefix
expected file 只写 Usage: ...
```

例如：

```text
Usage: nova_compile <input.nv> <output.c>
```

---

## 20. 新增 CLI test runner 方案 A：扩展 RunNovaToolTest.cmake

如果当前 `RunNovaToolTest.cmake` 已经可以：

```text
compile tool
run tool
expect failure
compare stderr
```

可以扩展它支持：

```cmake
-DCLI_TEST=ON
-DCLI_ARGS=""
-DOUTPUT_PATH=""
```

CLI test 时 command 应该是：

```text
tool_exe <CLI_ARGS...>
```

而不是：

```text
tool_exe <input> <output>
```

这种方案复用多，但 runner 会复杂一点。

---

## 21. 新增 CLI test runner 方案 B：单独 RunNovaToolCliTest.cmake

推荐更清楚：

```text
cmake/RunNovaToolCliTest.cmake
```

职责：

```text
compile TOOL_SOURCE with C++ seed nova_compile
compile generated C tool
run generated tool with CLI_ARGS
expect success/failure
compare stdout/stderr
```

最低只需要支持 negative bad-args：

```text
run generated tool with no args
expect non-zero
compare stderr
```

这会让 runner 更小、更专注。

---

## 22. 推荐 RunNovaToolCliTest.cmake 参数

建议：

```cmake
-DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
-DTOOL_SOURCE=...
-DEXPECT=...
-DRUNTIME_DIR=...
-DWORK_DIR=...
-DCC=cc
-DCLI_ARGS="..."
-DEXPECT_FAILURE=ON
-DSTRIP_RUNTIME_PREFIX=ON
```

可选：

```cmake
-DCOMPARE_STDOUT=ON
-DCOMPARE_STDERR=ON
```

但 Step 28 最低只需要 stderr negative。

---

## 23. 推荐 CLI helper

新增：

```cmake
function(add_nova_tool_cli_negative_test group tool name)
    set(test_name nova_tool_cli_negative_${group}_${name})

    add_test(
        NAME ${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/${tool}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/cli/negative/${name}.err
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/cli_negative_${group}_${name}
            -DCC=cc
            -DCLI_ARGS=
            -DEXPECT_FAILURE=ON
            -DSTRIP_RUNTIME_PREFIX=ON
            -P ${NOVA_TEST_DRIVER_TOOL_CLI}
    )

    set_tests_properties(
        ${test_name}
        PROPERTIES LABELS "nova_tool;cli;negative;${group}"
    )
endfunction()
```

Register:

```cmake
add_nova_tool_cli_negative_test(frontend nova_frontend nova_frontend_bad_args)
add_nova_tool_cli_negative_test(codegen nova_codegen nova_codegen_bad_args)
add_nova_tool_cli_negative_test(compile nova_compile nova_compile_bad_args)
```

Optional:

```cmake
add_nova_tool_cli_negative_test(tokenizer nova_tokenizer nova_tokenizer_bad_args)
add_nova_tool_cli_negative_test(parser nova_parser nova_parser_bad_args)
add_nova_tool_cli_negative_test(checker nova_checker nova_checker_bad_args)
```

---

## 24. frontend invalid mode test

Bad arg count is useful, but `nova_frontend` also has a mode argument.

Add optional:

```text
tests/tools/cli/negative/nova_frontend_invalid_mode.err
```

CLI args:

```text
invalid tests/tools/frontend/positive/check/let_return_int.nv /tmp/out.check
```

Expected:

```text
Usage: nova_frontend <tokens|parse|check> <input.nv> <output>
```

or:

```text
unknown frontend mode: invalid
```

Choose one and keep it stable.

This is optional, but valuable.

---

## 25. CLI tests should not require input files unless needed

For bad arg count tests, run with no args:

```text
nova_compile
```

This avoids input path dependency.

For invalid mode test, input file is needed.

---

# Part E：Usage Cleanup

---

## 26. Tool inventory

Audit these tools:

```text
tools/nova_tokenizer.nv
tools/nova_parser.nv
tools/nova_checker.nv
tools/nova_frontend.nv
tools/nova_codegen.nv
tools/nova_compile.nv
```

Confirm each tool has:

```text
Usage: ...
input path before output path
bad args -> nova_runtime_error(...)
no stale old tool names
diagnostics still source-aware
```

---

## 27. Recommended usage strings

Use:

```text
Usage: nova_tokenizer <input.nv> <output.tok>
Usage: nova_parser <input.nv> <output.out>
Usage: nova_checker <input.nv> <output.check>
Usage: nova_frontend <tokens|parse|check> <input.nv> <output>
Usage: nova_codegen <input.nv> <output.c>
Usage: nova_compile <input.nv> <output.c>
```

If generated binary is called `nova_compile_stage1`, the usage string can still say:

```text
nova_compile <input.nv> <output.c>
```

because that is the source tool name.

---

## 28. Optional lib/tool_cli.nv

If repeated usage code becomes annoying, add:

```text
lib/tool_cli.nv
```

Possible helpers:

```nova
fn usage_error(usage: str) : void {
    nova_runtime_error("Usage: " + usage);
}

fn require_arg_count(actual: int, expected: int, usage: str) : void {
    if (actual != expected) {
        usage_error(usage);
    }
    return;
}
```

Then tools can use:

```nova
require_arg_count(arg_count(), 3, "nova_compile <input.nv> <output.c>");
```

This is optional.

If adding this helper causes import churn or bootstrapping issues, keep usage inline.

---

# Part F：Self-host Smoke

---

## 29. Why self_host should include nova_compile

Step 25 proved the self-hosting codegen workflow.

Step 28 should additionally prove:

```text
stage-generated Nova codegen can produce a Nova-written compiler driver
that compiler driver can compile a representative program
```

This is stronger than normal CTest because it uses stage-generated tools.

---

## 30. Add nova_compile smoke to scripts/self_host.sh

Append or insert a section:

```bash
echo "[self_host] building nova_compile_stage1"

"$STAGE0" tools/nova_compile.nv "$WORK/nova_compile_stage1.c"
cc "$WORK/nova_compile_stage1.c" runtime/nova_runtime.c -I runtime -o "$WORK/nova_compile_stage1"

echo "[self_host] testing nova_compile_stage1"

"$WORK/nova_compile_stage1" tests/tools/compile/positive/hello.nv "$WORK/hello.from_nova_compile.c"
cc "$WORK/hello.from_nova_compile.c" runtime/nova_runtime.c -I runtime -o "$WORK/hello.from_nova_compile"

actual="$("$WORK/hello.from_nova_compile")"
if [ "$actual" != "hello" ]; then
    echo "nova_compile_stage1 smoke failed"
    echo "expected: hello"
    echo "actual: $actual"
    exit 1
fi
```

Adjust variable names to current `self_host.sh`.

---

## 31. Alternative: dedicated CTest

If you prefer not to expand `self_host.sh`, add a dedicated CTest:

```text
selfhost_nova_compile_driver
```

But since `scripts/self_host.sh` already represents the bootstrap workflow, putting the smoke there is cleaner.

---

# Part G：docs/tools.md

---

## 32. Add docs/tools.md

新增：

```text
docs/tools.md
```

Purpose:

```text
Document the Nova toolchain in one place.
```

---

## 33. docs/tools.md recommended structure

```markdown
# Nova Tools

## Overview

Nova currently has a C++ seed compiler and several Nova-written tools.

## C++ seed tools

### build/nova_lex
Usage: nova_lex <input.nv>

### build/nova_parse
Usage: nova_parse <input.nv>

### build/nova_sema
Usage: nova_sema <input.nv>

### build/nova_compile
Usage: nova_compile <input.nv> <output.c>

## Nova-written tools

### tools/nova_tokenizer.nv
Usage: nova_tokenizer <input.nv> <output.tok>

### tools/nova_parser.nv
Usage: nova_parser <input.nv> <output.out>

### tools/nova_checker.nv
Usage: nova_checker <input.nv> <output.check>

### tools/nova_frontend.nv
Usage: nova_frontend <tokens|parse|check> <input.nv> <output>

### tools/nova_codegen.nv
Usage: nova_codegen <input.nv> <output.c>

### tools/nova_compile.nv
Usage: nova_compile <input.nv> <output.c>

## Scripts

### scripts/self_host.sh
Runs the stage0/stage1/stage2 self-hosting workflow.
```

---

## 34. Clarify seed vs Nova-written compiler

Include this wording:

```text
`build/nova_compile` is the C++ seed compiler.
`tools/nova_compile.nv` is the Nova-written compile-to-C driver.

Generated binaries for `tools/nova_compile.nv` are usually named
`nova_compile_nova` or `nova_compile_stage1` in examples to avoid
confusion with the C++ seed compiler.
```

---

# Part H：Docs updates

---

## 35. README update

Add a short section:

```markdown
## Nova-written compile driver

Nova includes a Nova-written compile-to-C driver:

```bash
./build/nova_compile tools/nova_compile.nv /tmp/nova_compile_nova.c
cc /tmp/nova_compile_nova.c runtime/nova_runtime.c -I runtime -o /tmp/nova_compile_nova

/tmp/nova_compile_nova tests/tools/compile/positive/hello.nv /tmp/hello.c
cc /tmp/hello.c runtime/nova_runtime.c -I runtime -o /tmp/hello
/tmp/hello
```

For all tools, see `docs/tools.md`.
```

---

## 36. docs/bootstrap.md update

Add a section:

```text
Nova-written compile driver
```

Include:

```text
C++ seed compiler builds tools/nova_compile.nv
generated nova_compile compiles a representative .nv file to C
generated C compiles and runs
```

---

## 37. docs/limitations.md update

Add:

```text
The Nova-written compile driver compiles Nova source to C.
It does not invoke the system C compiler.
It is not a build system.
The C++ seed compiler is still required to build the initial stage0 tools.
```

---

## 38. docs/testing.md update

Add CLI test info:

```text
CLI regression tests live under tests/tools/cli/negative.
They compile Nova-written tools and check bad-args usage diagnostics.
```

Also add compile tool tests:

```text
tests/tools/compile/positive contains tests for tools/nova_compile.nv.
```

---

# Part I：Recommended Implementation Order

---

## 39. Step A：Add nova_compile.nv

Create:

```text
tools/nova_compile.nv
```

Manually verify:

```bash
./build/nova_compile tools/nova_compile.nv /tmp/nova_compile_nova.c
cc /tmp/nova_compile_nova.c runtime/nova_runtime.c -I runtime -o /tmp/nova_compile_nova
/tmp/nova_compile_nova tests/tools/codegen/positive/hello.nv /tmp/hello.c
cc /tmp/hello.c runtime/nova_runtime.c -I runtime -o /tmp/hello
/tmp/hello
```

---

## 40. Step B：Add compile positive tests

Add:

```text
tests/tools/compile/positive/hello
tests/tools/compile/positive/struct_vec
tests/tools/compile/positive/recursion
```

Add CMake helper and register tests.

---

## 41. Step C：Add CLI runner/tests

Add one of:

```text
cmake/RunNovaToolCliTest.cmake
```

or extend:

```text
cmake/RunNovaToolTest.cmake
```

Then add bad-args tests for at least:

```text
nova_frontend
nova_codegen
nova_compile
```

---

## 42. Step D：Unify usage strings

Audit:

```text
nova_tokenizer
nova_parser
nova_checker
nova_frontend
nova_codegen
nova_compile
```

Make usage consistent.

---

## 43. Step E：Add self_host nova_compile smoke

Update:

```text
scripts/self_host.sh
```

Verify generated `nova_compile_stage1`.

---

## 44. Step F：Add docs/tools.md and update docs

Update:

```text
docs/tools.md
README.md
docs/bootstrap.md
docs/limitations.md
docs/testing.md
```

---

## 45. Step G：Full regression

Run:

```bash
cmake --build build --parallel
ctest --test-dir build -j8 --output-on-failure
scripts/self_host.sh
```

---

# Part J：验收标准

---

## 46. 合格

```text
tools/nova_compile.nv exists
C++ seed compiler can compile nova_compile.nv
generated nova_compile can compile hello.nv to C
generated C compiles and runs
tests/tools/compile/positive exists
docs/tools.md exists
full ctest passes
```

---

## 47. 良好

```text
compile tool tests cover hello / struct_vec / recursion
CLI bad-args tests cover nova_frontend / nova_codegen / nova_compile
usage strings are consistent across major Nova-written tools
self_host includes nova_compile_stage1 smoke
README / bootstrap / limitations / testing docs updated
scripts/self_host.sh passes
```

---

## 48. 优秀

```text
CLI regression runner is reusable
bad-args tests cover all Nova-written tools
frontend invalid mode is tested
docs/tools.md clearly distinguishes C++ seed compiler and Nova-written driver
stage-generated nova_compile is part of the regular self-host workflow
```

---

## 49. 完成标志

Step 28 完成时，应该可以说：

```text
Nova has a Nova-written compile-to-C driver and automated CLI regression coverage for its toolchain.
```

至少这些成立：

```text
tools/nova_compile.nv is implemented
generated nova_compile compiles representative Nova programs
CLI usage diagnostics are tested
docs/tools.md documents the toolchain
self_host validates nova_compile_stage1
ctest and self_host pass
```

---

## 50. 后续关联

Step 29 将继续：

```text
Runtime Process/Filesystem APIs + Nova Build Driver
```

Step 29 的核心是：

```text
run_command(...)
file_exists(...)
dir_exists(...)
make_dir(...)
remove_file(...)
tools/nova_build.nv
```

也就是从：

```text
Nova can compile input.nv to output.c
```

推进到：

```text
Nova can drive build/self-host commands itself
```

Step 28 和 Step 29 的边界：

```text
Step 28:
  compiler driver and CLI regression

Step 29:
  runtime process/filesystem APIs and build driver
```
