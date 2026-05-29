# Changelog

All notable changes to **ceedless** are documented here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

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

[Unreleased]: https://github.com/tevfik/ceedless/compare/v0.4.0...HEAD
[0.4.0]: https://github.com/tevfik/ceedless/releases/tag/v0.4.0
[0.3.0]: https://github.com/tevfik/ceedless/releases/tag/v0.3.0
[0.2.0]: https://github.com/tevfik/ceedless/releases/tag/v0.2.0
[0.1.0]: https://github.com/tevfik/ceedless/releases/tag/v0.1.0
