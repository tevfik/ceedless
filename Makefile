# --- ceedless: minimal embedded test framework ---------------------------
CC      ?= gcc
CSTD    ?= -std=c11
WARN    ?= -Wall -Wextra -Wshadow -Wpedantic -Wno-unused-function -Wno-stringop-truncation
OPT     ?= -O2 -g
CFLAGS  ?= $(CSTD) $(WARN) $(OPT)
INCS    := -Iinclude 
LDLIBS  := -lm

CORE_SRC := \
    src/runner.c \
    src/exception.c \
    src/mock.c \
    src/virtual_peripheral.c \
    src/trace/trace_port.c \
    src/trace/trace_host.c \
    src/trace/trace_uart.c \
    src/trace/trace_rtt.c \
    src/trace/trace_itm.c \
    src/trace/trace_buffer.c \
    src/peripherals/vp_spi.c \
    src/peripherals/vp_uart.c \
    src/peripherals/vp_gpio.c \
    src/peripherals/vp_adc.c

SELFTEST_SRC := \
    tests/test_main.c \
    tests/test_trace_port.c \
    tests/test_virtual_peripheral.c \
    tests/test_runner.c \
    tests/test_assertions.c \
    tests/test_exception.c \
    tests/test_mock.c \
    tests/test_peripherals.c

EXAMPLE_SRC := \
    examples/sensor_driver/sensor_driver.c \
    examples/sensor_driver/test_sensor_driver.c

BUILD := build

.PHONY: all cli test example cli-test docker docker-test lint clean
all: cli test example cli-test

# Build the Go CLI into bin/ceedless. Pure stdlib, no deps.
cli:
	cd cli && go build -trimpath -ldflags "-s -w" -o ../bin/ceedless .

# Build the dev/CI container image. Run-once; cached afterwards.
docker:
	docker build -t ceedless:latest .

# Run the full self-test suite inside the container.
docker-test: docker
	docker run --rm -t -v $(CURDIR):/work -w /work ceedless:latest \
	    sh -c "make all"

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/selftest: $(CORE_SRC) $(SELFTEST_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(INCS) -DCEEDLESS_TRACE_BUFFER \
	    $(CORE_SRC) $(SELFTEST_SRC) -o $@ $(LDLIBS)

$(BUILD)/negative: $(CORE_SRC) tests/test_fail_main.c | $(BUILD)
	$(CC) $(CFLAGS) $(INCS) -DCEEDLESS_TRACE_BUFFER \
	    $(CORE_SRC) tests/test_fail_main.c -o $@ $(LDLIBS)

$(BUILD)/example_sensor: $(CORE_SRC) $(EXAMPLE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(INCS) -DCEEDLESS_TRACE_HOST \
	    -Iexamples/sensor_driver \
	    $(CORE_SRC) $(EXAMPLE_SRC) -o $@ $(LDLIBS)

test: $(BUILD)/selftest $(BUILD)/negative $(BUILD)/example_sensor
	@echo "================================================================"
	@echo " ceedless self-tests"
	@echo "================================================================"
	@./$(BUILD)/selftest
	@echo
	@echo "================================================================"
	@echo " negative path (expected to FAIL)"
	@echo "================================================================"
	@if ./$(BUILD)/negative; then \
	    echo "ERROR: negative test should have failed"; exit 1; \
	 else \
	    echo "OK: negative test failed as expected"; \
	 fi
	@echo
	@echo "================================================================"
	@echo " example: sensor_driver"
	@echo "================================================================"
	@./$(BUILD)/example_sensor

example: $(BUILD)/example_sensor

# Smoke-test the CLI: scaffolds a project, adds a module, runs its tests.
cli-test: cli
	@echo "================================================================"
	@echo " ceedless CLI smoke test"
	@echo "================================================================"
	@rm -rf $(BUILD)/cli_demo
	@mkdir -p $(BUILD)
	@./bin/ceedless new $(BUILD)/cli_demo
	@cd $(BUILD)/cli_demo && CEEDLESS_HOME=$(CURDIR) ../../bin/ceedless module hello && CEEDLESS_HOME=$(CURDIR) ../../bin/ceedless test

lint:
	@echo "==> gofmt"
	@cd cli && out=$$(gofmt -l .); if [ -n "$$out" ]; then echo "$$out"; echo "gofmt issues found"; exit 1; fi
	@echo "==> go vet"
	@cd cli && go vet ./...
	@echo "==> go test"
	@cd cli && go test ./...

clean:
	rm -rf $(BUILD)
