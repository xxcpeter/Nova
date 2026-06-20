# Nova Project — Step 29
## 题目：Runtime Process/Filesystem APIs + Nova Build Driver

---

## 1. 当前起点

Step 28 已经完成：

```text
tools/nova_compile.nv 已新增
Nova-written compile-to-C driver 可用
tests/tools/compile/positive 已接入
CLI bad-args regression tests 已接入
RunNovaToolCliTest.cmake 已加入
6 个主要 Nova tool 的 bad-args tests 已覆盖
self_host.sh 已加入 nova_compile stage1 smoke
ctest 全过
scripts/self_host.sh 全过
```

现在 Nova 已经可以：

```text
C++ seed compiler
  -> build Nova-written tools

Nova-written nova_codegen
  -> generate C

Nova-written nova_compile
  -> compile input.nv to output.c

scripts/self_host.sh
  -> run stage0 / stage1 / stage2 workflow
```

但是 self-host workflow 仍然依赖 shell script：

```text
scripts/self_host.sh
```

Nova 程序本身还不能：

```text
run external commands
check whether a file exists
check whether a directory exists
create directories
remove temporary files
drive a build workflow
```

Step 29 的目标就是补齐最小 runtime process/filesystem 能力，并开始实现 Nova-written build driver。

---

## 2. 本次作业目标

Step 29 的目标是：

> **让 Nova 程序具备最小 build-tool 能力，并新增 `tools/nova_build.nv` prototype。**

核心成果：

```text
1. 新增 runtime process API
2. 新增 runtime filesystem API
3. C++ sema / Nova checker / Nova codegen 同步认识这些 builtin
4. docs/standard_library.md 更新
5. 新增 tests 覆盖 runtime process/filesystem API
6. 新增 tools/nova_build.nv
7. nova_build 可以驱动至少一个 self-host 或 compile workflow
8. self_host.sh 仍然通过
9. full ctest 通过
```

这一步的重点是从：

```text
Shell script drives Nova self-hosting
```

推进到：

```text
Nova program can start driving build/self-hosting workflows
```

---

## 3. 本阶段定位

Step 29 是 runtime capability + build driver step。

它应该解决：

```text
Nova 程序不能执行外部命令
Nova 程序不能做基本 filesystem checks
Nova-written build driver 无法实现
self-host workflow 只能靠 shell script
runtime / builtin 表缺少 process/filesystem API
```

它不要求解决：

```text
完整 build system
incremental build cache
parallel build scheduler
package manager
cross-platform shell abstraction
safe argv-based process spawning
directory recursive remove
glob
environment variable management
complete path library
```

---

## 4. 本阶段不做什么

Step 29 不做：

```text
package manager
project config file
dependency graph cache
parallel jobs
watch mode
portable Windows shell support
async process handling
process stdout/stderr capture
recursive directory traversal
LSP / formatter
module system
```

Step 29 只做最小可用能力：

```text
run_command
file_exists
dir_exists
make_dir
remove_file
tools/nova_build.nv prototype
```

---

# Part A：Runtime Process API

---

## 5. 新增 run_command

新增 user-facing builtin：

```nova
run_command(command: str) : int
```

语义：

```text
执行 shell command
返回 command exit code
exit code 0 表示成功
非 0 表示失败
```

示例：

```nova
let code : int = run_command("cmake --build build --parallel");
if (code != 0) {
    nova_runtime_error("build failed");
}
```

---

## 6. C runtime 实现建议

在：

```text
runtime/nova_runtime.h
runtime/nova_runtime.c
```

新增：

```c
int run_command(const char* command);
```

实现可以先使用：

```c
system(command)
```

建议尽量返回 normalized exit code。

Linux/POSIX 下可以：

```c
#include <stdlib.h>
#include <sys/wait.h>

int run_command(const char* command) {
    int status = system(command);

    if (status == -1) {
        nova_runtime_error("run_command failed to start command");
    }

#ifdef WIFEXITED
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
#endif

    return status;
}
```

