# ceedless CLI

The CLI is a single static Go binary. No runtime, no dependencies beyond a C
compiler in your `$PATH`.

## Install

```sh
# from source (Go 1.22+)
go install github.com/tevfik/ceedless/cli@latest

# or in this repo
make cli           # produces ./bin/ceedless
```

The binary self-locates the framework via, in order:

1. `$CEEDLESS_HOME` (must contain `include/ceedless/`)
2. the directory two levels above the binary (works for `./bin/ceedless`)
3. `/opt/ceedless` (Docker default)

## Commands

### Project lifecycle

| Command | Purpose |
|---------|---------|
| `ceedless new <path>` | Scaffold a project (project.yml, src/, test/, build/) |
| `ceedless init` | Interactive wizard (name, trace backend, C std) |
| `ceedless module <name>` | Generate `src/<name>.{c,h}` + `test/test_<name>.c` |
| `ceedless clean` | Remove `build/` |
| `ceedless version` | Print version |

### Building & running

```
ceedless test [options] [pattern]
  -p, --pattern PAT      substring filter on test filename
  -j, --jobs N           parallel build jobs (default: NumCPU)
  -v, --verbose          echo compiler command lines
      --gcov             build with --coverage
      --junit-dir DIR    write one JUnit XML per test program to DIR
      --tap-dir DIR      write one TAP file per test program to DIR
      --target T         define CEEDLESS_TARGET=T (hybrid host/MCU builds)
      --timing           print per-test program wall time
      --format text|tap  report format (TAP at build/report.tap)
      --report-dir DIR   collect all reports under DIR
      --asan             link with AddressSanitizer
      --ubsan            link with UndefinedBehaviorSanitizer
      --msan             link with MemorySanitizer (clang only)
      --shuffle          randomize test order
      --seed N           PRNG seed (non-zero implies --shuffle)
```

| Command | Shortcut for |
|---------|--------------|
| `ceedless gcov [...]` | `test --gcov ...` |

### Observability & dev loop

| Command | What it does |
|---------|--------------|
| `ceedless watch [test-args]` | Re-run tests on .c/.h change (polling, dep-free) |
| `ceedless bench` | Run tests then print 10 slowest with timing |
| `ceedless cov` | Run tests with gcov, then write `build/coverage.html` |
| `ceedless mock <header.h>` | Print `MOCK_DEFINE(...)` stubs for every function declared |
| `ceedless fuzz <fn> -h <hdr>` | Generate + run a libFuzzer harness (needs clang) |
| `ceedless report DIR` | Aggregate JUnit XMLs into `DIR/index.html` |
| `ceedless doctor` | Check toolchain (gcc, gcov, gcovr, docker, qemu, arm-none-eabi) |

### Containers

| Command | What it does |
|---------|--------------|
| `ceedless docker <subcmd>` | Pull/build `ceedless:latest` and run subcmd inside, mounting CWD at `/work` |
| `make docker` | Build the image locally (multi-stage: golang → debian-slim) |
| `make docker-test` | Run the framework self-tests inside the image |

## Environment variables

| Var | Purpose |
|-----|---------|
| `CEEDLESS_HOME` | Override framework location |
| `CEEDLESS_JUNIT` | Path to write a single JUnit XML (consumed by test binaries) |
| `CEEDLESS_TAP` | Path to write a single TAP report (consumed by test binaries) |
| `CEEDLESS_SEED` | xorshift32 seed; non-zero enables deterministic shuffle |
| `CEEDLESS_GOLDEN_UPDATE` | Set to `1` to refresh all `TEST_ASSERT_GOLDEN_BYTES` files |
| `CEEDLESS_NO_COLOR` / `NO_COLOR` | Disable ANSI colors |
| `CC` | C compiler (default `gcc`; use `clang` for `fuzz`/`msan`) |

## project.yml

```yaml
project:
  name: my_fw
paths:
  src:     [src]
  include: [include, src]
  test:    [test]
defines: ["HW_VARIANT=2", "CEEDLESS_TRACE_HOST"]
cflags:  ["-std=c11", "-Wall", "-Wextra", "-O0", "-g"]
files:
  extra: [src/util/helper.c]
```

All keys are optional. A minimal subset of YAML is parsed by the CLI itself
(no Go YAML dependency).

## Auto-generated test runners

You do **not** write `main()` in test files. The CLI scans every `test_*.c`
for `void test_<name>(void)` and synthesizes a `build/<name>_runner.c` that
calls `ceedless_begin`, every `RUN_TEST(...)`, then `ceedless_end`.

Need additional sources beyond `src/<modulename>.c`? Annotate the test:

```c
/* TEST_SOURCE_FILE("src/util/crc.c") */
```
