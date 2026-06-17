# Nova Syntax for VS Code

This extension provides lightweight editor support for the Nova programming language.

## Features

- `.nv` file association
- TextMate syntax highlighting
- Comment toggling for `//` and `/* ... */`
- Bracket and quote auto-closing
- Basic indentation rules
- Snippets for common Nova constructs
- Optional problem matcher for Nova diagnostics
- Example VS Code tasks for build, test, self-host, check, and compile

## Not included

This extension does not provide a language server. It does not include hover, go-to-definition, rename, formatting, semantic diagnostics, or semantic tokens.

## Local development

Open this folder in VS Code:

```text
tools/vscode-nova-syntax
```

Then press `F5` to launch an Extension Development Host.

Open `test.nv` in the Extension Development Host and use:

```text
Developer: Inspect Editor Tokens and Scopes
```

to check highlighting scopes.

## Snippets

Available snippet prefixes include:

```text
fn
main
let
if
ifelse
while
struct
enum
import
vec
prints
printi
```

## Problem matcher

Nova diagnostics use this format:

```text
path/to/file.nv:line:column: ErrorKind: message
```

The extension contributes a `$nova` problem matcher for that format.

Example `.vscode/tasks.json` is available at:

```text
examples/tasks.json
```

Copy it into your repository root as:

```text
.vscode/tasks.json
```

## Validation

Run:

```bash
scripts/validate.sh
```

This checks that the extension JSON files are valid.

## Packaging

Optional local packaging:

```bash
npm install -g @vscode/vsce
vsce package
```

Publishing to the VS Code Marketplace is not part of the current Nova project milestone.