如果你想保持实现更简单，也可以直接返回 `system(command)` 的结果。  
但更推荐 normalized exit code，因为 Nova 侧判断更自然：

```nova
if (run_command("true") != 0) {
    ...
}
```

---

## 7. Safety note

`run_command(command: str)` 通过 shell 执行字符串。

这意味着：

```text
它不是安全的 argv-based process API
不要把 untrusted input 拼进 command
```

Step 29 可以接受这个限制，因为它是 build driver prototype。

在 `docs/standard_library.md` 中要明确：

```text
run_command executes through the host shell and is intended for trusted build tooling.
```

---

# Part B：Runtime Filesystem APIs

---

## 8. 新增 filesystem builtins

建议新增：

```nova
file_exists(path: str) : bool
dir_exists(path: str) : bool
make_dir(path: str) : void
remove_file(path: str) : void
```

最小用途：

```text
file_exists:
  检查生成文件是否存在

dir_exists:
  检查 build/tmp 目录是否存在

make_dir:
  创建 work directory

remove_file:
  删除生成的临时文件
```

---

## 9. C runtime declarations

在 `runtime/nova_runtime.h`：

```c
int file_exists(const char* path);
int dir_exists(const char* path);
void make_dir(const char* path);
void remove_file(const char* path);
```

Nova `bool` 当前一般映射到 C `int`，所以返回 `int` 即可。

---

## 10. C runtime implementation

POSIX-style 实现：

```c
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int file_exists(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISREG(st.st_mode) ? 1 : 0;
}

int dir_exists(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISDIR(st.st_mode) ? 1 : 0;
}

void make_dir(const char* path) {
    if (mkdir(path, 0777) != 0) {
        if (errno == EEXIST && dir_exists(path)) {
            return;
        }

        nova_runtime_error(str_concat("cannot create directory: ", path));
    }
}

void remove_file(const char* path) {
    if (remove(path) != 0) {
        if (errno == ENOENT) {
            return;
        }

        nova_runtime_error(str_concat("cannot remove file: ", path));
    }
}
```

如果你不想在 C runtime 内调用 `str_concat`，可以写一个小 helper 或直接使用 existing runtime error style。

注意：

```text
make_dir 不要求 recursive mkdir
remove_file 不要求删除 directory
```

这些留给以后。

---

## 11. Behavior decisions

建议固定这些行为：

```text
file_exists(path):
  true only for regular file

dir_exists(path):
  true only for directory

make_dir(path):
  succeeds if directory already exists
  errors if path exists but is not a directory
  does not create parent directories recursively

remove_file(path):
  succeeds if file does not exist
  removes regular file
  errors on other failure
```

这样 build driver 写起来比较方便。

---

# Part C：Builtin Table Alignment

---

## 12. C++ sema builtin table

更新 C++ semantic analyzer builtin list。

新增：

```text
run_command(command: str) : int
file_exists(path: str) : bool
dir_exists(path: str) : bool
make_dir(path: str) : void
remove_file(path: str) : void
```

要求：

```text
参数数量正确
参数类型正确
返回类型正确
```

---

## 13. Nova checker builtin handling

更新 `lib/checker.nv`。

新增或更新：

```text
is_runtime_builtin
infer_runtime_builtin_call
check runtime builtin args
```

新增 signature：

```text
run_command(str) -> int
file_exists(str) -> bool
dir_exists(str) -> bool
make_dir(str) -> void
remove_file(str) -> void
```

这些不是 no-return builtin。

---

## 14. Nova codegen runtime call handling

更新 `lib/codegen_c.nv`。

要求：

```text
run_command(...) 直接生成 run_command(...)
file_exists(...) 直接生成 file_exists(...)
dir_exists(...) 直接生成 dir_exists(...)
make_dir(...) 直接生成 make_dir(...)
remove_file(...) 直接生成 remove_file(...)
```

如果 codegen runtime builtin inference 有 return type table，也要加入：

