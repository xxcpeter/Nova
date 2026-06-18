# Nova Project — Step 26
## 题目：VS Code Developer Tooling

---

## 1. 当前起点

Step 25 已经完成：

```text
self-hosted codegen workflow exists
stage0 -> stage1 -> stage2 nova_codegen works
representative program matrix passes
ctest passes
self-hosting docs updated
```

现在 Phase 2 进入 developer tooling step。

当前仓库已经有一个轻量 VS Code extension：

```text
tools/vscode-nova-syntax/package.json
tools/vscode-nova-syntax/language-configuration.json
tools/vscode-nova-syntax/syntaxes/nova.tmLanguage.json
```

当前 extension 已经支持：

```text
.nv language registration
TextMate syntax highlighting
line/block comments
strings
imports
keywords
primitive types
vec type keyword
runtime/vector builtins
function / struct / enum / let declarations
enum member access
numbers
operators
punctuation
basic bracket/comment/quote behavior
basic indentation rules
```

Step 26 的目标是在不引入完整 LSP 的前提下，把这个 extension 整理成一个可安装、可测试、可维护的 Nova developer tooling 包。

---

## 2. 本次作业目标

Step 26 的目标是：

> **完善 Nova VS Code extension，让 Nova 源码编辑体验足够稳定，并提供基础 tasks / problem matcher / snippets / 文档。**

核心成果：

```text
syntax highlighting polish
language configuration polish
snippets
tasks / problem matcher
extension README
sample file for manual testing
optional packaging script
```

完成后，用户应该可以：

```text
打开 .nv 文件获得稳定 syntax highlighting
使用 snippets 快速写 fn / if / while / struct / enum / import
通过 VS Code task 运行 Nova build/test/self-host 命令
看到 compiler diagnostics 被 problem matcher 捕获
阅读 extension README 完成本地安装/调试
```

---

## 3. 本阶段定位

Step 26 是 editor tooling，不是 compiler feature step。

它应该解决：

```text
syntax highlighting 覆盖不完整或容易误高亮
extension 缺少 snippets
extension 缺少 tasks/problem matcher
缺少 extension README
缺少 manual highlight test sample
开发者不知道如何安装/调试 extension
```

它不要求解决：

```text
language server
hover
go to definition
rename symbol
semantic tokens
live diagnostics daemon
formatting provider
completion provider backed by parser
workspace indexing
```

这些可以以后做真正 LSP 时再实现。

---

## 4. 本阶段不做什么

Step 26 不做：

```text
LSP server
real-time parser diagnostics
semantic type-aware highlighting
cross-file symbol navigation
code actions
formatter
debug adapter
VS Code Marketplace publishing
```

可以准备 extension 结构，但不需要发布到 Marketplace。

---

# Part A：Current Extension Review

---

## 5. 当前 extension 文件

当前 extension 应该位于：

```text
tools/vscode-nova-syntax/
  package.json
  language-configuration.json
  syntaxes/
    nova.tmLanguage.json
```

如果 `syntaxes/` 目录当前还没有在本地结构中创建，需要确保：

```text
package.json 中的 grammar path 与实际路径一致
```

也就是：

```json
"path": "./syntaxes/nova.tmLanguage.json"
```

---

## 6. package.json 要求

`package.json` 至少应该包含：

```json
{
  "name": "vscode-nova-syntax",
  "displayName": "Nova Syntax",
  "description": "Syntax highlighting and basic editor support for the Nova programming language.",
  "version": "0.1.0",
  "publisher": "nova",
  "engines": {
    "vscode": "^1.80.0"
  },
  "categories": [
    "Programming Languages"
  ],
  "contributes": {
    "languages": [
      {
        "id": "nova",
        "aliases": ["Nova", "nova"],
        "extensions": [".nv"],
        "configuration": "./language-configuration.json"
      }
    ],
    "grammars": [
      {
        "language": "nova",
        "scopeName": "source.nova",
        "path": "./syntaxes/nova.tmLanguage.json"
      }
    ]
  }
}
```

Step 26 会扩展 `contributes`，加入：

```text
snippets
problemMatchers
taskDefinitions or documented tasks
```

如果 taskDefinitions 太重，可以先只提供 `.vscode/tasks.json` 示例。

---

## 7. language-configuration.json 要求

当前 language configuration 已经有：

```text
line comment: //
block comment: /* */
brackets: {}, [], (), <>
auto closing pairs
surrounding pairs
basic indentation rules
```

