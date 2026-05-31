# Cortex-M PIL example

This example demonstrates running `ceedless` test suites unchanged on an ARM
Cortex-M target. The same assertion macros that pass on the host are
re-compiled with `arm-none-eabi-gcc`, linked against a minimal startup,
loaded into QEMU's `mps2-an385` machine, and report their results back to
the host terminal via ARM semihosting.

## Prerequisites

```bash
# Debian / Ubuntu
sudo apt install gcc-arm-none-eabi qemu-system-arm

# Fedora
sudo dnf install arm-none-eabi-gcc-cs arm-none-eabi-newlib qemu-system-arm

# macOS (Homebrew)
brew install --cask gcc-arm-embedded
brew install qemu
```

`ceedless doctor` reports both tools so you can verify they're on PATH.

## Run

```bash
make qemu
```

You should see:

```
=== ceedless suite: cortex_m3_pil ===
[RUN ] test_arithmetic_on_target
[PASS] test_arithmetic_on_target
[RUN ] test_assert_hw_macro_is_active
[PASS] test_assert_hw_macro_is_active
[RUN ] test_bit_ops
[PASS] test_bit_ops
=== result: 3/3 passed, 0 failed, 0 ignored ===
```

QEMU then exits cleanly via semihosting `SYS_EXIT`.

## How it works

| Piece | File |
|------|------|
| Vector table + Reset_Handler + newlib syscalls | `startup.c` |
| Memory layout (4 MB FLASH @ 0x0, 4 MB RAM @ 0x20000000) | `mps2-an385.ld` |
| Semihosting `bkpt #0xAB` trace backend | `../../src/trace/trace_semihost.c` |
| Test sources (same API as host) | `test_cortex_m.c` |

The build defines `CEEDLESS_TARGET=cortex_m3` so any `TEST_ASSERT_HW(...)`
macro in the suite becomes active and `TEST_ASSERT_HOST(...)` becomes a
no-op — the same source compiles and passes on both sides.

## Porting to your own board

1. Swap `mps2-an385.ld` for your part's memory map.
2. Replace `--specs=nano.specs --specs=nosys.specs` with your usual flags
   (e.g. add CMSIS, vendor HAL).
3. Pick a different trace backend if semihosting isn't available
   (RTT/ITM/UART are all built-in).
