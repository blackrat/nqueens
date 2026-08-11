/* Copyright (c) 1992-2026 Paul McKibbin */

#include "recorder.h"

#include "runtime.h"

#include <stdlib.h>
#include <string.h>

void recorder_init(Recorder *recorder, int n, FILE *out)
{
    memset(recorder, 0, sizeof *recorder);
    recorder->n = n;
    recorder->out = out;
    recorder->print_boards = true;
    recorder->last = queens_alloc((size_t)n, sizeof *recorder->last);
}

void recorder_free(Recorder *recorder)
{
    for (size_t i = 0; i < recorder->seen_len; i++) {
        free(recorder->seen[i]);
    }
    free(recorder->seen);
    free(recorder->last);
    recorder->last = NULL;
    recorder->has_last = false;
    recorder->seen = NULL;
    recorder->seen_len = 0;
    recorder->seen_cap = 0;
}

static bool already_seen(const Recorder *recorder, const int *queens)
{
    const size_t bytes = (size_t)recorder->n * sizeof *queens;
    for (size_t i = 0; i < recorder->seen_len; i++) {
        if (memcmp(recorder->seen[i], queens, bytes) == 0) {
            return true;
        }
    }
    return false;
}

static void remember(Recorder *recorder, const int *queens)
{
    if (recorder->seen_len == recorder->seen_cap) {
        recorder->seen_cap = recorder->seen_cap != 0 ? recorder->seen_cap * 2 : 64;
        recorder->seen = queens_grow(recorder->seen, recorder->seen_cap, sizeof *recorder->seen);
    }
    int *copy = queens_alloc((size_t)recorder->n, sizeof *copy);
    memcpy(copy, queens, (size_t)recorder->n * sizeof *copy);
    recorder->seen[recorder->seen_len++] = copy;
}

bool recorder_submit(Recorder *recorder, const int *queens)
{
    /* Solvers notice the limit between steps, and the genetic one can hand in
       several solutions within a single generation. Enforce it here so --max-
       solutions is exact rather than approximate. */
    if (recorder_limit_reached(recorder)) {
        return false;
    }
    if (recorder->dedupe) {
        if (already_seen(recorder, queens)) {
            return false;
        }
        remember(recorder, queens);
    }

    recorder->count++;
    memcpy(recorder->last, queens, (size_t)recorder->n * sizeof *recorder->last);
    recorder->has_last = true;

    if (recorder->heatmap != NULL) {
        heatmap_add(recorder->heatmap, queens);
    }
    if (recorder->print_boards) {
        fprintf(recorder->out, "Solution %ld\n", recorder->count);
        queens_print(recorder->out, queens, recorder->n);
        fputc('\n', recorder->out);
    }
    if (recorder->hook != NULL) {
        recorder->hook(recorder->hook_context, queens, recorder->n, recorder->count);
    }
    return true;
}

bool recorder_limit_reached(const Recorder *recorder)
{
    return recorder->limit > 0 && recorder->count >= recorder->limit;
}

void recorder_show_last(const Recorder *recorder)
{
    if (!recorder->has_last) {
        fprintf(recorder->out, "no solution yet\n\n");
        return;
    }
    fprintf(recorder->out, "last solution (%ld so far)\n", recorder->count);
    queens_print(recorder->out, recorder->last, recorder->n);
    fputc('\n', recorder->out);
}
