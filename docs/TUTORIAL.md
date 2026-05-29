# ceedless — getting started

This walkthrough takes you from an empty directory to a passing test
suite, on the host and inside Docker.

## 1. Install

ceedless is a single bash script plus the C framework. There is no
runtime to install — only a POSIX shell and a C compiler.

```sh
git clone https://example.com/ceedless /opt/ceedless
sudo ln -s /opt/ceedless/bin/ceedless /usr/local/bin/ceedless
ceedless version
```

## 2. Create a project

```sh
ceedless new blinky
cd blinky
```

Layout:

```
blinky/
├── project.yml      # configuration (paths, defines, cflags)
├── src/             # production code
├── include/         # public headers
├── test/            # tests
└── build/           # generated; gitignored
```

## 3. Add your first module

```sh
ceedless module led
```

This creates:

- `include/led.h`
- `src/led.c`           — a placeholder `led_add(a,b)`
- `test/test_led.c`     — two failing-by-design tests, **no `main()`**

`ceedless` auto-generates the test runner: it scans your `test_*.c` for
`void test_<name>(void)` and synthesizes a `main()` behind the scenes.

## 4. Run

```sh
ceedless test
```

Filter and parallelize:

```sh
ceedless test -p led          # only test programs whose name contains 'led'
ceedless test -j 4            # build 4 programs at once
ceedless test --junit-dir out # write JUnit XML for CI
```

## 5. Coverage

```sh
ceedless gcov
```

Compiles with `--coverage`, runs the tests, and emits `.gcov` files
next to the objects. Use `gcovr -r .` for an HTML report.

## 6. Stateful mocks (CMock-style)

```c
#include "ceedless/mock.h"
MOCK_DEFINE(i2c_write, int, (uint8_t addr, const uint8_t *p, size_t n))

void test_driver_writes_config(void) {
    EXPECT_AND_RETURN(i2c_write, 0)
        .with(uint8_t, 0x42)
        .with_mem(p_buf, expected, 4)
        .with(size_t, 4);
    driver_configure();
    MOCK_VERIFY(i2c_write);
}
```

## 7. Exceptions (CException-style)

```c
volatile EM_EXC e;
Try { do_thing(); } Catch (e) { TEST_ASSERT_EQUAL_INT(EIO, e); }
```

## 8. Extra source files

If a test needs a source file the auto-discovery won't find (it only
matches `src/<modulename>.c`), annotate it in the test:

```c
/* TEST_SOURCE_FILE("src/helpers/crc.c") */
```

## 9. Hardware-in-the-loop

`CEEDLESS_TARGET` is defined when you build for an MCU. Use the
`TEST_ASSERT_HW(...)` / `TEST_ASSERT_HOST(...)` variants to guard
host-only or target-only assertions, and configure a trace backend
(`CEEDLESS_TRACE_UART`, `CEEDLESS_TRACE_RTT`, `CEEDLESS_TRACE_ITM`, or
`CEEDLESS_TRACE_BUFFER`) instead of host stdio.

## 10. Docker

No tooling on the dev box? Use the container.

```sh
ceedless docker test           # builds ceedless:latest once, then runs `test`
ceedless docker gcov
ceedless docker test -p led -j 4
```

The image carries gcc, make, gcovr, and the framework. Your working
directory is mounted at `/work`.

## 11. CI example

```yaml
# .github/workflows/test.yml
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: ceedless docker test --junit-dir reports
      - uses: actions/upload-artifact@v4
        with: { name: junit, path: reports }
```
