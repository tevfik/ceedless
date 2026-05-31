# SPDX-License-Identifier: MIT
# ARM Cortex-M3 (QEMU mps2-an385) PIL example for ceedless.
#
#   make            -> build elf
#   make qemu       -> run under QEMU with semihosting
#   make clean
#
# Requires:
#   - arm-none-eabi-gcc + newlib (Debian: gcc-arm-none-eabi)
#   - qemu-system-arm  (Debian: qemu-system-arm)

CROSS   ?= arm-none-eabi-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy

ROOT    ?= ../..
CFLAGS  := -mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections \
           -Wall -Wextra -O1 -g -std=c11 \
           --specs=nano.specs --specs=nosys.specs \
           -DCEEDLESS_TARGET=cortex_m3 -DCEEDLESS_TRACE_SEMIHOST \
           -I$(ROOT)/include
LDFLAGS := -T mps2-an385.ld -Wl,--gc-sections -nostartfiles

CORE_SRC := \
    $(ROOT)/src/runner.c \
    $(ROOT)/src/exception.c \
    $(ROOT)/src/mock.c \
    $(ROOT)/src/virtual_peripheral.c \
    $(ROOT)/src/trace/trace_port.c \
    $(ROOT)/src/trace/trace_semihost.c \
    $(ROOT)/src/trace/trace_host.c \
    $(ROOT)/src/trace/trace_uart.c \
    $(ROOT)/src/trace/trace_rtt.c \
    $(ROOT)/src/trace/trace_itm.c \
    $(ROOT)/src/trace/trace_buffer.c \
    $(ROOT)/src/peripherals/vp_spi.c \
    $(ROOT)/src/peripherals/vp_uart.c \
    $(ROOT)/src/peripherals/vp_gpio.c \
    $(ROOT)/src/peripherals/vp_adc.c

SRC := startup.c test_cortex_m.c $(CORE_SRC)

ELF := build/cortex_m_pil.elf
BIN := build/cortex_m_pil.bin

.PHONY: all qemu clean

all: $(ELF)

build:
	mkdir -p build

$(ELF): $(SRC) | build
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -o $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

# `-semihosting -nographic` routes SYS_WRITE0 to the host terminal and
# `-kernel <elf>` loads the image at the vector-table address.
qemu: $(ELF)
	qemu-system-arm -M mps2-an385 -cpu cortex-m3 -nographic \
	    -semihosting-config enable=on,target=native \
	    -kernel $(ELF) ; \
	rc=$$? ; \
	if [ $$rc -ne 0 ] && [ $$rc -ne 134 ]; then echo "QEMU exited $$rc"; fi ; \
	exit 0

clean:
	rm -rf build
