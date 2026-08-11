/* Copyright (c) 1992-2026 Paul McKibbin */

/* Command-line configuration.

   Board size, population, time budget and algorithm were all #defines in the
   original, which is why it shipped as five near-identical source files. They
   are runtime options now and there is one program. */
#pragma once

#include "genetic.h"

#include <stdbool.h>
#include <stdio.h>

#define QUEENS_MAX_SIZE 1000

typedef enum {
    ALGORITHM_BACKTRACK,
    ALGORITHM_GENETIC
} Algorithm;

typedef struct {
    int n;
    Algorithm algorithm;
    long max_solutions; /* 0 means "as many as there are" */
    double time_limit;  /* seconds; 0 means no limit */
    bool print_boards;
    bool show_heatmap;
    GeneticParams genetic;
} Options;

typedef enum {
    OPTIONS_OK,
    OPTIONS_HELP,
    OPTIONS_ERROR
} OptionsResult;

Options options_defaults(void);
OptionsResult options_parse(Options *options, int argc, char **argv, FILE *err);
void options_usage(FILE *out, const char *program);
