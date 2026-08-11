/* Copyright (c) 1992-2026 Paul McKibbin */

#include "runtime.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

volatile sig_atomic_t queens_interrupted = 0;
bool queens_quit_key = false;

static void on_interrupt(int sig)
{
    (void)sig;
    queens_interrupted = 1;
}

void queens_catch_interrupt(void)
{
    signal(SIGINT, on_interrupt);
}

double queens_now(void)
{
    struct timespec ts;
    if (timespec_get(&ts, TIME_UTC) != TIME_UTC) {
        return 0.0;
    }
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

uint64_t queens_seed_from_clock(void)
{
    struct timespec ts = {0, 0};
    (void)timespec_get(&ts, TIME_UTC);

    /* Nanoseconds carry the entropy; the seconds keep runs apart across reboots
       and the address stirs in whatever ASLR gives us. */
    uint64_t seed = (uint64_t)ts.tv_sec * 1000000000u + (uint64_t)ts.tv_nsec;
    seed ^= (uint64_t)(uintptr_t)&ts << 16;
    return seed;
}

Deadline deadline_start(double limit_seconds)
{
    Deadline deadline;
    deadline.start = queens_now();
    deadline.limit = limit_seconds > 0.0 ? limit_seconds : 0.0;
    return deadline;
}

bool deadline_expired(const Deadline *deadline)
{
    return deadline->limit > 0.0 && queens_now() - deadline->start >= deadline->limit;
}

double deadline_elapsed(const Deadline *deadline)
{
    return queens_now() - deadline->start;
}

static void die_out_of_memory(void)
{
    fputs("nqueens: out of memory\n", stderr);
    exit(EXIT_FAILURE);
}

void *queens_alloc(size_t count, size_t size)
{
    void *ptr = calloc(count != 0 ? count : 1, size);
    if (ptr == NULL) {
        die_out_of_memory();
    }
    return ptr;
}

void *queens_grow(void *ptr, size_t count, size_t size)
{
    if (count != 0 && size > SIZE_MAX / count) {
        die_out_of_memory();
    }
    void *grown = realloc(ptr, count != 0 ? count * size : 1);
    if (grown == NULL) {
        die_out_of_memory();
    }
    return grown;
}
