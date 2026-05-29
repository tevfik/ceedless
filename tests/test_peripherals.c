/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 */
#include "ceedless/ceedless.h"
#include "tests.h"

/* ---- SPI ---- */
static void test_spi_xfer(void)
{
    vp_spi_t s; vp_spi_init(&s, "spi", NULL, 0);
    uint8_t miso_seed[] = { 0xAA, 0x55 };
    vp_spi_load_rx(&s, miso_seed, 2);

    uint8_t tx[] = { 0x01, 0x02 };
    uint8_t rx[2] = { 0 };
    vp_spi_cs_assert(&s);
    vp_spi_xfer(&s, tx, rx, 2);
    vp_spi_cs_deassert(&s);

    TEST_ASSERT_EQUAL_HEX8(0xAA, rx[0]);
    TEST_ASSERT_EQUAL_HEX8(0x55, rx[1]);
    TEST_ASSERT_EQUAL_INT(2, (int)s.tx_len);
    TEST_ASSERT_EQUAL_HEX8(0x01, s.tx_log[0]);
}

/* ---- UART ---- */
static void test_uart_loopback(void)
{
    vp_uart_t u; vp_uart_init(&u, "u", 115200);
    const uint8_t in[] = "hello";
    vp_uart_inject(&u, in, 5);
    TEST_ASSERT_EQUAL_INT(5, vp_uart_available(&u));
    uint8_t c;
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL_INT(0, vp_uart_getc(&u, &c));
        TEST_ASSERT_EQUAL_INT8(in[i], c);
    }
    vp_uart_write(&u, (const uint8_t *)"OK", 2);
    uint8_t buf[8] = { 0 };
    TEST_ASSERT_EQUAL_INT(2, (int)vp_uart_tx_take(&u, buf, sizeof buf));
    TEST_ASSERT_EQUAL_STRING_LEN("OK", (char *)buf, 2);
}

/* ---- GPIO ---- */
static void test_gpio_output_and_input(void)
{
    vp_gpio_t g; vp_gpio_init(&g, "g");
    vp_gpio_set_mode(&g, 3, VP_GPIO_OUT);
    vp_gpio_write   (&g, 3, 1);
    TEST_ASSERT_EQUAL_INT(1, vp_gpio_read(&g, 3));

    vp_gpio_set_mode(&g, 4, VP_GPIO_IN);
    vp_gpio_set_pull(&g, 4, VP_PULL_UP);
    TEST_ASSERT_EQUAL_INT(1, vp_gpio_read(&g, 4));

    vp_gpio_set_pull(&g, 4, VP_PULL_DOWN);
    TEST_ASSERT_EQUAL_INT(0, vp_gpio_read(&g, 4));
}

/* ---- ADC ---- */
static uint32_t ramp(uint8_t ch, uint32_t i, void *u)
{
    (void)ch; (void)u; return 100 + 10 * i;
}
static void test_adc_fixed_and_gen(void)
{
    vp_adc_t a; vp_adc_init(&a, "adc", 12);
    vp_adc_set_fixed(&a, 1, 0x123);
    TEST_ASSERT_EQUAL_HEX32(0x123, vp_adc_read(&a, 1));

    vp_adc_set_gen(&a, 2, ramp, NULL);
    TEST_ASSERT_EQUAL_INT(100, (int)vp_adc_read(&a, 2));
    TEST_ASSERT_EQUAL_INT(110, (int)vp_adc_read(&a, 2));
    TEST_ASSERT_EQUAL_INT(120, (int)vp_adc_read(&a, 2));
    TEST_ASSERT_EQUAL_INT(3,   (int)a.conversions[2]);
}

void run_peripherals_tests(void)
{
    RUN_TEST(test_spi_xfer);
    RUN_TEST(test_uart_loopback);
    RUN_TEST(test_gpio_output_and_input);
    RUN_TEST(test_adc_fixed_and_gen);
}
