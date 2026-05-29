# Contributing to ceedless

Thanks for your interest! Contributions of every size are welcome —
bug reports, doc fixes, examples for new MCUs, peripheral models, or
brand-new features.

## Quick start

```sh
git clone https://github.com/tevfik/ceedless
cd ceedless
make all          # builds CLI, runs self-tests, example, CLI smoke
```

You need:

- A C11 compiler (`gcc` or `clang`)
- GNU `make`
- Go ≥ 1.22 (only to build the CLI)

Verify your environment with:

```sh
./bin/ceedless doctor
```

## Project layout

```
include/ceedless/   public C headers
src/                framework C sources (runner, mocks, peripherals, trace)
tests/              framework self-tests
examples/           reference users (sensor_driver/)
cli/                Go source of the `ceedless` CLI
docs/               CLI.md, TUTORIAL.md, …
```

## Coding conventions

**C code**

- C11, four-space indent, no tabs.
- All public API symbols are prefixed `ceedless_` / `CEEDLESS_` / `TEST_`.
- Headers carry an `SPDX-License-Identifier: MIT` line.
- No dynamic allocation (the framework targets MCUs). Use static caps
  configurable via macros like `CEEDLESS_MAX_TESTS`, `CEEDLESS_EXC_STACK`.
- Code must compile clean under `-Wall -Wextra -Wshadow -Wpedantic`.

**Go code**

- `gofmt` + `go vet` must be clean before opening a PR (`make lint`).
- Stdlib only — no third-party imports. The CLI must remain a single
  static binary with zero runtime dependencies.
- Each file starts with the SPDX/copyright header.

## Running the checks locally

```sh
make all          # full pipeline
make lint         # gofmt + go vet
```

To run a single test program:

```sh
./bin/ceedless test -p mock     # only tests matching "mock"
```

## Pull requests

1. Fork & branch off `main`.
2. Keep PRs focused — one feature or fix per PR.
3. Add or update tests in `tests/` covering your change.
4. Update `CHANGELOG.md` under `## [Unreleased]`.
5. Update `docs/CLI.md` or `README.md` if user-visible behavior changes.
6. Make sure `make all` is green and `make lint` produces no output.

## Reporting bugs

Please open an issue with:

- Output of `ceedless doctor` and `ceedless version`.
- A minimal reproduction (smaller is better than complete).
- Expected vs actual behavior.

## License

By contributing, you agree your contributions are licensed under the
[MIT License](LICENSE), the same as the rest of the project.
