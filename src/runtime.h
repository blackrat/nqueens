/* Copyright (c) 1992-2026 Paul McKibbin */

/* Process-wide odds and ends: the clock, the search budget, the interrupt
   flag, and allocation that cannot fail. */
#pragma once

#include <signal.h>
#include <stdbool.h>
#include <stddef.h>

#include <stdint.h>

/* Seconds from an unspecified epoch; only differences are meaningful. */
double queens_now(void);

/* A seed that differs between runs, including runs started in the same second. */
uint64_t queens_seed_from_clock(void);

/* An optional wall-clock budget for a search. A limit of 0 means "no limit". */
typedef struct {
    double start;
    double limit;
} Deadline;

Deadline deadline_start(double limit_seconds);
bool deadline_expired(const Deadline *deadline);
double deadline_elapsed(const Deadline *deadline);

/* Set from SIGINT so a long search can unwind and still report what it found.
   This replaces the original's kbhit()/ESC polling, which only worked on DOS. */
extern volatile sig_atomic_t queens_interrupted;
void queens_catch_interrupt(void);

/* Set when the user pressed ESC or q during a search. Kept apart from the
   signal flag so the summary can say which of the two it was. */
extern bool queens_quit_key;

/* Every allocation here is small and mandatory, so there is no useful recovery
   path: these abort instead of returning NULL. */
void *queens_alloc(size_t count, size_t size);
void *queens_grow(void *ptr, size_t count, size_t size);
