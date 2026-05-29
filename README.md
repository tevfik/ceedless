# ceedless — lightweight unit-test framework for embedded C

[![CI](https://github.com/tevfik/ceedless/actions/workflows/ci.yml/badge.svg)](https://github.com/tevfik/ceedless/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Go Reference](https://pkg.go.dev/badge/github.com/tevfik/ceedless/cli.svg)](https://pkg.go.dev/github.com/tevfik/ceedless/cli)

`ceedless` is a small, dependency-light unit-test framework for microcontroller
firmware. It provides assertions, function mocks, exception handling, and
virtual peripherals in pure C11 — no Ruby, no code generation, single static
binary CLI.

**Key features:**

- Unity-style assertion macros (full set: INT, HEX, FLOAT, STRING, MEMORY, ARRAY)
- Header-only function mocks (MOCK_DEFINE / MOCK_EXPECT_* / MOCK_VERIFY)
- Try / Catch / Throw exception handling
- Virtual peripherals: SPI, UART, GPIO, ADC (stateful, register-bank backed)
- On-target execution support (same source builds for host SIL and MCU PIL)
- Pluggable trace transport (ITM, RTT, UART, host-printf, ring-buffer)
- Auto test discovery + runner generation (`ceedless test`)
- gcov coverage, JUnit XML output, file-watch mode

Zero dynamic allocation. ~1500 LOC of portable C11.

---

## Quick start

```bash
# clone, then add bin/ to your PATH (or symlink ceedless into /usr/local/bin)
export PATH="$PWD/bin:$PATH"

ceedless new my_fw && cd my_fw
ceedless module sensor      # creates src/sensor.c, include/sensor.h, test/test_sensor.c
ceedless test               # discovers, builds, and runs every test_*.c
ceedless gcov               # same, with coverage report
ceedless test --junit-dir reports/   # also emits JUnit XML per test program
```

The framework also has its own self-tests:

```bash
make           # builds & runs self-tests, negative test, example, and CLI smoke
```

Output ends with:

```
=== result: 33/34 passed, 0 failed, 1 ignored ===
OK: negative test failed as expected
=== result: 3/3 passed, 0 failed, 0 ignored ===
ceedless: 1/1 test programs passed
```

---

## Layout

```
bin/ceedless                     CLI (single static Go binary)
include/ceedless/
    ceedless.h                   umbrella include
    hybrid_runner.h            assertions + lifecycle hooks
    virtual_peripheral.h       generic stateful peripheral primitive
    peripherals/vp_spi.h       SPI virtual device
    peripherals/vp_uart.h      UART virtual device
    peripherals/vp_gpio.h      GPIO virtual bank
    peripherals/vp_adc.h       ADC virtual device
    mock.h                     CMock-style function mocks
    exception.h                CException-style Try/Catch/Throw
    trace_port.h               pluggable trace front-end
    trace_buffer.h             optional in-memory trace inspection
    trace_config.h             compile-time backend selector
src/
    runner.c                   suite/test lifecycle + JUnit emitter
    mock.c                     mock framework
    exception.c                exception stack
    virtual_peripheral.c       I2C-style device + register-bank base
    peripherals/vp_*.c
    trace/trace_{port,host,uart,rtt,itm,buffer}.c
tests/                         framework self-tests
examples/sensor_driver/        end-to-end test using mocks + virtual I2C
Makefile
```

---

## Assertion macros

Drop-in compatible with Unity. The full set:

```
TEST_FAIL[_MESSAGE]                       TEST_PASS[_MESSAGE]
TEST_IGNORE[_MESSAGE]                     TEST_MESSAGE

TEST_ASSERT[_TRUE|_FALSE|_UNLESS|_NULL|_NOT_NULL|_EMPTY|_NOT_EMPTY]

TEST_ASSERT_EQUAL[_INT|_INT8|_INT16|_INT32|_INT64]
TEST_ASSERT_EQUAL_UINT[8|16|32|64]
TEST_ASSERT_EQUAL_HEX[8|16|32|64]
TEST_ASSERT_EQUAL[_CHAR|_PTR|_STRING|_STRING_LEN|_MEMORY]
TEST_ASSERT_NOT_EQUAL[_INT]

TEST_ASSERT_GREATER_THAN[_INT|_UINT|_FLOAT|_DOUBLE]
TEST_ASSERT_LESS_THAN   [_INT|_UINT|_FLOAT|_DOUBLE]
TEST_ASSERT_GREATER_OR_EQUAL[_INT|_FLOAT|_DOUBLE]
TEST_ASSERT_LESS_OR_EQUAL   [_INT|_FLOAT|_DOUBLE]

TEST_ASSERT_INT[8|16|32|64]_WITHIN
TEST_ASSERT_UINT[8|16|32|64]_WITHIN     TEST_ASSERT_HEX_WITHIN

TEST_ASSERT_BITS / BITS_HIGH / BITS_LOW / BIT_HIGH / BIT_LOW

TEST_ASSERT_EQUAL_{INT,UINT,HEX,CHAR}{8|16|32|64}_ARRAY
TEST_ASSERT_EQUAL_{PTR,STRING,MEMORY}_ARRAY
TEST_ASSERT_EACH_EQUAL_{INT,UINT,HEX,CHAR}{8|16|32|64}

TEST_ASSERT_FLOAT_WITHIN / NOT_WITHIN     TEST_ASSERT_EQUAL_FLOAT
TEST_ASSERT_FLOAT_IS_{INF,NEG_INF,NAN,DETERMINATE,NOT_*}
TEST_ASSERT_DOUBLE_* (same set, double precision)

TEST_ASSERT_HW(c)       // only enforced on target (compile with -DCEEDLESS_TARGET)
TEST_ASSERT_HOST(c)     // only enforced on host
```

Plus `*_MESSAGE` variants for the common ones (`TEST_ASSERT_EQUAL_INT_MESSAGE`,
etc.).

### Lifecycle hooks (Unity convention)

```c
void setUp(void)         { /* runs before each test */ }
void tearDown(void)      { /* runs after  each test */ }
void suiteSetUp(void)    { /* runs once before suite */ }
int  suiteTearDown(int failures) { return failures; }
```

All hooks are weak — define them in your test file to override.

---

## CMock-style stateful function mocks

`ceedless`'s mock framework gives you Unity/CMock semantics
(`Expect`, `ExpectAndReturn`, `ExpectAnyArgs`, `Ignore`, `IgnoreAndReturn`,
`StopIgnore`, `ExpectAndThrow`, `ReturnThruPtr`, `StubWithCallback`, `Verify`)
without code generation. You write one tiny stub per mocked function:

```c
#include "ceedless/ceedless.h"

struct hal_send_args { uint8_t addr; uint16_t value; };

MOCK_DEFINE(hal_send);                       /* allocates hal_send_mock */

int hal_send(uint8_t addr, uint16_t value) {
    struct hal_send_args a;
    memset(&a, 0, sizeof a);                 /* zero padding (important) */
    a.addr = addr; a.value = value;
    int rv = -1;
    mock_invoke(&hal_send_mock, &a, sizeof a, &rv, sizeof rv);
    return rv;
}

/* in a test: */
struct hal_send_args a; memset(&a, 0, sizeof a); a.addr = 0x10; a.value = 0xBEEF;
int rv = 1;
MOCK_EXPECT_AND_RETURN(hal_send, &a, sizeof a, &rv, sizeof rv);
/* …call code-under-test… */
MOCK_VERIFY(hal_send);
```

| Macro                                                    | Purpose                                 |
|----------------------------------------------------------|-----------------------------------------|
| `MOCK_DEFINE(name)`                                      | Allocate the mock_t                     |
| `MOCK_RESET(name)`                                       | Clear expectations + counters           |
| `MOCK_EXPECT_AND_RETURN(name, args, alen, ret, rlen)`    | Queue expected call + return value      |
| `MOCK_EXPECT_ANY_ARGS_AND_RETURN(name, ret, rlen)`       | Ignore args, return value               |
| `MOCK_EXPECT_AND_THROW(name, args, alen, code)`          | Queue expected call, then `Throw(code)` |
| `MOCK_RETURN_THRU_PTR(name, arg_offset, data, dlen)`     | Write into a pointer arg                |
| `MOCK_IGNORE / IGNORE_AND_RETURN / STOP_IGNORE`          | Ignore all calls (optional return val)  |
| `MOCK_STUB(name, cb)`                                    | Replace mock with callback              |
| `MOCK_VERIFY(name)`                                      | Fail test if unconsumed expectations    |
| `MOCK_CALL_COUNT(name)`                                  | Observed call count                     |

> **Padding gotcha.** Argument matching is a byte-wise memcmp. ALWAYS
> `memset()` your args struct to zero before populating fields, otherwise
> unspecified padding bytes will cause false mismatches.

---

## CException-style Try/Catch/Throw

```c
#include "ceedless/exception.h"

volatile EXCEPTION_T e = CEXCEPTION_NONE;
Try {
    do_risky_thing();
} Catch (e) {
    log("caught %d", e);
}
```

Nested `Try` blocks up to `CEEDLESS_EXC_STACK` (default 8). Define
`CEEDLESS_EXC_STACK` before including to change.

---

## Virtual peripherals

All peripherals are statically allocated, observe register-level state, and
expose call counters and event logs for assertions.

### I2C / generic register bank

```c
static uint8_t regs[64];
vp_i2c_t eep;
vp_i2c_init(&eep, "eeprom", 0x50, regs, sizeof regs);

uint8_t w[] = { 0x10, 0xDE, 0xAD };          // register pointer + payload
vp_i2c_master_write(&eep, w, sizeof w);

uint8_t ptr = 0x10;
vp_i2c_master_write(&eep, &ptr, 1);          // re-arm cursor

uint8_t r[2] = {0};
vp_i2c_master_read(&eep, r, 2);              // -> {0xDE, 0xAD}
TEST_ASSERT_EQUAL_INT(0x12, eep.cursor);
```

### SPI

```c
vp_spi_t s; vp_spi_init(&s, "spi", NULL, 0);
uint8_t miso[] = { 0xAA, 0x55 };
vp_spi_load_rx(&s, miso, 2);                 // queue what the device returns
vp_spi_xfer(&s, mosi, rx, 2);
TEST_ASSERT_EQUAL_HEX8(0xAA, rx[0]);
```

### UART

```c
vp_uart_t u; vp_uart_init(&u, "u", 115200);
vp_uart_inject(&u, (uint8_t*)"hello", 5);    // simulate other end transmitting
vp_uart_write (&u, (uint8_t*)"OK",    2);    // what driver-under-test sent
uint8_t buf[8];
size_t n = vp_uart_tx_take(&u, buf, sizeof buf);
```

### GPIO

```c
vp_gpio_t g; vp_gpio_init(&g, "g");
vp_gpio_set_mode(&g, 3, VP_GPIO_OUT);
vp_gpio_write   (&g, 3, 1);
TEST_ASSERT_EQUAL_INT(1, vp_gpio_read(&g, 3));
vp_gpio_set_pull(&g, 4, VP_PULL_UP);         // input pull-up
```

### ADC

```c
vp_adc_t a; vp_adc_init(&a, "adc", 12);
vp_adc_set_fixed(&a, 1, 0x123);
TEST_ASSERT_EQUAL_HEX32(0x123, vp_adc_read(&a, 1));

vp_adc_set_gen(&a, 2, ramp_fn, NULL);        // dynamic generator
```

---

## Pluggable trace backend (on-target verification)

The test runner prints test results through `trace_*`. Select the backend
with **one** macro:

| Macro                   | Backend             | Notes                                     |
|-------------------------|---------------------|-------------------------------------------|
| `CEEDLESS_TRACE_HOST`     | `fwrite` to stdout  | default (host simulator)                  |
| `CEEDLESS_TRACE_UART`     | weak `emtest_uart_*`| BSP supplies init/putc/flush hooks        |
| `CEEDLESS_TRACE_RTT`      | weak `emtest_rtt_*` | wrap `SEGGER_RTT_Write` in the hook       |
| `CEEDLESS_TRACE_ITM`      | direct / weak       | define `EMTEST_ITM_DIRECT` for raw regs   |
| `CEEDLESS_TRACE_BUFFER`   | RAM ring (tees)     | used by the framework's own self-tests    |

Target build skeleton (ARM Cortex-M + RTT):

```
arm-none-eabi-gcc -Iinclude -DCEEDLESS_TARGET -DCEEDLESS_TRACE_RTT \
    -mcpu=cortex-m4 -mthumb \
    src/*.c src/trace/*.c src/peripherals/*.c \
    my_tests.c rtt_glue.c -o firmware.elf
```

---

## CLI

`ceedless` is a single static Go binary — no runtime, no dependencies beyond a C compiler.

```
ceedless new myapp && cd myapp
ceedless module led
ceedless test            # auto-discover, build, run
ceedless watch           # re-run on file change
ceedless bench           # slowest tests
ceedless cov             # gcov + HTML report
ceedless mock include/hw.h    # generate MOCK_DEFINE stubs
ceedless doctor          # toolchain check
ceedless docker test     # same, inside a container
```

Build it locally with `make cli` (produces `./bin/ceedless`) or install with
`go install github.com/tevfik/ceedless/cli@latest`.

**Auto-runners**: tests don't declare `main()`. The CLI scans every
`test_*.c` for `void test_<name>(void)` and synthesizes the runner.
Extra sources: `/* TEST_SOURCE_FILE("src/util/crc.c") */`.

Detailed reference: [docs/CLI.md](docs/CLI.md) · Tutorial: [docs/TUTORIAL.md](docs/TUTORIAL.md)

---

## Hybrid (host + target) runner

Identical test source compiles on host and on the MCU. Conditional
assertions:

```c
TEST_ASSERT_TRUE(cond);     // always enforced
TEST_ASSERT_HW(cond);       // only on -DCEEDLESS_TARGET builds
TEST_ASSERT_HOST(cond);     // only on host simulator
```

---

## Performance / footprint notes

- No malloc anywhere in `src/`.
- Static caps are tunable: `CEEDLESS_MAX_TESTS`, `CEEDLESS_MOCK_MAX_CALLS`,
  `CEEDLESS_MOCK_ARG_BYTES`, `EMTEST_MOCK_RET_BYTES`, `CEEDLESS_EXC_STACK`,
  `VP_SPI_BUFSZ`, `VP_UART_BUFSZ`, `CEEDLESS_TRACE_BUFFER_SIZE`.
- All trace backends are byte-streamed; no printf is forced on the target
  build path (UART/RTT/ITM backends do raw `putc`).

---

## License

MIT.
