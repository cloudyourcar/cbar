/*
 * Copyright © 2014 Kosma Moczek <kosma@cloudyourcar.com>
 * This program is free software. It comes without any warranty, to the extent
 * permitted by applicable law. You can redistribute it and/or modify it under
 * the terms of the Do What The Fuck You Want To Public License, Version 2, as
 * published by Sam Hocevar. See the COPYING file for more details.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <check.h>

#include "cbar.h"


/****************************************************************************/

enum gpio_pin {
    GPIO_IN0,
    GPIO_IN1,
    GPIO_IN2,
    N_GPIO_PINS
};

static int gpio_pins[N_GPIO_PINS];

int gpio_get(intptr_t id)
{
    ck_assert_int_ge(id, 0);
    ck_assert_int_lt(id, N_GPIO_PINS);
    return gpio_pins[id];
}

void gpio_set(int id, bool value)
{
    gpio_pins[id] = value;
}

/****************************************************************************/

START_TEST(test_cbar_input)
{
    enum lines {
        LINE_VOLTAGE,
    };
    static const struct cbar_line_config configs[] = {
        { "input0", CBAR_INPUT },
        { NULL }
    };

    CBAR_DECLARE(cbar, configs);
    CBAR_INIT(cbar, configs);

    /* inputs start at zero */
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE), 0);

    /* input changes don't take effect until a recalculation */
    cbar_input(&cbar, LINE_VOLTAGE, 3185);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE), 0);
    cbar_recalculate(&cbar, 0);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE), 3185);
}
END_TEST

/****************************************************************************/

START_TEST(test_cbar_external)
{
    enum lines {
        LINE_IN0,
        LINE_IN1,
        LINE_IN2,
    };
    static const struct cbar_line_config configs[] = {
        { "in0", CBAR_EXTERNAL, .external = { gpio_get, GPIO_IN0 } },
        { "in1", CBAR_EXTERNAL, .external = { gpio_get, GPIO_IN1 } },
        { "in2", CBAR_EXTERNAL, .external = { gpio_get, GPIO_IN2, .invert = true } },
        { NULL }
    };

    CBAR_DECLARE(cbar, configs);

    /* set initial input states */
    gpio_set(GPIO_IN0, false);
    gpio_set(GPIO_IN1, true);
    gpio_set(GPIO_IN2, true);

    /* make sure cbar reads them */
    CBAR_INIT(cbar, configs);
    ck_assert_int_eq(cbar_value(&cbar, LINE_IN0), false);
    ck_assert_int_eq(cbar_value(&cbar, LINE_IN1), true);
    ck_assert_int_eq(cbar_value(&cbar, LINE_IN2), false);

    /* flip the inputs */
    gpio_set(GPIO_IN0, true);
    gpio_set(GPIO_IN1, false);
    gpio_set(GPIO_IN2, false);

    /* cbar should still see the old values */
    ck_assert_int_eq(cbar_value(&cbar, LINE_IN0), false);
    ck_assert_int_eq(cbar_value(&cbar, LINE_IN1), true);
    ck_assert_int_eq(cbar_value(&cbar, LINE_IN2), false);

    /* new values should be visible after recalculation */
    cbar_recalculate(&cbar, 0);
    ck_assert_int_eq(cbar_value(&cbar, LINE_IN0), true);
    ck_assert_int_eq(cbar_value(&cbar, LINE_IN1), false);
    ck_assert_int_eq(cbar_value(&cbar, LINE_IN2), true);
}
END_TEST

/****************************************************************************/

START_TEST(test_cbar_threshold)
{
    enum lines {
        LINE_VOLTAGE,
        LINE_VOLTAGE_OK,
        LINE_VOLTAGE_GE,
    };
    const struct cbar_line_config configs[] = {
        { "voltage",    CBAR_INPUT },
        /* with hysteresis */
        { "voltage_ok", CBAR_THRESHOLD, .threshold = { LINE_VOLTAGE, 1050, 950 } },
        /* without hysteresis */
        { "voltage_ge", CBAR_THRESHOLD, .threshold = { LINE_VOLTAGE, 1000, 1000 } },
        { NULL }
    };

    CBAR_DECLARE(cbar, configs);
    CBAR_INIT(cbar, configs);

    /* initial input state is zero, so the threshold isn't met */
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE), 0);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_OK), false);

    /* set input voltage to not enough, shouldn't budge at all */
    cbar_input(&cbar, LINE_VOLTAGE, 1049);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_OK), false);
    cbar_recalculate(&cbar, 0);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_OK), false);

    /* set it high enough, should flip to true */
    cbar_input(&cbar, LINE_VOLTAGE, 1050);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_OK), false);
    cbar_recalculate(&cbar, 0);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_OK), true);

    /* set it to lower threshold, should stay high */
    cbar_input(&cbar, LINE_VOLTAGE, 950);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_OK), true);
    cbar_recalculate(&cbar, 0);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_OK), true);

    /* set it below lower threshold, should go low again */
    cbar_input(&cbar, LINE_VOLTAGE, 949);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_OK), true);
    cbar_recalculate(&cbar, 0);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_OK), false);

    /* lack of hysteresis shouldn't cause flapping on constant value */
    cbar_input(&cbar, LINE_VOLTAGE, 1000);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_GE), false);
    cbar_recalculate(&cbar, 0);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_GE), true);
    cbar_recalculate(&cbar, 0);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_GE), true);
    cbar_recalculate(&cbar, 0);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_GE), true);
}
END_TEST

/****************************************************************************/

START_TEST(test_cbar_debounce)
{
}
END_TEST

/****************************************************************************/

START_TEST(test_cbar_request)
{
}
END_TEST

/****************************************************************************/

START_TEST(test_cbar_calculated)
{
}
END_TEST

/****************************************************************************/

START_TEST(test_cbar_monitor)
{
}
END_TEST

/****************************************************************************/

START_TEST(test_cbar_periodic)
{
}
END_TEST

/****************************************************************************/

Suite *cbar_suite(void)
{
    Suite *s = suite_create("cbar");
    TCase *tc;

    tc = tcase_create("cbar");
    tcase_add_test(tc, test_cbar_input);
    tcase_add_test(tc, test_cbar_external);
    tcase_add_test(tc, test_cbar_threshold);
    tcase_add_test(tc, test_cbar_debounce);
    tcase_add_test(tc, test_cbar_request);
    tcase_add_test(tc, test_cbar_calculated);
    tcase_add_test(tc, test_cbar_monitor);
    tcase_add_test(tc, test_cbar_periodic);
    suite_add_tcase(s, tc);

    return s;
}

int main()
{
    int number_failed;
    Suite *s = cbar_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* vim: set ts=4 sw=4 et: */
