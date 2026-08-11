/* Copyright (c) 1992-2026 Paul McKibbin */

/* Where solvers hand in the solutions they find.

   The original repeated this logic — dedupe, count, print, stop at the limit,
   tally the heatmap — inside every variant of the program. It lives here once
   and both solvers share it. */
#pragma once

#include "board.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef void (*SolutionHook)(void *context, const int *queens, int n, long index);

typedef struct {
    int n;
    long limit;          /* stop after this many solutions; 0 means no limit */
    bool print_boards;
    bool dedupe;         /* backtracking never repeats itself; the GA does */
    Heatmap *heatmap;    /* optional */
    FILE *out;
    SolutionHook hook;   /* optional; called for each accepted solution */
    void *hook_context;

    long count;
    int *last;           /* the most recent solution, for the interactive peek */
    bool has_last;
    int **seen;          /* retained placements, only when dedupe is on */
    size_t seen_len;
    size_t seen_cap;
} Recorder;

void recorder_init(Recorder *recorder, int n, FILE *out);
void recorder_free(Recorder *recorder);

/* Offer a solution. Returns true if it was new (and therefore counted). */
bool recorder_submit(Recorder *recorder, const int *queens);

/* True once the configured solution limit has been reached. */
bool recorder_limit_reached(const Recorder *recorder);

/* Print the most recent solution: what the interactive peek shows. */
void recorder_show_last(const Recorder *recorder);