```text
run_command -> int
file_exists -> bool
dir_exists -> bool
make_dir -> void
remove_file -> void
```

---

## 15. VS Code extension builtin list

如果 Step 26 grammar 高亮 runtime builtin，应更新：

```text
tools/vscode-nova-syntax/syntaxes/nova.tmLanguage.json
```

把这些加入 runtime builtin list：

```text
run_command
file_exists
dir_exists
make_dir
remove_file
```

---

## 16. docs/standard_library.md

更新：

```text
docs/standard_library.md
```

新增 section：

```markdown
## Process

### run_command(command: str) : int

Runs a trusted shell command and returns its exit code.

## Filesystem

### file_exists(path: str) : bool
### dir_exists(path: str) : bool
### make_dir(path: str) : void
### remove_file(path: str) : void
```

说明限制：

```text
run_command uses the host shell
make_dir is not recursive
remove_file does not remove directories
these APIs are intended for trusted build tooling
```

---

# Part D：Runtime API Tests

---

## 17. Typecheck tests

新增：

```text
tests/tools/typecheck/positive/runtime_process_filesystem.nv
tests/tools/typecheck/positive/runtime_process_filesystem.out
```

示例：

```nova
fn main() : void {
    let code : int = run_command("true");
    let a : bool = file_exists("README.md");
    let b : bool = dir_exists("runtime");

    make_dir("./tmp/nova_runtime_api_test");
    remove_file("./tmp/nova_runtime_api_test_file.txt");

    return;
}
```

Expected:

```text
Check OK
Structs: 0
Enums: 0
Functions: 1
```

如果 `run_command("true")` 在 checker tests 中不会执行，只是 typecheck，所以没问题。

---

## 18. C++ codegen runtime test

如果项目环境是 POSIX/Linux，可以新增 C++ codegen test：

```text
tests/codegen/positive/runtime_process_filesystem.nv
tests/codegen/positive/runtime_process_filesystem.out
```

示例：

```nova
fn main() : void {
    let code : int = run_command("true");
    print_int(code);

    if (file_exists("README.md")) {
        print_str("file yes");
    } else {
        print_str("file no");
    }

    if (dir_exists("runtime")) {
        print_str("dir yes");
    } else {
        print_str("dir no");
    }

    make_dir("./tmp/nova_runtime_api_test");
    if (dir_exists("./tmp/nova_runtime_api_test")) {
        print_str("made dir");
    }

    remove_file("./tmp/nova_runtime_api_missing_file.txt");
    print_str("remove ok");

    return;
}
```

Expected:

```text
0
file yes
dir yes
made dir
remove ok
```

Note:

```text
This test assumes POSIX-like shell command `true`.
```

If you want to avoid shell dependency in normal CTest, only include typecheck/codegen compile smoke and leave runtime behavior to nova_build tests.

---

## 19. Nova tool codegen test

Add:

```text
tests/tools/codegen/positive/runtime_process_filesystem.nv
tests/tools/codegen/positive/runtime_process_filesystem.out
```

Same or smaller program.

If `run_command("true")` is acceptable in your environment, include it.

---

# Part E：Nova Build Driver

---

## 20. 新增 tools/nova_build.nv

新增：

```text
tools/nova_build.nv
```

目标：

```text
Nova-written build driver prototype
```

This is not a full build system.  
It is a minimal driver that proves Nova can execute build/self-host commands.

---

## 21. Recommended CLI

Support at least:

```bash
nova_build self-host
nova_build compile <input.nv> <output.c>
```

Optional:

```bash
nova_build test
```

Suggested usage:

```text
Usage: nova_build <self-host|compile|test> [args...]
```

---

## 22. Minimal behavior

### `nova_build self-host`

Simplest version:

```nova
let code : int = run_command("scripts/self_host.sh");
if (code != 0) {
    nova_runtime_error("self-host command failed");
}
```

This is acceptable for Step 29.

It proves:

```text
Nova program can drive the existing self-host workflow.
```

### `nova_build compile <input.nv> <output.c>`

