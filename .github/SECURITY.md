<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# Security Policy

## Supported versions

CLOCK is currently an alpha project. Security and safety fixes are applied to the active development line; older alpha snapshots should be treated as unsupported once a newer snapshot supersedes them.

## Reporting

Do not publish suspected security or safety vulnerabilities as a public issue before they can be assessed. Use GitHub's private vulnerability reporting feature when it is enabled for the repository, or contact the maintainer through the private contact channel associated with the repository account.

Useful reports include:

- affected version/commit;
- exact build environment;
- reproducible steps;
- expected and observed behavior;
- whether the issue can affect gate outputs, Flash persistence, boot safety, or firmware update integrity.

## Scope

Relevant reports include software defects that can cause unsafe output state, persistence corruption, unintended firmware disclosure through release artifacts, malformed input handling, or simulator/build tooling that can produce materially misleading qualification results.

Electrical design compliance and general Eurorack installation safety are engineering topics rather than software-security vulnerabilities, but defects that bypass CLOCK's explicit boot/output safety guarantees are in scope.