Step 26 建议确认：

```text
" not auto-close inside string
{ } indentation works
< > 是否真的需要作为 bracket pair
comments toggle works
```

注意：`<` and `>` 既是 comparison operators，又是 `vec<int>` type punctuation。作为 bracket pair 可以接受，但如果编辑体验不好，可以移除 `< >` 的 bracket pair，只保留 syntax highlighting。

---

# Part B：Syntax Highlighting Polish

---

## 8. TextMate grammar 当前覆盖

当前 grammar 已经覆盖：

```text
comments
strings
imports
keywords
types
builtins
declarations
enum member access
numbers
operators
punctuation
```

Step 26 的目标是 polish，而不是重写。

---

## 9. 需要检查的 highlighting cases

使用 `test.nv` 或新增 sample 覆盖：

```nova
import "../lib/tokenizer.nv";

enum Color {
    Red;
    Green;
}

struct Point {
    x: int;
    y: int;
}

fn make_point(x: int, y: int) : Point {
    return Point { x: x, y: y };
}

fn main() : void {
    let c : Color = Color.Red;
    let p : Point = make_point(1, 2);
    let xs : vec<int> = vec_new();

    vec_push(xs, p.x);

    if (c == Color.Red && vec_len(xs) > 0) {
        print_int(vec_get(xs, 0));
        print_str("color=" + "red");
    }

    return;
}
```

检查：

```text
import keyword 和 path
struct / enum / fn / let declarations
primitive types
Named types
vec<int>
enum member Color.Red
struct literal Point { ... }
field access p.x
runtime builtins
vector builtins
string plus
operators
comments
```

---

## 10. Highlighting improvement suggestions

### 10.1 Struct literal vs type name

当前 grammar 可能把所有 `UpperCamelCase` 都高亮为 type，这可以接受。

但要确保：

```nova
Point { x: x, y: y }
```

至少 `Point` 是 type-ish highlight，`x` fields 不被误认为 declarations。

---

### 10.2 Function calls

可以新增 function call pattern：

```json
{
  "name": "entity.name.function.call.nova",
  "match": "\\b([A-Za-z_][A-Za-z0-9_]*)\\s*(?=\\()",
  "captures": {
    "1": {
      "name": "entity.name.function.call.nova"
    }
  }
}
```

但注意 ordering：

```text
builtins 应该先匹配
function declarations 应该先匹配
function calls 再匹配
```

否则 `fn main` 里的 `main` 或 builtins 可能被重复/误分类。

---

### 10.3 Struct fields

可以新增 field access pattern：

```json
{
  "name": "meta.field-access.nova",
  "match": "(\\.)([A-Za-z_][A-Za-z0-9_]*)\\b",
  "captures": {
    "1": { "name": "punctuation.accessor.dot.nova" },
    "2": { "name": "variable.other.member.nova" }
  }
}
```

但要注意和 enum member access 的 ordering：

```text
enumMembers 应该先于 generic field access
```

这样 `Color.Red` 可以保持 enum member highlight。

---

### 10.4 Builtin list 同步

Step 24 已经整理了 standard library。grammar 中 builtins 应该与 docs/standard_library.md 对齐：

```text
print_int
print_str
str_eq
str_concat
str_len
str_get
str_slice
str_starts_with
str_contains
str_ends_with
int_to_str
read_file
write_file
buf_new
buf_push_str
buf_push_int
buf_to_str
arg_count
arg_get
nova_runtime_error
vec_new
vec_push
vec_get
vec_set
vec_len
```

`str_vec_*` 不应该再出现在 grammar builtin list 中。

---

### 10.5 Boolean and constants

确保：

```text
true
false
```

仍然高亮为 boolean constant。

---

## 11. Grammar validation

TextMate grammar 是 JSON 文件。至少做：

```bash
python3 -m json.tool tools/vscode-nova-syntax/syntaxes/nova.tmLanguage.json >/dev/null
python3 -m json.tool tools/vscode-nova-syntax/package.json >/dev/null
python3 -m json.tool tools/vscode-nova-syntax/language-configuration.json >/dev/null
```

如果项目没有 Python requirement，也可以用：

```bash
node -e "JSON.parse(require('fs').readFileSync('tools/vscode-nova-syntax/package.json', 'utf8'))"
```

---

# Part C：Snippets

---

## 12. Add snippets file

新增：

```text
tools/vscode-nova-syntax/snippets/nova.code-snippets
```

