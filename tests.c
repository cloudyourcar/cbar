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

/* Compatibility corkaround for older versions of Check framework. */
#ifndef ck_assert_int_ge
#define ck_assert_int_ge(X, Y) _ck_assert_int(X, >=, Y)
#endif
#ifndef ck_assert_int_lt
#define ck_assert_int_lt(X, Y) _ck_assert_int(X, <, Y)
#endif

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
        LINE_VOLTAGE_NOT_OK,
    };
    const struct cbar_line_config configs[] = {
        { "voltage",    CBAR_INPUT },
        /* with hysteresis */
        { "voltage_ok", CBAR_THRESHOLD, .threshold = { LINE_VOLTAGE, 1050, 950 } },
        /* without hysteresis */
        { "voltage_ge", CBAR_THRESHOLD, .threshold = { LINE_VOLTAGE, 1000, 1000 } },
        /* inverted logic (active below threshold) */
        { "voltage_not_ok",  CBAR_THRESHOLD, .threshold = { LINE_VOLTAGE, 950, 1050 } },
        { NULL }
    };

    CBAR_DECLARE(cbar, configs);
    CBAR_INIT(cbar, configs);

    /* initial input state is zero, so the threshold isn't met */
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE), 0);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_OK), false);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_NOT_OK), true);

    /* set input voltage to not enough, shouldn't budge at all */
    cbar_input(&cbar, LINE_VOLTAGE, 1049);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_OK), false);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_NOT_OK), true);
    cbar_recalculate(&cbar, 0);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_OK), false);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_NOT_OK), true);

    /* set it high enough, should flip to true */
    cbar_input(&cbar, LINE_VOLTAGE, 1050);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_OK), false);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_NOT_OK), true);
    cbar_recalculate(&cbar, 0);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_OK), true);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_NOT_OK), false);

    /* set it to lower threshold, should stay high */
    cbar_input(&cbar, LINE_VOLTAGE, 950);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_OK), true);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_NOT_OK), false);
    cbar_recalculate(&cbar, 0);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_OK), true);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_NOT_OK), false);

    /* set it below lower threshold, should go low again */
    cbar_input(&cbar, LINE_VOLTAGE, 949);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_OK), true);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_NOT_OK), false);
    cbar_recalculate(&cbar, 0);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_OK), false);
    ck_assert_int_eq(cbar_value(&cbar, LINE_VOLTAGE_NOT_OK), true);

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
    enum lines {
        LINE_IN0,
        LINE_DEBOUNCE_A,
        LINE_DEBOUNCE_B,
        LINE_DEBOUNCE_C,
    };
    static const struct cbar_line_config configs[] = {
        { "in0", CBAR_INPUT },
        { "a",   CBAR_DEBOUNCE, .debounce = { LINE_IN0,    0, 1000 } },
        { "b",   CBAR_DEBOUNCE, .debounce = { LINE_IN0, 1000, 1000 } },
        { "c",   CBAR_DEBOUNCE, .debounce = { LINE_IN0, 1000,    0 } },
        { NULL }
    };

    CBAR_DECLARE(cbar, configs);
    CBAR_INIT(cbar, configs);

    /* all values should be zero initially */
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_A), false);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_B), false);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_C), false);

    /* set the input to a new value. nothing should change yet. */
    cbar_input(&cbar, LINE_IN0, true);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_A), false);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_B), false);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_C), false);

    /* pass no time. the first debounce should fire immediately. */
    cbar_recalculate(&cbar, 0);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_A), true);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_B), false);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_C), false);

    /* pass almost enough time. nothing should fire right now. */
    cbar_recalculate(&cbar, 999);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_A), true);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_B), false);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_C), false);

    /* pass the remaining. the other ones should fire now too. */
    cbar_recalculate(&cbar, 1);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_A), true);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_B), true);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_C), true);

    /* go back down after a while. the third one should fire immediately. */
    cbar_input(&cbar, LINE_IN0, false);
    cbar_recalculate(&cbar, 5000);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_A), true);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_B), true);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_C), false);

    /* pass some time. nothing should change yet. */
    cbar_recalculate(&cbar, 250);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_A), true);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_B), true);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_C), false);

    /* go back down. */
    cbar_recalculate(&cbar, 750);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_A), false);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_B), false);
    ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_C), false);

    /* fire a few times; should not printf "stable" repeatedly.
     * NOTE: no unit test here; just look at the logs. */
    printf("<this section should be empty>\n");
    cbar_recalculate(&cbar, 1000);
    cbar_recalculate(&cbar, 1000);
    cbar_recalculate(&cbar, 1000);
    printf("</this section should be empty>\n");

    /* cause the line to flap below debounce threshold. it should behave like this: */
    for (int i=0; i<4; i++) {
        cbar_input(&cbar, LINE_IN0, true);
        cbar_recalculate(&cbar, 500);
        ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_A), true);
        ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_B), false);
        ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_C), false);

        cbar_input(&cbar, LINE_IN0, false);
        cbar_recalculate(&cbar, 500);
        ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_A), true);
        ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_B), false);
        ck_assert_int_eq(cbar_value(&cbar, LINE_DEBOUNCE_C), false);
    }
}
END_TEST

