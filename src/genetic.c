/* Copyright (c) 1992-2026 Paul McKibbin */

#include "genetic.h"

#include "board.h"
#include "keys.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Randomness comes from the C library, as it did in the 1992 original: srand()
   to seed, rand() to draw. There is deliberately no generator implemented here.

   Two earlier drafts of this rewrite carried one - PCG32 (M. E. O'Neill, 2014),
   then an additive lagged Fibonacci (Mitchell and Moore, 1958). Neither was
   copied from anyone's source, and an algorithm is not copyrightable in any
   case, so neither was a licensing problem. But both were somebody's algorithm,
   and the point of this file is that it is not. Calling the standard library
   settles it: the generator lives in libc, where nobody expects it to be ours.

   The cost is that rand() differs between platforms, so a given --seed
   reproduces a run on one machine and libc, not everywhere. The original had
   the same property.

   RAND_MAX is only guaranteed to reach 32767, so a full 32-bit word is built
   from several draws rather than assuming the platform's range. */
typedef struct {
    int unused; /* the state lives in the C library */
} Rng;

static uint32_t rng_next(Rng *rng)
{
    (void)rng;
    uint32_t value = 0;
    for (int bits = 0; bits < 32; bits += 15) {
        value = (value << 15) | ((uint32_t)rand() & 0x7fffu);
    }
    return value;
}

static void rng_seed(Rng *rng, uint64_t seed)
{
    (void)rng;
    srand((unsigned)seed);
}

/* Uniform in [0, bound), rejecting the biased tail. */
static uint32_t rng_below(Rng *rng, uint32_t bound)
{
    const uint32_t threshold = (uint32_t)(0x100000000ULL % bound);
    for (;;) {
        const uint32_t value = rng_next(rng);
        if (value >= threshold) {
            return value % bound;
        }
    }
}

static double rng_unit(Rng *rng)
{
    return (double)rng_next(rng) / 4294967296.0;
}

typedef struct {
    int *genes;
    long fitness;
} Individual;

static int by_fitness_desc(const void *a, const void *b)
{
    const long left = ((const Individual *)a)->fitness;
    const long right = ((const Individual *)b)->fitness;
    if (left < right) {
        return 1;
    }
    return left > right ? -1 : 0;
}

long genetic_perfect_fitness(int n)
{
    return 2L * n * n * n;
}

static long evaluate(const int *genes, int n)
{
    long fitness = 0;
    for (int row = 0; row < n; row++) {
        const int attackers = queens_attackers(genes, n, row);
        long score = (long)(n - attackers) * n;
        if (attackers == 0) {
            score *= 2;
        }
        fitness += score;
    }
    return fitness;
}

static void mutate(Rng *rng, int *genes, int n)
{
    genes[rng_below(rng, (uint32_t)n)] = (int)rng_below(rng, (uint32_t)n);
}

/* One-point crossover producing complementary children; `daughter` may be NULL
   when the population is odd. */
static void crossover(Rng *rng, const Individual *father, const Individual *mother,
                      Individual *son, Individual *daughter, int n, double mutation_rate)
{
    const size_t split = rng_below(rng, (uint32_t)n);
    const size_t head = split * sizeof(int);
    const size_t tail = ((size_t)n - split) * sizeof(int);

    memcpy(son->genes, father->genes, head);
    memcpy(son->genes + split, mother->genes + split, tail);
    if (rng_unit(rng) < mutation_rate) {
        mutate(rng, son->genes, n);
    }

    if (daughter != NULL) {
        memcpy(daughter->genes, mother->genes, head);
        memcpy(daughter->genes + split, father->genes + split, tail);
        if (rng_unit(rng) < mutation_rate) {
            mutate(rng, daughter->genes, n);
        }
    }
}

long genetic_default_population(int n)
{
    /* Cost per solution grows linearly with population, so the ideal is the
       smallest one that still wins - but this program does not restart a
       failed search for you, so a default that stalls one run in three is a
       worse experience than one that is slightly slower and finds it. The
       measured point where the win rate settles is around 10n, with a floor
       for small boards where 10n is too few to hold any diversity. See the
       population benchmark in README.md. */
    const long scaled = 10L * n;
    return scaled > 400 ? scaled : 400;
}

GeneticParams genetic_default_params(void)
{
    GeneticParams params = {
        .population = genetic_default_population(8),
        .mutation_rate = 0.9,
        .seed = 1,
        .max_generations = 0,
        .stall_limit = 2000,
        .progress_every = 0,
        .progress_out = NULL,
    };
    return params;
}

/* Any key shows the best board in the population right now, solution or not —
   the original's print_top(). ESC or q gives up. */
static bool handle_key(const Individual *best, int n, long perfect, long generation, FILE *out)
{
    const int key = keys_poll();

    if (key == KEY_NONE) {
        return false;
    }
    if (key_is_quit(key)) {
        queens_quit_key = true;
        return true;
    }
    fprintf(out, "generation %ld, best fitness %ld of %ld\n", generation, best->fitness, perfect);
    queens_print(out, best->genes, n);
    fputc('\n', out);
    return false;
}

/* Score an individual and hand it in if it solves the board. */
static void assess(Individual *individual, int n, long perfect, Recorder *recorder,
                   long long *evaluations)
{
    individual->fitness = evaluate(individual->genes, n);
    (*evaluations)++;
    if (individual->fitness == perfect) {
        recorder_submit(recorder, individual->genes);
    }
}

GeneticStats genetic_solve(int n, const GeneticParams *params, Recorder *recorder,
                           const Deadline *deadline)
{
    const long count = params->population >= 2 ? params->population : 2;
    const long half = count / 2;
    const long perfect = genetic_perfect_fitness(n);
    long stall = 0;

    Rng rng;
    rng_seed(&rng, params->seed);

    Individual *population = queens_alloc((size_t)count, sizeof *population);
    int *pool = queens_alloc((size_t)count * (size_t)n, sizeof *pool);

    GeneticStats stats = {0};
    stats.perfect_fitness = perfect;

    for (long i = 0; i < count; i++) {
        population[i].genes = pool + i * n;
        for (int row = 0; row < n; row++) {
            population[i].genes[row] = (int)rng_below(&rng, (uint32_t)n);
        }
        assess(&population[i], n, perfect, recorder, &stats.evaluations);
    }

    while (!recorder_limit_reached(recorder)) {
        if (queens_interrupted != 0 || deadline_expired(deadline)
            || (params->max_generations > 0 && stats.generations >= params->max_generations)) {
            stats.stopped = true;
            break;
        }

        qsort(population, (size_t)count, sizeof *population, by_fitness_desc);

        if (handle_key(&population[0], n, perfect, stats.generations, recorder->out)) {
            stats.stopped = true;
            break;
        }

        /* The original gave up the moment the surviving half converged on one
           genome, which with any mutation at all is premature: crossover is
           spent but mutation still moves. Give up on a stalled leader instead. */
        if (population[0].fitness > stats.best_fitness) {
            stats.best_fitness = population[0].fitness;
            stall = 0;
        } else if (params->stall_limit > 0 && ++stall >= params->stall_limit) {
            stats.stagnated = true;
            break;
        }

        stats.generations++;

        for (long i = half; i < count; i += 2) {
            const Individual *father = &population[rng_below(&rng, (uint32_t)half)];
            const Individual *mother = &population[rng_below(&rng, (uint32_t)half)];
            Individual *son = &population[i];
            Individual *daughter = (i + 1 < count) ? &population[i + 1] : NULL;

            crossover(&rng, father, mother, son, daughter, n, params->mutation_rate);

            assess(son, n, perfect, recorder, &stats.evaluations);
            if (daughter != NULL) {
                assess(daughter, n, perfect, recorder, &stats.evaluations);
            }
        }

        if (params->progress_out != NULL && params->progress_every > 0
            && stats.generations % params->progress_every == 0) {
            fprintf(params->progress_out,
                    "generation %ld  best %ld/%ld  solutions %ld\n",
                    stats.generations, stats.best_fitness, perfect, recorder->count);
        }
    }

    /* Children bred after the last sort may beat the ranked leader. */
    for (long i = 0; i < count; i++) {
        if (population[i].fitness > stats.best_fitness) {
            stats.best_fitness = population[i].fitness;
        }
    }

    free(pool);
    free(population);
    return stats;
}