Can call C++ seed compiler:

```nova
let command : str = "./build/nova_compile " + input + " " + output;
let code : int = run_command(command);
if (code != 0) {
    nova_runtime_error("compile command failed");
}
```

This is a prototype and assumes trusted paths.

### `nova_build test`

Can call:

```nova
run_command("ctest --test-dir build -j8 --output-on-failure");
```

Optional.

---

## 23. More independent self-host mode, optional

Instead of simply calling:

```text
scripts/self_host.sh
```

`nova_build.nv` could directly execute each command:

```text
build stage0
compile stage0 C
build stage1
compile stage1 C
build stage2
compile stage2 C
run representative tests
```

This is more impressive but more code.

Step 29 does not require full reimplementation of `self_host.sh`.

Minimum accepted:

```text
nova_build self-host delegates to scripts/self_host.sh
```

---

## 24. Quoting limitation

Because `run_command(command: str)` uses shell string, Step 29 should document:

```text
nova_build expects trusted paths
paths with spaces are not supported or not guaranteed
do not pass untrusted input into shell commands
```

This is acceptable for now.

---

# Part F：Build Driver Tests

---

## 25. Build driver typecheck

Add:

```text
tests/tools/typecheck/positive/nova_build_cli.nv
tests/tools/typecheck/positive/nova_build_cli.out
```

This can be a small input that uses `run_command`, `file_exists`, etc., or you can rely on compiling `tools/nova_build.nv` through smoke.

---

## 26. Compile nova_build with seed compiler

Manual smoke:

```bash
./build/nova_compile tools/nova_build.nv /tmp/nova_build.c
cc /tmp/nova_build.c runtime/nova_runtime.c -I runtime -o /tmp/nova_build
/tmp/nova_build self-host
```

This will run the full self-host workflow.

---

## 27. CTest for nova_build bad args

Use Step 28 CLI runner:

```text
tests/tools/cli/negative/nova_build_bad_args.err
```

Expected:

```text
Usage: nova_build <self-host|compile|test> [args...]
```

Add:

```cmake
add_nova_tool_cli_negative_test(build nova_build nova_build_bad_args)
```

---

## 28. Optional CTest for nova_build self-host

You may choose not to run `nova_build self-host` in normal CTest if it duplicates `scripts/self_host.sh`.

Options:

```text
Option A:
  Keep scripts/self_host.sh as the CTest selfhost test.
  Document nova_build self-host as manual smoke.

Option B:
  Add nova_build_self_host CTest labeled selfhost;build.
  This will be slower because it compiles nova_build then runs self_host.
```

Recommended for Step 29:

```text
Keep scripts/self_host.sh as canonical CTest selfhost.
Run nova_build self-host manually or as optional local smoke.
```

---

# Part G：CMake Integration

---

## 29. Add nova_build compile smoke

At minimum, add a CMake helper or test that compiles `tools/nova_build.nv` and checks bad args.

If using CLI runner:

```cmake
add_nova_tool_cli_negative_test(build nova_build nova_build_bad_args)
```

This proves:

```text
C++ seed compiler can compile nova_build
generated nova_build runs
bad args path works
```

---

## 30. Runtime API tests in CMake

Register:

```cmake
add_nova_typecheck_positive_test(runtime_process_filesystem)
add_nova_cpp_codegen_positive_test(runtime_process_filesystem)
add_nova_codegen_tool_test(runtime_process_filesystem)
```

Only add runtime execution tests if environment supports shell `true`.

---

# Part H：Docs Updates

---

## 31. Update docs/standard_library.md

Add Process and Filesystem sections.

Include warning:

```text
run_command executes trusted shell commands.
It is intended for build tooling, not sandboxed execution.
```

---

## 32. Update docs/tools.md

Add:

```text
tools/nova_build.nv
Usage: nova_build <self-host|compile|test> [args...]
```

Explain:

```text
nova_build is a prototype build driver.
It currently delegates self-hosting to scripts/self_host.sh.
It requires runtime process/filesystem APIs.
```