/****************************************************************************/

START_TEST(test_cbar_request)
{
    enum lines {
        LINE_REQ1,
        LINE_REQ2,
    };
    static const struct cbar_line_config configs[] = {
        { "req1", CBAR_REQUEST },
        { "req2", CBAR_REQUEST },
        { NULL }
    };

    CBAR_DECLARE(cbar, configs);
    CBAR_INIT(cbar, configs);

    /* request are initially non-pending */
    ck_assert_int_eq(cbar_pending(&cbar, LINE_REQ1), false);
    ck_assert_int_eq(cbar_pending(&cbar, LINE_REQ2), false);

    /* send some, should fire once */
    cbar_post(&cbar, LINE_REQ1);
    ck_assert_int_eq(cbar_pending(&cbar, LINE_REQ1), true);
    ck_assert_int_eq(cbar_pending(&cbar, LINE_REQ1), false);

    /* request are non-counting. */
    cbar_post(&cbar, LINE_REQ2);
    cbar_post(&cbar, LINE_REQ2);
    ck_assert_int_eq(cbar_pending(&cbar, LINE_REQ2), true);
    ck_assert_int_eq(cbar_pending(&cbar, LINE_REQ2), false);
}
END_TEST

/****************************************************************************/

enum calculated_lines {
    LINE_ENGINE_RUNNING,
    LINE_IN_MOTION,
    LINE_CAR_IDLING,
};

static int calculate_idling(struct cbar *cbar)
{
    return  cbar_value(cbar, LINE_ENGINE_RUNNING) &&
           !cbar_value(cbar, LINE_IN_MOTION);
}

START_TEST(test_cbar_calculated)
{
    static const struct cbar_line_config configs[] = {
        { "engine_running", CBAR_INPUT },
        { "in_motion",      CBAR_INPUT },
        { "car_idling",     CBAR_CALCULATED, .calculated = { calculate_idling } },
        { NULL }
    };

    CBAR_DECLARE(cbar, configs);
    CBAR_INIT(cbar, configs);

    /* initially everything is zero */
    ck_assert_int_eq(cbar_value(&cbar, LINE_ENGINE_RUNNING), false);
    ck_assert_int_eq(cbar_value(&cbar, LINE_IN_MOTION), false);
    ck_assert_int_eq(cbar_value(&cbar, LINE_CAR_IDLING), false);

    /* start the engine. we're now idling. */
    cbar_input(&cbar, LINE_ENGINE_RUNNING, true);
    cbar_recalculate(&cbar, 100);
    ck_assert_int_eq(cbar_value(&cbar, LINE_ENGINE_RUNNING), true);
    ck_assert_int_eq(cbar_value(&cbar, LINE_IN_MOTION), false);
    ck_assert_int_eq(cbar_value(&cbar, LINE_CAR_IDLING), true);

    /* start moving. we're no longer idling. */
    cbar_input(&cbar, LINE_IN_MOTION, true);
    cbar_recalculate(&cbar, 100);
    ck_assert_int_eq(cbar_value(&cbar, LINE_ENGINE_RUNNING), true);
    ck_assert_int_eq(cbar_value(&cbar, LINE_IN_MOTION), true);
    ck_assert_int_eq(cbar_value(&cbar, LINE_CAR_IDLING), false);
}
END_TEST

/****************************************************************************/

static int temperature;
static int get_temperature(intptr_t priv)
{
    return temperature;
}

