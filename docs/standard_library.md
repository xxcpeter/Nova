# Nova Standard Library and Builtins

This document describes the currently supported Nova builtins and runtime-facing APIs.

Nova is still a small educational language. The functions listed here are the supported language surface for normal Nova programs and compiler tools. Runtime implementation details that are not listed here should not be treated as stable user-facing APIs.

---

## 1. Overview

Nova currently exposes a small set of builtins for:

```text
printing
strings
integers
files
buffers
command-line arguments
errors
typed vectors
```

Most ordinary builtins compile to direct C runtime calls with the same name. Typed vector operations are special compiler-known operations and are described separately.

---

## 2. Printing

### `print_int(value: int) : void`

Prints an integer value.

Example:

```nova
fn main() : void {
    print_int(42);
}
```

### `print_str(value: str) : void`

Prints a string value.

Example:

```nova
fn main() : void {
    print_str("hello");
}
```

---

## 3. Strings

Nova strings use the type:

```nova
str
```

String values are immutable from the language point of view.

---

### `str_eq(a: str, b: str) : bool`

Returns whether two strings are equal.

Example:

```nova
fn main() : void {
    if (str_eq("nova", "nova")) {
        print_str("same");
    }
}
```

---

### `str_concat(a: str, b: str) : str`

Returns a new string containing `a` followed by `b`.

Example:

```nova
fn main() : void {
    print_str(str_concat("hello, ", "world"));
}
```

Nova also supports string addition syntax:

```nova
fn main() : void {
    print_str("hello, " + "world");
}
```

For normal source code, prefer `+` for simple string concatenation.

---

### `str_len(s: str) : int`

Returns the length of a string.

Example:

```nova
fn main() : void {
    print_int(str_len("nova"));
}
```

---

### `str_get(s: str, index: int) : int`

Returns the character code at `index`.

Indexes are zero-based.

Example:

```nova
fn main() : void {
    print_int(str_get("abc", 0));
}
```

---

### `str_slice(s: str, start: int, end: int) : str`

Returns the substring from `start` up to, but not including, `end`.

Indexes are zero-based.

Example:

```nova
fn main() : void {
    print_str(str_slice("abcdef", 1, 4));
}
```

Expected output:

```text
bcd
```

---

### `str_starts_with(s: str, prefix: str) : bool`

Returns whether `s` starts with `prefix`.

Example:

```nova
fn main() : void {
    if (str_starts_with("nova.nv", "nova")) {
        print_str("yes");
    }
}
```

---

### `str_contains(s: str, needle: str) : bool`

Returns whether `s` contains `needle`.

Example:

```nova
fn main() : void {
    if (str_contains("nova compiler", "compile")) {
        print_str("yes");
    }
}
```

---

### `str_ends_with(s: str, suffix: str) : bool`

Returns whether `s` ends with `suffix`.

Example:

```nova
fn main() : void {
    if (str_ends_with("main.nv", ".nv")) {
        print_str("nova file");
    }
}
```

---

## 4. Integers

Nova integers use the type:

```nova
int
```

---

### `int_to_str(value: int) : str`

Converts an integer to a string.

Example:

```nova
fn main() : void {
    print_str("value=" + int_to_str(123));
}
```

---

## 5. File I/O

### `read_file(path: str) : str`

Reads the entire contents of a file and returns it as a string.

Example:

```nova
fn main() : void {
    let text : str = read_file("input.txt");
    print_str(text);
}
```

---

### `write_file(path: str, content: str) : void`

Writes `content` to `path`.

Example:

```nova
fn main() : void {
    write_file("out.txt", "hello");
}
```

---

## 6. Buffers

Buffers are mutable string builders. They are useful for efficient incremental output construction, especially inside loops or compiler tools.

For small string expressions, prefer `+`:

```nova
let s : str = "hello, " + name;
```

For large generated output, prefer `buf_*`:

```nova
let out : int = buf_new();
buf_push_str(out, "hello");
buf_push_str(out, " world");
let text : str = buf_to_str(out);
```

Buffers are represented as integer handles.

---

### `buf_new() : int`

Creates a new buffer and returns its handle.

---

### `buf_push_str(buf: int, value: str) : void`

Appends a string to the buffer.

---

### `buf_push_int(buf: int, value: int) : void`

Appends an integer to the buffer.

This is equivalent in meaning to appending `int_to_str(value)`, but avoids an explicit conversion at the call site.

---

### `buf_to_str(buf: int) : str`

Returns the accumulated buffer contents as a string.

Example:

```nova
fn main() : void {
    let out : int = buf_new();
    buf_push_str(out, "x=");
    buf_push_int(out, 42);
    print_str(buf_to_str(out));
}
```

Expected output:

```text
x=42
```

---

## 7. Command-line Arguments

### `arg_count() : int`

Returns the number of command-line arguments available to the program.

---

### `arg_get(index: int) : str`

Returns the command-line argument at `index`.

Example:

```nova
fn main() : void {
    let n : int = arg_count();
    if (n > 0) {
        print_str(arg_get(0));
    }
}
```