并在 `package.json` 中贡献：

```json
"snippets": [
  {
    "language": "nova",
    "path": "./snippets/nova.code-snippets"
  }
]
```

---

## 13. Required snippets

至少提供：

```text
fn
main
if
ifelse
while
let
struct
enum
import
vec
```

示例：

```json
{
  "Function": {
    "prefix": "fn",
    "body": [
      "fn ${1:name}(${2}) : ${3:void} {",
      "    ${0:return;}",
      "}"
    ],
    "description": "Nova function declaration"
  },
  "Main function": {
    "prefix": "main",
    "body": [
      "fn main() : void {",
      "    ${0:return;}",
      "}"
    ],
    "description": "Nova main function"
  },
  "Import": {
    "prefix": "import",
    "body": [
      "import \"${1:path}.nv\";"
    ],
    "description": "Nova import directive"
  }
}
```

---

# Part D：Tasks and Problem Matcher

---

## 14. Why tasks/problem matcher

Nova diagnostics now use source-aware format like:

```text
path/to/file.nv:line:column: ErrorKind: message
```

This means VS Code can detect errors using a problem matcher.

---

## 15. Problem matcher pattern

Add a problem matcher to `package.json` or provide it in `.vscode/tasks.json` docs.

Recommended regex:

```json
{
  "owner": "nova",
  "fileLocation": ["relative", "${workspaceFolder}"],
  "pattern": {
    "regexp": "^(.*\\.nv):(\\d+):(\\d+):\\s+([A-Za-z]+Error):\\s+(.*)$",
    "file": 1,
    "line": 2,
    "column": 3,
    "severity": 4,
    "message": 5
  }
}
```

VS Code severity only recognizes values like `error` / `warning`, so `LexerError` may not map directly as severity. If that becomes an issue, use:

```json
"severity": "error"
```

or omit severity capture and set owner/pattern only.

---

## 16. Recommended task examples

Create documentation or optional sample:

```text
tools/vscode-nova-syntax/examples/tasks.json
```

Tasks:

```text
Nova: Build
Nova: Test
Nova: Self-host
Nova: Check current file
Nova: Compile current file
```

Example:

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "Nova: Test",
      "type": "shell",
      "command": "ctest --test-dir build -j8 --output-on-failure",
      "group": "test",
      "problemMatcher": "$nova"
    },
    {
      "label": "Nova: Self-host",
      "type": "shell",
      "command": "scripts/self_host.sh",
      "group": "build",
      "problemMatcher": "$nova"
    }
  ]
}
```

If you contribute problem matcher from extension package, users can reference:

```json
"problemMatcher": "$nova"
```

---

## 17. Check current file task

Optional task:

```json
{
  "label": "Nova: Check current file",
  "type": "shell",
  "command": "build/nova_sema ${file}",
  "problemMatcher": "$nova",
  "group": "build"
}
```

If `nova_frontend check` is preferred:

```json
{
  "label": "Nova: Frontend check current file",
  "type": "shell",
  "command": "build/nova_frontend check ${file} /tmp/nova_check.out",
  "problemMatcher": "$nova",
  "group": "build"
}
```

Only include commands that actually match current project binaries.

---

# Part E：Extension README and Manual Test

---

## 18. Extension README

Add:

```text
tools/vscode-nova-syntax/README.md
```

Content should include:

```text
What this extension provides
What it does not provide
Local install / development instructions
How to open extension development host
How to use snippets
How to configure tasks
Known limitations
```

Suggested wording:

```markdown
# Nova Syntax for VS Code

This extension provides syntax highlighting and basic editor support for the Nova programming language.

## Features

- `.nv` file association
- TextMate syntax highlighting
- Comment toggling
- Bracket and quote auto-closing
- Basic indentation rules
- Snippets
- Optional problem matcher for Nova diagnostics

## Not included

