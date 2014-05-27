# the little crossbar

``cbar`` is a little library that makes writing business logic for embedded
systems much easier.

## Problem that needs solving

Imagine your typical embedded C project: there are dozens of inputs and
outputs, some GPIO, other analog, some half-assed timers, ifs, switches and
conditions scattered around your codebase. Figuring out just when and why
things happen is a huge mystery; even worse, changing one thing causes three
other to break. Even worse, some time into the project you often find yourself
pretty much unable to explain exactly *how* the thing works; it does, but the
interactions between subsystems become so convoluted it's one big mess.

No, really. Imagine your boss comes to you and asks you to make the LEDs react
to input state changes faster. You change the delays, and suddenly your power
sequencing no longer works. Imagine how many times you've been unable to
explain the device's operation to someone else because the spaghetti code meant
you couldn't grasp it yourself.

``cbar`` solves this problem by making your so-called business logic declarative
and keeping it in one place - easy to understand, debug and change. And away
from drivers, where it doesn't belong.

## So what's business logic, anyway?

* Enabling the RS485 transmitter is driver logic. Enabling the relay that
  controls A/C is business logic.
* Turning the modem on with a two-second pulse is driver logic. Turning off
  after the user holds the power button for two seconds is business logic.
* Controlling the diagnostic LEDs is driver logic. Controlling the user-facing
  LEDs is business logic.
* Beeping the mainboard speaker is driver logic. Beeping the speaker in
  a consumer device is business logic.
* Configuring EXTI interrupts is driver logic. Deciding when to enable them
  and how to react to the wakeup is business logic.
* Turning off when the battery voltage goes critically low is driver logic.
  Turning off after 60 seconds of inactivity is business logic.

## A real life example

```c
/* First, enumerate your lines. */
enum {
    IN_VOLTAGE_CAR,
    IN_VOLTAGE_BATTERY,
    IN_TEMPERATURE,
    IN_DEVICE_INSERTED,

    IN_GPS_FIX,

    LINE_POWER_AVAILABLE_RAW,
    LINE_ENGINE_RUNNING_RAW,

    LINE_DEVICE_INSERTED,
    LINE_POWER_AVAILABLE,
    LINE_ENGINE_RUNNING,

    LINE_LED_COLOR,

    MONITOR_GPS_FIX,
    MONITOR_LED_COLOR,
    MONITOR_ENGINE_RUNNING,
};

/* Now define the inputs, outputs and their relationships. */
static const struct cbar_line_config configs[] = {
    /* Some ADC and GPIO inputs: */
    { "in_voltage_car",         CBAR_EXTERNAL, .external = { adc_measure, ADC_CHANNEL_VCAR  } },
    { "in_voltage_battery",     CBAR_EXTERNAL, .external = { adc_measure, ADC_CHANNEL_VBATT } },
    { "in_temperature",         CBAR_EXTERNAL, .external = { adc_measure, ADC_CHANNEL_TEMP  } },
    { "in_device_inserted",     CBAR_EXTERNAL, .external = { gpio_get_state, GPIO_PIN_INSDET } },

    /* Inputs coming from other software modules: */
    { "in_gps_fix",             CBAR_INPUT },

    /* Thresholds on analog inputs: */
    { "power_available_raw",    CBAR_THRESHOLD, .threshold = { IN_VOLTAGE_CAR, 11000, 10000 } },
    { "engine_running"_raw,     CBAR_THRESHOLD, .threshold = { IN_VOLTAGE_CAR, 13300, 13100 } },

    /* Apply some debouncing to the input lines: */
    { "device_inserted",        CBAR_DEBOUNCE, .debounce = { IN_DEVICE_INSERTED,    1000,  1000 } },
    { "power_available",        CBAR_DEBOUNCE, .debounce = { LINE_POWER_AVAILABLE_RAW, 1000,  1000 } },
    { "engine_running",         CBAR_DEBOUNCE, .debounce = { LINE_ENGINE_RUNNING_RAW,     0, 10000 } },

    /* Complex values can be calculated using external callbacks like this: */
    { "led_color",              CBAR_CALCULATED, .calculated = { calculate_led_color } },

    /* Listening for state changes is as simple as: */
    { "monitor_gpx_fix",        CBAR_MONITOR, .monitor = { IN_GPS_FIX } },
    { "monitor_led_color",      CBAR_MONITOR, .monitor = { LINE_LED_COLOR } },
    { "monitor_engine_running", CBAR_MONITOR, .monitor = { LINE_ENGINE_RUNNING } },

    { NULL }
};

static int calculate_led_color(struct cbar *cbar)
{
    if (cbar_value(cbar, LINE_POWER_AVAILABLE) == false)
        return LED_BLACK;
    if (cbar_value(cbar, LINE_ENGINE_RUNNING) == false)
        return LED_RED;
    if (cbar_value(cbar, IN_GPS_FIX) == false)
        return LED_BLUE;
    return LED_GREEN;
}

/* And here's the main loop. */
while (true) {
    sleep_ms(100);

    /* Feed inputs from external subsystems. */
    cbar_input(&cbar, INPUT_GPS_FIX, gps->fix_valid);

    cbar_recalculate(&cbar, 100);

    /* Control timers. */
    if (cbar_pending(&cbar, MONITOR_ENGINE_RUNNING))
        if (cbar_value(&cbar, LINE_ENGINE_RUNNING) == false)
            start_shutdown_timer();
        else
            stop_shutdown_timer();

    /* Do something when a line goes high. */
    if (cbar_pending(&cbar, MONITOR_GPS_FIX))
        if (cbar_value(&cbar, INPUT_GPS_FIX) == true)
            /* Synchronize RTC with GPS time. */
            settimeofday(gps_gettimeofday());

    /* Update output state whenever inputs change. */
    if (cbar_pending(&cbar, MONITOR_LED_COLOR))
        rgbled_setcolor(cbar_value(&cbar, LINE_LED_COLOR));
}
```

