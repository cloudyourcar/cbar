/*
 * Copyright © 2014 Kosma Moczek <kosma@cloudyourcar.com>
 * This program is free software. It comes without any warranty, to the extent
 * permitted by applicable law. You can redistribute it and/or modify it under
 * the terms of the Do What The Fuck You Want To Public License, Version 2, as
 * published by Sam Hocevar. See the COPYING file for more details.
 */

#ifndef CROSSBAR_H
#define CROSSBAR_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>


enum cbar_line_type {
    CROSSBAR_INPUT = 1,
    CROSSBAR_EXTERNAL,
    CROSSBAR_THRESHOLD,
    CROSSBAR_DEBOUNCE,
    CROSSBAR_REQUEST,
    CROSSBAR_CALCULATED,
    CROSSBAR_MONITOR,
    CROSSBAR_PERIODIC,
};

struct cbar;

struct cbar_line_config {
    const char *name;
    enum cbar_line_type type;

    union {
        struct {
        } input;
        struct {
            int (*get)(intptr_t);
            intptr_t priv;
            bool invert;
        } external;
        struct {
            int input;
            int threshold_up;
            int threshold_down;
        } threshold;
        struct {
            int input;
            int timeout_up;
            int timeout_down;
        } debounce;
        struct {
        } request;
        struct {
            int (*get)(struct cbar *cbar);
        } calculated;
        struct {
            int input;
        } monitor;
        struct {
            int period;
        } timer;
    };
};

/**
 * @internal
 */
struct cbar_line {
    int value;
    union {
        struct {
            int value;
            int timer;
        } debounce;
        struct {
            int previous;
        } monitor;
        struct {
            int elapsed;
        } timer;
    };
};

struct cbar {
    pthread_mutex_t mutex;
    struct cbar_line *lines;
    const struct cbar_line_config *configs;
};

/**
 * Declare a cbar instance. Allocates memory for lines storage as well.
 */
#define CBAR_DECLARE(VAR, CONFIGS) \
    struct cbar VAR; \
    struct cbar_line VAR ## _lines[sizeof(CONFIGS)/sizeof(CONFIGS[0])-1];

#define CBAR_INIT(VAR, CONFIGS) \
    cbar_init(&VAR, CONFIGS, VAR ## _lines)

/**
 * Initialize cbar instance.
 * @param cbar Instance to be initialized.
 */
void cbar_init(struct cbar *cbar, const struct cbar_line_config *configs, struct cbar_line *lines);

/**
 * Perform one round of debouncing/calculation of states.
 *
 * @param delay Delay since last call, in miliseconds.
 */
void cbar_recalculate(struct cbar *cbar, int delay);

/**
 * Feed cbar input line.
 */
void cbar_input(struct cbar *cbar, int id, int value);

/**
 * Read cbar line value.
 */
int cbar_value(struct cbar *cbar, int id);

/**
 * Post a request.
 */
void cbar_post(struct cbar *cbar, int id);

/**
 * Read and clear a "request pending" line.
 */
int cbar_pending(struct cbar *cbar, int id);

/**
 * Dump the cbar state.
 */
void cbar_dump(struct cbar *cbar);

#endif

/* vim: set ts=4 sw=4 et: */
