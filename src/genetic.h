/* Copyright (c) 1992-2026 Paul McKibbin */

/* Genetic search: the original's "GENETIC" build, minus the #ifdef maze.

   A genome is a placement (queens[row] = col, columns may repeat). Fitness
   ranks each row by how few queens attack it, with a bonus for rows that are
   clean; a perfect genome scores 2 * n^3. Each generation the ranked bottom
   half is replaced by one-point-crossover children of random top-half parents.

   Unlike backtracking this is not exhaustive: it stops when it stagnates, runs
   out of budget, or has found everything it was asked for. */
#pragma once

#include "recorder.h"
#include "runtime.h"

#include <stdint.h>

typedef struct {
    long population;
    double mutation_rate; /* probability that a child mutates one row */
    uint64_t seed;
    long max_generations; /* 0 means no limit */
    long stall_limit;     /* give up after this many generations with no gain */
    long progress_every;  /* generations between progress lines; 0 means never */
    FILE *progress_out;   /* where those lines go; NULL means nowhere */
} GeneticParams;

typedef struct {
    long generations;
    long long evaluations; /* fitness evaluations performed */
    long best_fitness;
    long perfect_fitness; /* the score a solution scores */
    bool stagnated;       /* gave up: no fitness gain for stall_limit generations */
    bool stopped;         /* ended on budget or interrupt */
} GeneticStats;

GeneticParams genetic_default_params(void);

/* The fitness a solution scores: 2 * n^3. */
long genetic_perfect_fitness(int n);

/* A population that suits the board: see the benchmark note in README.md.
   Cost per solution grows linearly with population once it is large enough to
   be reliable, so the aim is the smallest population that still wins. */
long genetic_default_population(int n);

GeneticStats genetic_solve(int n, const GeneticParams *params, Recorder *recorder,
                           const Deadline *deadline);
