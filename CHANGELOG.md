# Changelog

All notable changes to **ceedless** are documented here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

## [0.5.0] — 2026-05-31

### Added — framework
- `TEST_CASE(args...)` annotation + `RUN_TEST_CASE(name, label, args)` for
  parametric tests. The runner generator auto-emits one call per `TEST_CASE`.
- `TEST_PROPERTY(var, iters, body)` — property-based test loop.
- `TEST_ASSERT_GOLDEN_BYTES(label, buf, n)` — snapshot / golden-file
  comparison; `CEEDLESS_GOLDEN_UPDATE=1` to refresh.
- `mock_expect_with_matcher` / `MOCK_EXPECT_WITH_MATCHER` — user-supplied
  argument matchers (for ranges, partial matches, etc.).
- Deterministic shuffle via `ceedless_set_shuffle_seed` / `CEEDLESS_SEED`.
- TAP 13 output backend: `ceedless_set_tap_path` / `CEEDLESS_TAP`.
- ARM Cortex-M semihosting trace backend (`CEEDLESS_TRACE_SEMIHOST`).
- Memory assertion failures now show a ±8-byte hex window around the diff.

### Added — CLI
- `ceedless test --asan / --ubsan / --msan` — sanitizer flag passthrough.
- `ceedless test --shuffle [--seed N]` — reproducible random test order.
- `ceedless test --tap-dir DIR` — per-program TAP files.
- `ceedless fuzz <fn> -h <header>` — libFuzzer harness generator.
- `ceedless report DIR` — HTML aggregate over JUnit XMLs.
- Auto-runner now `#include`s the test source so `static` parametric
  functions resolve cleanly.

### Added — integrations & examples
- `examples/cortex_m/` — Cortex-M3 PIL example running under QEMU
  (`make qemu`). Same source compiles for host SIL and target PIL.
- `cmake/ceedless.cmake` + top-level `CMakeLists.txt` —
  `add_ceedless_test(<name> SOURCES … INCLUDE … DEFINES …)` integrates
  with CTest and emits JUnit XML via `CEEDLESS_JUNIT`.
- `library.json` — PlatformIO library manifest.
- `.vscode/tasks.json` + `extensions.json` — VS Code tasks for build,
  test, lint, coverage, watch, report.
- `.pre-commit-config.yaml` — pre-commit hooks template (gofmt, vet,
  large-file, line-ending checks; `make lint` on push).
- CI matrix gains a `cortex_m` job that installs `gcc-arm-none-eabi`
  and `qemu-system-arm` and runs the PIL example.

### Changed
- Self-test count: 33 → 42 (new `tests/test_extensions.c` exercises
  parametric, property, golden, PRNG, and memory-diff paths).

## [0.4.0] — 2026-05-29

### Added
- Go-based CLI (single static binary, no runtime dependencies).
- `watch` — re-run tests on file change.
- `bench` — list 10 slowest tests.
- `cov`   — gcov + single-file HTML coverage report.
- `mock`  — generate `MOCK_DEFINE(...)` stubs from a header.
- `doctor`— diagnose toolchain availability.
- `init`  — interactive project wizard (trace backend, C std).
- `--timing`, `--format tap`, `--report-dir`, `-j` parallel build.
- Multi-stage Dockerfile (golang builder → debian-slim runtime).
- `docs/CLI.md` — full CLI reference.

### Changed
- Replaced bash CLI with Go for portability and richer features.
- Renamed project from internal codename `emtest` to `ceedless`.

## [0.3.0] — 2026-05-29

### Added
- Bash CLI (zero runtime, POSIX shell only).
- Auto-runner generation: tests no longer declare `main()`.
- `TEST_SOURCE_FILE("...")` annotation for extra sources.
- Ceedling-parity `new`, `module`, `test`, `gcov`, `docker`, `clean`, `version`.
- Colored runner output (`CEEDLESS_NO_COLOR` to disable).
- Dockerfile + `make docker-test`.
- `docs/TUTORIAL.md` step-by-step walkthrough.

## [0.2.0] — 2026-05-28

### Added
- Unity-parity assertion macros (int/uint/hex/float/double/string/array/within/each_equal/bits).
- `setUp`/`tearDown`/`suiteSetUp`/`suiteTearDown` lifecycle hooks.
- CException-style `Try`/`Catch`/`Throw` with nested stack.
- CMock-style stateful function mocks (pure C macros, no codegen).
- Virtual peripherals: SPI, UART, GPIO, ADC.
- JUnit XML report output via `CEEDLESS_JUNIT=path`.

## [0.1.0] — 2026-05-28

### Added
- Hybrid host/MCU runner with platform-conditional assertions.
- Pluggable trace backend (host stdio, UART, RTT, ITM, in-memory buffer).
- Virtual I2C / generic register-bank peripheral.
- Sensor-driver example.

[Unreleased]: https://github.com/tevfik/ceedless/compare/v0.5.0...HEAD
[0.5.0]: https://github.com/tevfik/ceedless/releases/tag/v0.5.0
[0.4.0]: https://github.com/tevfik/ceedless/releases/tag/v0.4.0
[0.3.0]: https://github.com/tevfik/ceedless/releases/tag/v0.3.0
[0.2.0]: https://github.com/tevfik/ceedless/releases/tag/v0.2.0
[0.1.0]: https://github.com/tevfik/ceedless/releases/tag/v0.1.0