The exact indexing convention follows the current Nova runtime behavior.

---

## 8. Errors

### `nova_runtime_error(message: str) : void`

Stops execution with a runtime error message.

This function does not return.

Example:

```nova
fn fail() : str {
    nova_runtime_error("failed");
}
```

The compiler and checker may treat this as a no-return call for control-flow checking.

---

## 9. Typed Vectors

Nova supports typed vectors:

```nova
vec<int>
vec<str>
vec<MyStruct>
```

Vectors are language-level builtins known to the checker and code generator. They are not a fixed family of C runtime functions in `nova_runtime.h`.

The C backend may generate per-type helper structs and functions such as:

```text
NovaVec_int
NovaVec_str
NovaVec_Point
```

These generated names are compiler implementation details.

---

### `vec_new() : vec<T>`

Creates a new vector.

The element type is inferred from context.

Example:

```nova
fn main() : void {
    let xs : vec<int> = vec_new();
}
```

---

### `vec_push(xs: vec<T>, value: T) : void`

Appends `value` to `xs`.

Example:

```nova
fn main() : void {
    let xs : vec<int> = vec_new();
    vec_push(xs, 10);
    vec_push(xs, 20);
}
```

---

### `vec_get(xs: vec<T>, index: int) : T`

Returns the element at `index`.

Example:

```nova
fn main() : void {
    let xs : vec<int> = vec_new();
    vec_push(xs, 10);
    print_int(vec_get(xs, 0));
}
```

---

### `vec_set(xs: vec<T>, index: int, value: T) : void`

Sets the element at `index` to `value`.

Example:

```nova
fn main() : void {
    let xs : vec<int> = vec_new();
    vec_push(xs, 10);
    vec_set(xs, 0, 20);
    print_int(vec_get(xs, 0));
}
```

---

### `vec_len(xs: vec<T>) : int`

Returns the number of elements in `xs`.

Example:

```nova
fn main() : void {
    let xs : vec<int> = vec_new();
    vec_push(xs, 10);
    print_int(vec_len(xs));
}
```

---

## 10. Supported Builtin Summary

| Name | Signature | Category |
|---|---|---|
| `print_int` | `(int) -> void` | Printing |
| `print_str` | `(str) -> void` | Printing |
| `str_eq` | `(str, str) -> bool` | String |
| `str_concat` | `(str, str) -> str` | String |
| `str_len` | `(str) -> int` | String |
| `str_get` | `(str, int) -> int` | String |
| `str_slice` | `(str, int, int) -> str` | String |
| `str_starts_with` | `(str, str) -> bool` | String |
| `str_contains` | `(str, str) -> bool` | String |
| `str_ends_with` | `(str, str) -> bool` | String |
| `int_to_str` | `(int) -> str` | Integer/String |
| `read_file` | `(str) -> str` | File I/O |
| `write_file` | `(str, str) -> void` | File I/O |
| `buf_new` | `() -> int` | Buffer |
| `buf_push_str` | `(int, str) -> void` | Buffer |
| `buf_push_int` | `(int, int) -> void` | Buffer |
| `buf_to_str` | `(int) -> str` | Buffer |
| `arg_count` | `() -> int` | Arguments |
| `arg_get` | `(int) -> str` | Arguments |
| `nova_runtime_error` | `(str) -> void` | Error / no-return |
| `vec_new` | `() -> vec<T>` | Vector builtin |
| `vec_push` | `(vec<T>, T) -> void` | Vector builtin |
| `vec_get` | `(vec<T>, int) -> T` | Vector builtin |
| `vec_set` | `(vec<T>, int, T) -> void` | Vector builtin |
| `vec_len` | `(vec<T>) -> int` | Vector builtin |

---

## 11. Legacy and Removed APIs

### `str_vec_*`

The old `str_vec_*` runtime API has been removed from the supported language surface.

Use typed vectors instead:

```nova
let xs : vec<str> = vec_new();
vec_push(xs, "hello");
print_str(vec_get(xs, 0));
```

The supported vector API is:

```text
vec_new
vec_push
vec_get
vec_set
vec_len
```

---

## 12. Internal Runtime Details

The C runtime may contain helper functions or implementation details that are not documented here.

Only the APIs listed in this document should be treated as supported Nova builtins.

Compiler-generated C helper names, especially vector helper names, are not stable user-facing APIs.

---

## 13. Notes for Compiler Implementations

The C++ semantic analyzer, Nova checker, and Nova code generator should agree on:

```text
builtin names
argument counts
argument types
return types
no-return behavior
```

In particular:

```text
nova_runtime_error is no-return
str + str lowers to str_concat
vec_* operations are compiler-known vector operations
ordinary runtime builtins lower to direct C calls
```

When adding a new user-facing builtin, update all of:

```text
runtime/nova_runtime.h
runtime/nova_runtime.c
C++ semantic analyzer builtin handling
Nova checker builtin handling
Nova codegen type inference / call handling
this document
tests
```