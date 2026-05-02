# Nova Project — Step 6
## 题目：扩展 Runtime 与 Core Library，为自举做准备

## 1. 本次作业目标

在 Step 5 中，Nova 已经拥有了一个可以工作的 C 后端：

```text
Nova source
  -> lexer
  -> parser
  -> sema
  -> C codegen
  -> C compiler
  -> executable
```

Step 6 的目标是：

> **扩展 Nova 的 runtime / core library 能力，让 Nova 能开始编写更接近编译器组件的大型程序。**

这一步还不是自举，也不是要立刻用 Nova 重写编译器。

这一步要做的是：补齐写编译器所需的最小基础设施：

- 字符串处理
- 文件读写
- 字符扫描
- 字符串构建
- 简单动态缓冲区
- runtime 错误处理

---

## 2. 为什么需要 Step 6

如果以后你想用 Nova 写 lexer / parser / codegen，那么 Nova 程序至少需要做这些事情：

- 读取源文件
- 遍历字符串中的字符
- 拼接错误信息
- 构造输出代码
- 维护动态增长的 buffer
- 写出生成文件

当前 Nova v0 已经能写小程序，但还不适合写编译器。

所以 Step 6 的重点不是扩语法，而是：

> **通过 runtime API 给 Nova 提供足够的工程能力。**

---

## 3. 本阶段不做什么

本阶段明确不做：

- Nova-level `struct`
- Nova-level array syntax
- generic vector
- module system
- garbage collection
- self-hosted compiler
- AST in Nova
- parser in Nova
- optimizer
- IR

这一步仍然保持 Nova 语言本体很小。

---

## 4. 建议修改的文件

主要修改：

```text
runtime/
  nova_runtime.h
  nova_runtime.c

src/
  sema.cpp
```

可能需要检查但通常无需大改：

```text
src/
  codegen_c.cpp
```

因为大多数 runtime API 都是普通函数调用，codegen 已经能处理。

可选新增文档：

```text
docs/
  runtime_api.md
```

测试建议继续放在：

```text
tests/codegen/positive/
```

因为这些测试仍然是通过 `nova_compile -> C compiler -> executable` 跑端到端结果。

---

## 5. Runtime 设计原则

Nova v0 中，runtime 函数仍然应该表现为普通函数。

例如：

```nova
let s : str = str_concat("hello", " nova");
print_str(s);
```

编译器不需要为这些函数加特殊语法。

你只需要保证：

- runtime `.h` 中有声明
- runtime `.c` 中有实现
- sema 的 builtin function table 中注册了签名
- codegen 生成普通 C 函数调用

---

## 6. 字符串 API

你现在已有：

```nova
print_str(s: str) : void
str_eq(a: str, b: str) : bool
str_concat(a: str, b: str) : str
```

Step 6 建议新增：

```nova
str_len(s: str) : int
str_get(s: str, index: int) : int
str_slice(s: str, start: int, end: int) : str
str_starts_with(s: str, prefix: str) : bool
str_contains(s: str, needle: str) : bool
int_to_str(x: int) : str
```

说明：

- `str_get` 返回 `int`，因为 Nova v0 没有 `char` 类型。
- `str_get("nova", 0)` 应返回字符 `'n'` 的 ASCII 值，也就是 `110`。
- `str_slice(s, start, end)` 使用 `[start, end)` 语义。
- `int_to_str` 对构造错误信息和生成代码很有用。

---

## 7. 文件 API

已有：

```nova
read_file(path: str) : str
write_file(path: str, content: str) : void
```

Step 6 要求你把它们的行为做稳：

- 打不开文件时不能 segfault
- 写文件失败时不能静默成功
- malloc 失败时要有 runtime error
- 读出的字符串必须以 `\0` 结尾

建议 runtime 遇到错误时直接：

```text
Nova runtime error: <message>
```

并以非零状态退出。

---

## 8. Buffer API

写编译器时，频繁：

```nova
s = str_concat(s, piece);
```

会很笨重。

建议新增一个简单的 runtime string builder。

由于 Nova v0 没有 pointer / struct 类型，可以先用 `int` 作为 runtime handle。

推荐 API：

```nova
buf_new() : int
buf_push_str(buf: int, s: str) : void
buf_push_int(buf: int, x: int) : void
buf_to_str(buf: int) : str
```

示例：

```nova
fn main() : void {
    let b : int = buf_new();
    buf_push_str(b, "answer=");
    buf_push_int(b, 42);
    print_str(buf_to_str(b));
    return;
}
```

输出：

```text
answer=42
```

---

## 9. 可选：字符串向量 API

这部分不是必须，但如果你想更接近后续编译器实现，可以加一个最小 string vector。

```nova
str_vec_new() : int
str_vec_push(vec: int, value: str) : void
str_vec_get(vec: int, index: int) : str
str_vec_len(vec: int) : int
```

这对以后 token list / line list 很有帮助。

但如果你想保持 Step 6 小一点，可以先不做。

---

## 10. Sema 需要改什么

每新增一个 runtime 函数，都要在 `SemanticAnalyzer` 的 builtin function table 里注册。

例如：

