#!/usr/bin/env bash
set -euo pipefail

python3 -m json.tool package.json >/dev/null
python3 -m json.tool language-configuration.json >/dev/null
python3 -m json.tool syntaxes/nova.tmLanguage.json >/dev/null
python3 -m json.tool snippets/nova.code-snippets >/dev/null
python3 -m json.tool examples/tasks.json >/dev/null

echo "Nova VS Code extension files are valid JSON"