---

## 33. Update docs/limitations.md

Add limitations:

```text
run_command is shell-based and not safe for untrusted input.
make_dir is not recursive.
remove_file does not remove directories.
nova_build is a prototype, not a package manager or full build system.
```

---

## 34. Update docs/bootstrap.md

Add optional section:

```text
Nova build driver prototype
```

Example:

```bash
./build/nova_compile tools/nova_build.nv /tmp/nova_build.c
cc /tmp/nova_build.c runtime/nova_runtime.c -I runtime -o /tmp/nova_build
/tmp/nova_build self-host
```

---

## 35. Update README

Add short mention:

```markdown
## Nova build driver prototype

A prototype Nova-written build driver is available at `tools/nova_build.nv`.
It can delegate to the self-host workflow once compiled.
```

---

# Part I：Recommended Implementation Order

---

## 36. Step A：Add runtime APIs

Modify:

```text
runtime/nova_runtime.h
runtime/nova_runtime.c
```

Add:

```text
run_command
file_exists
dir_exists
make_dir
remove_file
```

---

## 37. Step B：Update builtin tables

Modify:

```text
src/sema.cpp
lib/checker.nv
lib/codegen_c.nv
tools/vscode-nova-syntax/syntaxes/nova.tmLanguage.json
docs/standard_library.md
```

---

## 38. Step C：Add runtime API tests

Add typecheck first.

Then add codegen tests if environment stable.

---

## 39. Step D：Add nova_build.nv

Implement:

```text
self-host
compile
optional test
```

Keep it simple.

---

## 40. Step E：Add nova_build CLI test

Add bad args test with existing CLI runner.

---

## 41. Step F：Manual smoke

Run:

```bash
./build/nova_compile tools/nova_build.nv /tmp/nova_build.c
cc /tmp/nova_build.c runtime/nova_runtime.c -I runtime -o /tmp/nova_build
/tmp/nova_build self-host
```

---

## 42. Step G：Docs update

Update:

```text
docs/standard_library.md
docs/tools.md
docs/limitations.md
docs/bootstrap.md
README.md
```

---

## 43. Step H：Full regression

Run:

```bash
cmake --build build --parallel
ctest --test-dir build -j8 --output-on-failure
scripts/self_host.sh
```

Optionally:

```bash
/tmp/nova_build self-host
```

---

# Part J：验收标准

---

## 44. 合格

```text
run_command exists
file_exists exists
dir_exists exists
make_dir exists
remove_file exists
C++ sema recognizes new builtins
Nova checker recognizes new builtins
Nova codegen emits new builtins
runtime API tests pass
tools/nova_build.nv exists
nova_build bad args test passes
full ctest passes
```

---

## 45. 良好

```text
docs/standard_library.md documents process/filesystem APIs
docs/tools.md documents nova_build
nova_build self-host delegates to scripts/self_host.sh
nova_build compile works for a simple input
scripts/self_host.sh still passes
VS Code builtin list updated
```

---

## 46. 优秀

```text
nova_build self-host smoke is documented and reproducible
nova_build can run test workflow
runtime API behavior is tested by generated Nova programs
limitations clearly document shell/FS restrictions
```

---

## 47. 完成标志

Step 29 完成时，应该可以说：

```text
Nova programs can run trusted build commands and perform basic filesystem checks, enabling a prototype Nova-written build driver.
```

至少这些成立：

```text
run_command works
basic filesystem builtins work
tools/nova_build.nv compiles
nova_build can invoke self-host workflow
builtins are aligned across C++ sema / Nova checker / Nova codegen
standard library docs are updated
ctest and self_host pass
```

---

## 48. 后续关联

Step 30 将作为 Phase 2 final milestone：

```text
final self-hosting status
final docs cleanup
README / bootstrap / limitations update
phase3 plan reference
release-style validation
```

Step 29 是最后一个 substantial feature step in Phase 2. Step 30 should focus on validation and milestone closure.
