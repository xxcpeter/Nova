# Nova Language Specification (v0)

## 1. Overview

Nova is a small statically typed language designed for a compiler bootstrapping project.

The goal of Nova v0 is not to be a general-purpose production language. Its purpose is to provide the smallest practical language subset sufficient to implement parts of the Nova compiler itself in later stages of the project.

Nova v0 is intentionally limited. It focuses on:

- simple C-like syntax
- explicit types
- top-level functions
- local variables
- control flow
- basic expressions
- a small runtime interface

Nova v0 does not aim to support advanced features such as:

- structs
- arrays as built-in language types
- generics
- classes
- modules
- closures
- type inference
- garbage collection
- optimizations
- operator overloading
- `match`
- `for`
- `break` / `continue`
- `+=`, `-=`, `++`, `--`

---

## 2. Lexical Rules

### 2.1 Whitespace

Whitespace may appear between tokens and is otherwise ignored, except inside string literals.

### 2.2 Comments

Nova supports two kinds of comments:

- line comments: `// ...`
- block comments: `/* ... */`

Block comments do not nest.

### 2.3 Identifiers

Identifiers consist of letters, digits, and underscores, and must not begin with a digit.

Examples:

- `x`
- `main`
- `repeat_str`
- `_tmp`

### 2.4 Keywords

Nova v0 reserves the following keywords:

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

### 2.5 Literals

Nova v0 supports the following literals:

- integer literals, e.g. `0`, `1`, `42`
- boolean literals: `true`, `false`
- string literals, e.g. `"hello"`

### 2.6 Punctuation

Nova uses the following punctuation tokens:

- `(` `)`
- `{` `}`
- `,`
- `;`
- `:`

### 2.7 Operators

Nova v0 supports the following operators:

- assignment: `=`
- arithmetic: `+`, `-`, `*`, `/`, `%`
- comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`
- logical: `&&`, `||`, `!`

All statements must end with `;`, except block statements introduced by `{ ... }`.

---

## 3. Grammar

The following grammar is an approximate description of Nova v0 syntax.

```ebnf
program        := function*

function       := "fn" IDENT "(" params? ")" ":" type block
params         := param ("," param)*
param          := IDENT ":" type

type           := "int" | "bool" | "str" | "void"

block          := "{" statement* "}"

statement      := let_stmt
                | assign_stmt
                | if_stmt
                | while_stmt
                | return_stmt
                | expr_stmt
                | block

let_stmt       := "let" IDENT ":" type "=" expression ";"
assign_stmt    := IDENT "=" expression ";"
if_stmt        := "if" "(" expression ")" statement ("else" statement)?
while_stmt     := "while" "(" expression ")" statement
return_stmt    := "return" expression? ";"
expr_stmt      := expression ";"

expression     := assignment
assignment     := IDENT "=" assignment
                | logical_or

logical_or     := logical_and ("||" logical_and)*
logical_and    := equality ("&&" equality)*
equality       := comparison (("==" | "!=") comparison)*
comparison     := additive (("<" | "<=" | ">" | ">=") additive)?
additive       := multiplicative (("+" | "-") multiplicative)*
multiplicative := unary (("*" | "/" | "%") unary)*
unary          := ("!" | "-") unary
                | primary

primary        := INT_LITERAL
                | STRING_LITERAL
                | "true"
                | "false"
                | IDENT
                | call
                | "(" expression ")"

call           := IDENT "(" arguments? ")"
arguments      := expression ("," expression)*
````

Notes:

* Assignment is an expression syntactically, but in practice Nova v0 only permits assignment where `IDENT = ...` is valid.
* Chained comparison such as `a < b < c` is not allowed.
* A comparison expression contains at most one comparison operator.

---

## 4. Operator Precedence and Associativity

Operators are listed from highest precedence to lowest.

1. unary: `!`, unary `-`

   * right-associative

2. multiplicative: `*`, `/`, `%`

   * left-associative

3. additive: `+`, `-`

   * left-associative

4. comparison: `<`, `<=`, `>`, `>=`

   * non-associative
   * chained comparison is forbidden

5. equality: `==`, `!=`

   * left-associative

6. logical AND: `&&`

   * left-associative

7. logical OR: `||`

   * left-associative

8. assignment: `=`

   * right-associative

---

## 5. Types and Static Semantics

### 5.1 Built-in Types

