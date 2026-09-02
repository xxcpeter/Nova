# Security Policy

Nova is an educational compiler and runtime prototype. Security reports are welcome, especially when they include a minimal reproducer and clearly distinguish a compiler bug from a documented trust assumption.

## Supported versions

Security fixes are developed against the latest revision of `master`. Historical milestone tags are retained for reproducibility but do not receive backported fixes.

| Version | Supported |
| --- | --- |
| `master` | Yes |
| Historical tags | No |

## Reporting a vulnerability

Please use **Report a vulnerability** in the repository's GitHub Security tab to submit details privately. If private reporting is not available, contact the maintainer through the [GitHub profile](https://github.com/xxcpeter) to establish a private channel. Do not include exploit details in a public issue.

Include, when possible:

- the affected commit and component;
- a minimal Nova source file or command that reproduces the issue;
- expected and observed behavior;
- security impact and realistic preconditions;
- relevant platform/compiler information;
- a proposed mitigation, if you have one.

You should receive an acknowledgement within seven days. The maintainer will coordinate validation, remediation, and disclosure timing with the reporter.

## Security-relevant surfaces

The most relevant review areas currently include:

- import expansion, path normalization, and file handling;
- lexer/parser/checker behavior on malformed or adversarial source;
- C code generation, including string/identifier escaping and generated helper names;
- memory and integer safety in generated C and `runtime/`;
- process and filesystem builtins;
- consistency across the C++ seed compiler and Nova-written stages;
- bootstrap integrity across stage0, stage1, and stage2.

## Current trust boundaries

Nova is not a sandbox. Compiling generated C and running the resulting native program can execute code with the current user's permissions.

The `run_command` runtime builtin intentionally delegates a string to the host shell and is restricted to trusted build tooling. Passing untrusted input into `run_command`, or constructing commands from untrusted paths or arguments, is outside the supported security boundary. File and directory helpers also operate with the current process's filesystem permissions.

The project has not received a comprehensive security audit and should not be used to compile or execute untrusted programs in a production environment without additional isolation.
