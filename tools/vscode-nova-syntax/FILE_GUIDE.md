# Nova VS Code Extension File Guide

This guide explains what each file in `tools/vscode-nova-syntax` does.

## package.json

The extension manifest. VS Code reads this file to know the extension name, `.nv` file association, grammar path, snippets, and the `$nova` problem matcher.

## language-configuration.json

Basic editing behavior: comments, bracket pairs, auto-closing pairs, indentation, and folding markers. This file does not parse Nova.

## syntaxes/nova.tmLanguage.json

Regex-based TextMate grammar for syntax highlighting. It is not type-aware.

## snippets/nova.code-snippets

Snippets for common Nova constructs.

## examples/tasks.json

Example VS Code tasks. Copy this to `.vscode/tasks.json` in the repo root.

## scripts/validate.sh

Checks that all JSON files parse successfully.

## README.md

User-facing extension documentation.

## test.nv

Manual syntax highlighting sample.
