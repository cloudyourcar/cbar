# Copyright © 2014 Kosma Moczek <kosma@cloudyourcar.com>
# This program is free software. It comes without any warranty, to the extent
# permitted by applicable law. You can redistribute it and/or modify it under
# the terms of the Do What The Fuck You Want To Public License, Version 2, as
# published by Sam Hocevar. See the COPYING file for more details.

CFLAGS = -g -Wall -Werror -std=c99
CFLAGS += -D_GNU_SOURCE
LDLIBS = -lcheck -lm -lpthread -lrt

all: scan-build test example
	@echo "+++ All good."""

test: tests
	@echo "+++ Running Check test suite..."
	./tests

scan-build: clean
	@echo "+++ Running Clang Static Analyzer..."
	scan-build $(MAKE) tests

clean:
	$(RM) tests *.o

tests: tests.o cbar.o
example: example.o cbar.o
tests.o: tests.c cbar.h
cbar.o: cbar.c cbar.h

.PHONY: all test scan-build clean
