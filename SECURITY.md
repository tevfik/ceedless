# Security Policy

## Supported Versions

ceedless is a test framework for hosted and embedded C development. It
is **not** intended for use in production runtime paths. Security
guarantees apply only to the CLI binary and to the framework's
on-development use.

| Version | Supported |
|---------|-----------|
| 0.4.x   | ✅ |
| < 0.4   | ❌ |

## Reporting a Vulnerability

If you believe you've found a security issue (e.g. shell injection in
the CLI, path traversal in `new` / `module`, mishandled untrusted
project.yml, etc.), please **do not open a public issue**.

Instead, contact the maintainer privately via GitHub Security
Advisories on the repository:

<https://github.com/tevfik/ceedless/security/advisories/new>

Please include:

1. A description of the issue and its impact.
2. Steps to reproduce or a proof-of-concept.
3. The version (`ceedless version`) and host OS.

You can expect an acknowledgment within one week. Coordinated
disclosure timelines will be agreed on a case-by-case basis.