Nova v0 has four built-in types:

* `int`
* `bool`
* `str`
* `void`

### 5.2 Variables

Variables are introduced with:

```nova
let name : Type = expr;
```

Examples:

```nova
let x : int = 1;
let ok : bool = true;
let msg : str = "hello";
```

Variables may be reassigned using `=`:

```nova
x = x + 1;
```

Nova v0 does not distinguish mutable and immutable variables. All local variables are assignable.

### 5.3 Scoping Rules

* Each `{ ... }` block introduces a new lexical scope.
* A name declared in an inner scope may shadow a name from an outer scope.
* Two names in the same scope may not be identical.
* Function parameters belong to the function body scope.

### 5.4 Functions

* Functions may only be declared at top level.
* Function names must be unique within a program.
* Function overloading is not supported.
* Forward calls are allowed: a function may be called before its textual definition appears, as long as it is defined somewhere in the same program.
* Function arguments must match the declared parameter count and types exactly.

### 5.5 Type Rules

* `if` and `while` conditions must have type `bool`.
* Assignment requires the left-hand side and right-hand side to have exactly the same type.
* Function arguments must match parameter types exactly.
* `==` and `!=` require both operands to have the same type.
* `<`, `<=`, `>`, `>=` are only valid on `int`.
* `&&`, `||`, and `!` are only valid on `bool`.
* unary `-` is only valid on `int`.
* arithmetic operators `+`, `-`, `*`, `/`, `%` are only valid on `int`.

Nova v0 does not perform implicit conversions.

Examples of illegal code:

* `if (1) { ... }`
* `1 + true`
* `"a" + "b"`
* `x = "hello"` when `x` is `int`

### 5.6 Return Rules

* In a function of type `void`, `return;` is allowed and `return expr;` is forbidden.
* In a function of non-`void` type, `return expr;` is required and `return;` is forbidden.
* Every control-flow path in a non-`void` function must return a value of the declared return type.
* Every control-flow path in a `void` function must terminate legally; explicit `return;` is allowed.

### 5.7 Main Function

Nova v0 uses the following entry function signature:

```nova
fn main() : void { ... }
```

---

## 6. Runtime Interface

Nova v0 allows programs to call a small set of externally provided runtime functions.

These functions are not special syntax. They behave like normal function calls in the language, but their implementations are supplied by the target runtime (initially C runtime support code).

The initial runtime API is:

```nova
fn print_int(x: int) : void
fn print_str(s: str) : void
fn str_eq(a: str, b: str) : bool
fn str_concat(a: str, b: str) : str
fn read_file(path: str) : str
fn write_file(path: str, content: str) : void
```

Rationale:

* `print_*` provides basic output without adding special syntax to the language.
* `str_*` avoids embedding string implementation details into the compiler front end.
* file I/O is needed later for compiler bootstrapping tasks.

These functions may be treated as predefined top-level declarations by the compiler.

---

## 7. Error Model

Nova v0 compilers should report errors with at least the following information:

* error category (lexical, syntax, or semantic)
* source line number
* source column number if available
* a short human-readable message

Preferred style:

* identify the offending token or source span
* explain the expected form when possible

Examples:

* `Syntax error at line 4, column 12: expected ';' after expression`
* `Semantic error at line 7, column 8: undefined variable 'x'`
* `Semantic error at line 10, column 15: expected bool in if-condition, got int`

---

## 8. Non-goals for Nova v0

The following features are explicitly out of scope for Nova v0:

* `match`
* `for`
* `break`
* `continue`
* arrays as built-in types
* structs
* enums
* generics
* classes
* modules
* closures
* lambdas
* type inference
* implicit conversion
* operator overloading
* exceptions
* garbage collection
* optimization passes
* compound assignment (`+=`, `-=`, etc.)
* increment/decrement (`++`, `--`)

---

## 9. Design Decisions Summary

* Return types use `:` rather than `->`.
* The string type name is `str`.
* Top-level functions may be called before they are defined.
* Runtime support is provided through ordinary function calls rather than special syntax.
* Variables are assignable after declaration.
* Scopes are block-based.
* Shadowing is allowed across different scopes, but not within the same scope.
* Comparison chaining is forbidden.
* `void` and non-`void` return forms must match exactly.
* All control-flow paths in non-`void` functions must return.

---

## 10. Examples
All examples for testing are written in the 'examples' folder