START_TEST(test_cbar_monitor)
{
    enum lines {
        INPUT_SUPPLY_VOLTAGE,
        INPUT_GPS_FIX,
        INPUT_TEMPERATURE,

        LINE_POWER_AVAILABLE,
        LINE_THERMAL_ALARM,

        MONITOR_POWER,
        MONITOR_GPS,
        MONITOR_THERMAL,
    };
    static const struct cbar_line_config configs[] = {
        { "supply_voltage",  CBAR_INPUT },
        { "gps_fix",         CBAR_INPUT },
        { "temperature",     CBAR_EXTERNAL, .external = { get_temperature, 0 } },

        { "power_available", CBAR_THRESHOLD, .threshold = { INPUT_SUPPLY_VOLTAGE, 3800, 3800 } },
        { "thermal_alarm",   CBAR_THRESHOLD, .threshold = { INPUT_TEMPERATURE, 50, 45 } },

        { "monitor_power",   CBAR_MONITOR, .monitor = { LINE_POWER_AVAILABLE } },
        { "monitor_gps",     CBAR_MONITOR, .monitor = { INPUT_GPS_FIX } },
        { "monitor_thermal", CBAR_MONITOR, .monitor = { LINE_THERMAL_ALARM } },

        { NULL }
    };

    CBAR_DECLARE(cbar, configs);
    CBAR_INIT(cbar, configs);

    /* all monitors should be pending after first recalculation. */
    ck_assert_int_eq(cbar_pending(&cbar, MONITOR_POWER), true);
    ck_assert_int_eq(cbar_pending(&cbar, MONITOR_POWER), false);
    ck_assert_int_eq(cbar_pending(&cbar, MONITOR_GPS), true);
    ck_assert_int_eq(cbar_pending(&cbar, MONITOR_GPS), false);
    ck_assert_int_eq(cbar_pending(&cbar, MONITOR_THERMAL), true);
    ck_assert_int_eq(cbar_pending(&cbar, MONITOR_THERMAL), false);

    /* cause a temperature raise, triggering a monitor */
    temperature = 60;
    cbar_recalculate(&cbar, 100);
    ck_assert_int_eq(cbar_pending(&cbar, MONITOR_THERMAL), true);
    ck_assert_int_eq(cbar_pending(&cbar, MONITOR_THERMAL), false);

    /* changing line state causes a request as well */
    cbar_input(&cbar, INPUT_GPS_FIX, true);
    cbar_recalculate(&cbar, 100);
    ck_assert_int_eq(cbar_pending(&cbar, MONITOR_GPS), true);
    ck_assert_int_eq(cbar_pending(&cbar, MONITOR_GPS), false);

    /* ...but a glitch doesn't. */
    cbar_input(&cbar, INPUT_GPS_FIX, false);
    cbar_input(&cbar, INPUT_GPS_FIX, true);
    cbar_recalculate(&cbar, 100);
    ck_assert_int_eq(cbar_pending(&cbar, MONITOR_GPS), false);
}
END_TEST

/****************************************************************************/

START_TEST(test_cbar_periodic)
{
    enum lines {
        LINE_TICK,
    };
    static const struct cbar_line_config configs[] = {
        { "tick", CBAR_PERIODIC, .periodic = { 1000 } },
        { NULL }
    };

    CBAR_DECLARE(cbar, configs);
    CBAR_INIT(cbar, configs);

    /* starts as inactive */
    ck_assert_int_eq(cbar_pending(&cbar, LINE_TICK), false);

    /* some time passes... */
    cbar_recalculate(&cbar, 500);
    ck_assert_int_eq(cbar_pending(&cbar, LINE_TICK), false);
    cbar_recalculate(&cbar, 499);
    ck_assert_int_eq(cbar_pending(&cbar, LINE_TICK), false);
    cbar_recalculate(&cbar, 1);
    ck_assert_int_eq(cbar_pending(&cbar, LINE_TICK), true);
    ck_assert_int_eq(cbar_pending(&cbar, LINE_TICK), false);

    /* activations don't stack. */
    cbar_recalculate(&cbar, 1000);
    cbar_recalculate(&cbar, 1000);
    cbar_recalculate(&cbar, 1000);
    ck_assert_int_eq(cbar_pending(&cbar, LINE_TICK), true);
    ck_assert_int_eq(cbar_pending(&cbar, LINE_TICK), false);

    /* time doesn't stack as well, to prevent activating too soon. */
    cbar_recalculate(&cbar, 1500);
    ck_assert_int_eq(cbar_pending(&cbar, LINE_TICK), true);
    ck_assert_int_eq(cbar_pending(&cbar, LINE_TICK), false);
    cbar_recalculate(&cbar, 500);
    ck_assert_int_eq(cbar_pending(&cbar, LINE_TICK), false);
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
