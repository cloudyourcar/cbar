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
    const char *name;               /**< Line name. Use NULL to terminate config block.  */
    enum cbar_line_type type;       /**< Line type. */

    union {
        struct {
        } input;
        struct {
            int (*get)(intptr_t);   /**< Callback for retrieving input state. */
            intptr_t priv;          /**< Callback argument. */
            bool invert;            /**< True if the input is active-low. */
        } external;
        struct {
            int input;              /**< Input line ID. */
            int threshold_up;       /**< Threshold for low->high transition. */
            int threshold_down;     /**< Threshold for high->low transition. */
        } threshold;
        struct {
            int input;              /**< Input line ID. */
            int timeout_up;         /**< Timeout for low->high transition. */
            int timeout_down;       /**< Timeout for high->low transition. */
        } debounce;
        struct {
        } request;
        struct {
            int (*get)(struct cbar *cbar);  /**< Callback for calculating line state. */
        } calculated;
        struct {
            int input;              /**< Input line ID. */
        } monitor;
        struct {
            int period;             /**< Timer period in miliseconds. */
        } periodic;
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
        } periodic;
    };
};

/**
 * @internal
 */
struct cbar {
    pthread_mutex_t mutex;
    struct cbar_line *lines;
    const struct cbar_line_config *configs;
};

/**
 * Declare a cbar instance. Allocates memory for state storage as well.
 */
#define CBAR_DECLARE(VAR, CONFIGS) \
    struct cbar VAR; \
    struct cbar_line VAR ## _lines[sizeof(CONFIGS)/sizeof(CONFIGS[0])-1];

/**
 * Initialize a cbar instance.
 * @param VAR Variable name (NOTE: not a pointer).
 * @param CONFIGS Configs variable name.
 */
#define CBAR_INIT(VAR, CONFIGS) \
    cbar_init(&VAR, CONFIGS, VAR ## _lines)

/**
 * @internal
 */
void cbar_init(struct cbar *cbar, const struct cbar_line_config *configs, struct cbar_line *lines);

/**
 * Perform one round of debouncing/calculation of states.
 *
 * @param delay Delay since last call, in miliseconds.
 */
void cbar_recalculate(struct cbar *cbar, int delay);

/**
 * Set cbar input line value.
 *
 * @param cbar Initialized cbar instance.
 * @param id Line ID.
 * @param value New value.
 */
void cbar_input(struct cbar *cbar, int id, int value);

/**
 * Read cbar line value.
 * @param cbar Initialized cbar instance.
 * @param id Line ID.
 * @returns Current line value.
 */
int cbar_value(struct cbar *cbar, int id);

/**
 * Post a request.
 * @param cbar Initialized cbar instance.
 * @param id Line ID.
 */
void cbar_post(struct cbar *cbar, int id);

/**
 * Read and clear a "request pending" line.
 * @param cbar Initialized cbar instance.
 * @param id Line ID. Must be one of request, monitor, or periodic.
 * @returns true if request was pending, false otherwise.
 */
bool cbar_pending(struct cbar *cbar, int id);

/**
 * Dump the cbar state.
 * @param cbar Initialized cbar instance.
 */
void cbar_dump(FILE *stream, struct cbar *cbar);

#endif

/* vim: set ts=4 sw=4 et: */
