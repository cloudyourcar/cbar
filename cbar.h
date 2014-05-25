/*
 * Copyright © 2014 Kosma Moczek <kosma@cloudyourcar.com>
 * This program is free software. It comes without any warranty, to the extent
 * permitted by applicable law. You can redistribute it and/or modify it under
 * the terms of the Do What The Fuck You Want To Public License, Version 2, as
 * published by Sam Hocevar. See the COPYING file for more details.
 */

#ifndef CROSSBAR_H
#define CROSSBAR_H

#include <stdbool.h>


struct cbar;

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

struct cbar_line_config {
    const char *name;
    enum cbar_line_type type;

    union {
        struct {
        } input;
        struct {
            int (*get)(void *priv);
            void *priv;
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
 * Initialize cbar instance.
 */
void cbar_init(struct cbar *cbar, struct cbar_line_config *configs);

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