This extension does not provide a language server, hover, go-to-definition, or live semantic diagnostics.
```

---

## 19. Manual highlight sample

Keep or add:

```text
tools/vscode-nova-syntax/test.nv
```

This file should exercise:

```text
import
struct
enum
function
let
typed vec
field access
enum member access
runtime builtin
vector builtin
string plus
if / while / return
comments
```

The uploaded `test.nv` is a good baseline. Expand it only if highlighting cases are missing.

---

# Part F：Optional Packaging

---

## 20. Packaging is optional

Do not block Step 26 on Marketplace publishing.

Optional local packaging:

```bash
npm install -g @vscode/vsce
cd tools/vscode-nova-syntax
vsce package
```

This produces:

```text
vscode-nova-syntax-0.1.0.vsix
```

This is optional.

---

## 21. Local development instructions

Recommended dev workflow:

```text
1. Open tools/vscode-nova-syntax in VS Code
2. Press F5 to launch Extension Development Host
3. Open test.nv
4. Run Developer: Inspect Editor Tokens and Scopes
5. Verify expected scopes
```

No need to publish.

---

# Part G：Tests / Validation

---

## 22. Required validation

At minimum:

```bash
python3 -m json.tool tools/vscode-nova-syntax/package.json >/dev/null
python3 -m json.tool tools/vscode-nova-syntax/language-configuration.json >/dev/null
python3 -m json.tool tools/vscode-nova-syntax/syntaxes/nova.tmLanguage.json >/dev/null
python3 -m json.tool tools/vscode-nova-syntax/snippets/nova.code-snippets >/dev/null
```

Manual validation:

```text
open test.nv in Extension Development Host
inspect tokens/scopes
verify snippets expand
verify task examples are documented
```

---

## 23. Optional repo-level test script

Add:

```text
tools/vscode-nova-syntax/scripts/validate.sh
```

Example:

```bash
#!/usr/bin/env bash
set -euo pipefail

python3 -m json.tool package.json >/dev/null
python3 -m json.tool language-configuration.json >/dev/null
python3 -m json.tool syntaxes/nova.tmLanguage.json >/dev/null
python3 -m json.tool snippets/nova.code-snippets >/dev/null

echo "Nova VS Code extension files are valid JSON"
```

---

# Part H：Recommended Implementation Order

---

## 24. Step A：Validate current extension

Run JSON validation on current files.

Fix any invalid JSON or path mismatch.

---

## 25. Step B：Polish grammar

Update `nova.tmLanguage.json` if needed:

```text
sync builtin list with standard_library.md
add function call highlighting if desired
add field access highlighting if desired
ensure enum member access still wins over generic field access
```

---

## 26. Step C：Add snippets

Create:

```text
snippets/nova.code-snippets
```

Update package manifest.

---

## 27. Step D：Add problem matcher / tasks docs

Either:

```text
contribute problem matcher in package.json
```

or:

```text
provide examples/tasks.json and document it
```

Best result: do both.

---

## 28. Step E：Add README

Create:

```text
tools/vscode-nova-syntax/README.md
```

Document features and limitations.

---

## 29. Step F：Manual Extension Development Host test

Use `test.nv`.

Check:

```text
highlighting
snippets
task/problem matcher docs
```

---

## 30. Step G：Docs update

Update top-level docs or README to mention:

```text
tools/vscode-nova-syntax
```

Example:

```markdown
## Editor support

A lightweight VS Code syntax extension is available in `tools/vscode-nova-syntax`.
```

---

# Part I：验收标准

---

## 31. 合格

```text
package.json valid
language-configuration.json valid
nova.tmLanguage.json valid
.nv files open as Nova
syntax highlighting covers current language basics
snippets file exists and is contributed
extension README exists
manual test.nv exists
```

---

## 32. 良好

```text
builtin highlighting matches docs/standard_library.md
field access and enum member access highlighting are stable
problem matcher exists for file:line:column diagnostics
tasks examples exist for build/test/self-host/check current file
validate script exists
README documents local Extension Development Host workflow
```

---

## 33. 优秀

```text
snippets cover common Nova constructs
problem matcher works with current Nova diagnostics
manual scope inspection confirms expected scopes
extension can be packaged locally with vsce
project README links editor support docs
```

---

## 34. 完成标志

Step 26 完成时，应该可以说：

```text
Nova has a lightweight but usable VS Code developer tooling package with syntax highlighting, snippets, task/problem-matcher support, and clear local setup documentation.
```

至少这些成立：

```text
VS Code recognizes .nv files
syntax highlighting matches current Nova language surface
snippets improve editing speed
Nova diagnostics can be captured by a problem matcher
extension docs explain features and limitations
```

---

## 35. 后续关联

Step 27 将作为 Phase 2 self-hosting milestone：

```text
final self-hosting status summary
demo commands
README / bootstrap docs cleanup
known limitations
release-style validation
```

Step 26 不阻塞 compiler semantics；它只是让项目更可用、更展示友好。
