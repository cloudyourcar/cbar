# the little crossbar

``cbar`` is a little library that makes writing business logic for embedded
systems much easier.

## The problem that needed solving

Imagine your typical embedded C project: there are dozens of inputs and
outputs, some GPIO, other analog, some half-assed timers, ifs, switches and
conditions scattered around your codebase. Figuring out just when and why
things happen is a huge mystery; even worse, changing one thing causes three
other to break. Even worse, some time into the project you often find yourself
pretty much unable to explain exactly *how* the thing works; it does, but the
interactions between subsystems become so convoluted it's one big mess.

``cbar`` solves this problem by making your logic declarative and keeping it
in one place - easy to understand, debug and change.

Sounds too good to be true? Here's some real code.

## Example

```c
```

## Features

* Written in ISO C99.
* No dynamic memory allocation.
* Small and lean - one source file and one header.
* Complete with a test suite and static analysis.

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