## WTF. It's so complicated. Why bother with all this?

Because otherwise your logic will get lost SOMEWHERE DEEP IN THE CODE.
We went that road before. We thought we can get away with it. We thought
it was simple. We were wrong.

At some point, all decent consumer devices acquire lots of quirks and rules
governing operation in different situations. Unless you can keep those rules
in one place, when you can control them and keep them simple, you'll end up
with a cheap-feeling Chinese-like contraption instead of an Apple-esque piece
of art.

Decide for yourself. Our logic router is not perfect, since it was written
for our purposes - but even if it just inspires you or ignites your
imagination - that's all the better.

## Features

* Written in ISO C99.
* No dynamic memory allocation.
* Small and lean - one source file and one header.
* Complete with a test suite and static analysis.
* Thread-safe.

## Requirements

* Mutex support. If your RTOS doesn't support pthreads, writing a little shim
  layer should be straightforward; see compat/ directory for examples.
  Alternatively, if you're running single-threaded, you can replace the mutex
  calls with no-ops.
* Some way of measuring passing time: an RTC, a timer, anything like that.
  ``cbar`` doesn't call the C ``time`` function; it's up to you to pass the
  elapsed time to ``cbar_recalculate``.

## Usage

Simply add ``cbar.[ch]`` to your project, ``#include "cbar.h"`` and you're
good to go.

## Running unit tests

Building and running the tests requires the following:

* Check Framework (http://check.sourceforge.net/).
* Clang Static Analyzer (http://clang-analyzer.llvm.org/).

If you have both in your ``$PATH``, running the tests should be as simple as
typing ``make``.

## Licensing

``cbar`` is open source software; see ``COPYING`` for amusement. Email me if the
license bothers you and I'll happily re-license under anything else under the sun.

## Author

``cbar`` was written by Kosma Moczek &lt;kosma@cloudyourcar.com&gt; at Cloud Your Car.