```text
str_len(str) -> int
str_get(str, int) -> int
str_slice(str, int, int) -> str
buf_new() -> int
buf_push_str(int, str) -> void
buf_to_str(int) -> str
```

用户不允许定义和 runtime builtin 同名的函数。

也就是说：

```nova
fn str_len(s: str) : int {
    return 0;
}
```

应该报 duplicate function。

---

## 11. Codegen 需要改什么

大多数情况下不需要改。

因为：

```nova
let n : int = str_len(s);
```

可以直接生成：

```c
int n = str_len(s);
```

你只要保证：

- 生成 C 文件包含 `#include "nova_runtime.h"`
- runtime 头文件声明一致
- sema 注册的签名和 C runtime 签名一致

---

## 12. Runtime 实现要求

### 12.1 错误处理 helper

建议 runtime 内部加：

```c
static void rt_fatal(const char* message);
```

行为：

```c
fprintf(stderr, "Nova runtime error: %s\n", message);
exit(1);
```

### 12.2 malloc / realloc 检查

所有分配都要检查：

```c
if (!ptr) {
    rt_fatal("out of memory");
}
```

### 12.3 buffer handle

可以用一个全局表：

```c
static Buffer* buffers[MAX_BUFFERS];
```

`buf_new()` 返回一个 int index。

这不是最终设计，但足够支持 Nova v0。

---

## 13. 推荐测试

继续使用 codegen 的端到端测试方式。

每个测试流程：

```text
.nv
  -> nova_compile
  -> generated .c
  -> C compiler + runtime
  -> executable
  -> compare stdout
```

建议把这些文件放在：

```text
tests/codegen/positive/
```

---

## 13.1 `string_len_get.nv`

```nova
fn main() : void {
    let s : str = "nova";
    print_int(str_len(s));
    print_int(str_get(s, 0));
    print_int(str_get(s, 3));
    return;
}
```

`.out`:

```text
4
110
97
```

---

## 13.2 `string_slice.nv`

```nova
fn main() : void {
    let s : str = "hello nova";
    print_str(str_slice(s, 6, 10));
    return;
}
```

`.out`:

```text
nova
```

---

## 13.3 `string_predicates.nv`

```nova
fn main() : void {
    let s : str = "hello nova";
    if (str_starts_with(s, "hello")) {
        print_str("starts");
    } else {
        print_str("no");
    }

    if (str_contains(s, "nova")) {
        print_str("contains");
    } else {
        print_str("no");
    }
    return;
}
```

`.out`:

```text
starts
contains
```

---

## 13.4 `int_to_str_and_concat.nv`

```nova
fn main() : void {
    let s : str = str_concat("answer=", int_to_str(42));
    print_str(s);
    return;
}
```

`.out`:

```text
answer=42
```

---

## 13.5 `buffer_builder.nv`

```nova
fn main() : void {
    let b : int = buf_new();
    buf_push_str(b, "hello");
    buf_push_str(b, " ");
    buf_push_str(b, "nova");
    buf_push_int(b, 2026);
    print_str(buf_to_str(b));
    return;
}
```

`.out`:

```text
hello nova2026
```

---

## 13.6 `file_roundtrip.nv`

```nova
fn main() : void {
    write_file("nova_step6_tmp.txt", "hello file");
    let s : str = read_file("nova_step6_tmp.txt");
    print_str(s);
    return;
}
```

`.out`:

```text
hello file
```

---

## 14. CMake 测试添加

如果这些测试继续放在 `tests/codegen/positive/`，只需要在 CMake 里加：

```cmake
add_codegen_positive_test(string_len_get)
add_codegen_positive_test(string_slice)
add_codegen_positive_test(string_predicates)
add_codegen_positive_test(int_to_str_and_concat)
add_codegen_positive_test(buffer_builder)
add_codegen_positive_test(file_roundtrip)
```

---

## 15. 推荐实现顺序

### Step A：加固已有 runtime

先修：

- fopen 失败
- malloc 失败
- realloc 失败
- read/write 错误

### Step B：添加 string utilities

实现：

- `str_len`
- `str_get`
- `str_slice`
- `str_starts_with`
- `str_contains`
- `int_to_str`

### Step C：更新 sema builtin table

注册所有新增 runtime 函数。

### Step D：添加并运行 string tests

先让 string tests 过。

### Step E：实现 buffer API

实现：

- `buf_new`
- `buf_push_str`
- `buf_push_int`
- `buf_to_str`

### Step F：可选实现 string vector

如果你觉得 Step 6 还不够，可以加 `str_vec_*`。

---

## 16. 验收标准

### 合格

- Step 5 原有 codegen 测试继续通过
- 新增 string runtime 测试通过
- 文件读写不会崩溃
- runtime 错误有清晰输出

### 良好

- buffer builder 测试通过
- sema builtin 注册清晰
- 所有测试接入 CTest

### 优秀

- string vector API 可用
- runtime API 有文档
- runtime 内部错误处理统一

---

## 17. 完成标志

Step 6 完成的标志是：

> Nova 已经可以编写会读写文件、扫描字符串、构建字符串输出的程序。

这一步完成后，后续就可以开始考虑：

- 用 Nova 写小工具
- 用 Nova 写 tokenizer prototype
- 逐步向 self-hosting 方向推进
