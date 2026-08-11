/* Copyright (c) 1992-2026 Paul McKibbin */

#include "board.h"
#include "check.h"
#include "genetic.h"
#include "recorder.h"
#include "runtime.h"

#include <stdlib.h>

typedef struct {
    int invalid;
    int n;
} Validator;

static void validate(void *context, const int *queens, int n, long index)
{
    Validator *validator = context;
    (void)index;
    if (!queens_is_solution(queens, n)) {
        validator->invalid++;
    }
}

static void test_perfect_fitness(void)
{
    CHECK_EQ_LONG(genetic_perfect_fitness(8), 2L * 8 * 8 * 8);
    CHECK_EQ_LONG(genetic_perfect_fitness(1), 2);
}

/* A fixed seed must find solutions, and every solution it reports must be one. */
static void test_finds_solutions(void)
{
    Validator validator = {0, 8};
    Recorder recorder;
    recorder_init(&recorder, 8, stdout);
    recorder.print_boards = false;
    recorder.dedupe = true;
    recorder.limit = 3;
    recorder.hook = validate;
    recorder.hook_context = &validator;

    GeneticParams params = genetic_default_params();
    params.population = 400;
    params.seed = 12345;
    params.max_generations = 2000;

    const Deadline deadline = deadline_start(30.0);
    const GeneticStats stats = genetic_solve(8, &params, &recorder, &deadline);

    CHECK_EQ_LONG(validator.invalid, 0);
    CHECK(recorder.count > 0);
    CHECK(stats.evaluations > 0);
    CHECK(stats.best_fitness == stats.perfect_fitness);

    recorder_free(&recorder);
}

/* The same seed must give the same run twice. */
static void test_reproducible(void)
{
    long counts[2] = {0, 0};
    long long evaluations[2] = {0, 0};

    for (int run = 0; run < 2; run++) {
        Recorder recorder;
        recorder_init(&recorder, 8, stdout);
        recorder.print_boards = false;
        recorder.dedupe = true;
        recorder.limit = 4;

        GeneticParams params = genetic_default_params();
        params.population = 200;
        params.seed = 99;
        params.max_generations = 500;

        const Deadline deadline = deadline_start(30.0);
        const GeneticStats stats = genetic_solve(8, &params, &recorder, &deadline);

        counts[run] = recorder.count;
        evaluations[run] = stats.evaluations;
        recorder_free(&recorder);
    }

    CHECK_EQ_LONG(counts[0], counts[1]);
    CHECK(evaluations[0] == evaluations[1]);
}

/* An odd population must not run off the end of the array while breeding, and
   a population of one individual must not divide by zero. */
static void test_awkward_populations(void)
{
    const long sizes[] = {2, 3, 7, 101};

    for (size_t i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
        Validator validator = {0, 6};
        Recorder recorder;
        recorder_init(&recorder, 6, stdout);
        recorder.print_boards = false;
        recorder.dedupe = true;
        recorder.hook = validate;
        recorder.hook_context = &validator;

        GeneticParams params = genetic_default_params();
        params.population = sizes[i];
        params.seed = 7;
        params.max_generations = 200;

        const Deadline deadline = deadline_start(10.0);
        genetic_solve(6, &params, &recorder, &deadline);

        CHECK_EQ_LONG(validator.invalid, 0);
        recorder_free(&recorder);
    }
}

/* The recorder must not hand the same placement in twice. */
static void test_dedupe(void)
{
    Recorder recorder;
    recorder_init(&recorder, 4, stdout);
    recorder.print_boards = false;
    recorder.dedupe = true;

    const int solution[4] = {1, 3, 0, 2};
    const int other[4] = {2, 0, 3, 1};

    CHECK(recorder_submit(&recorder, solution));
    CHECK(!recorder_submit(&recorder, solution));
    CHECK(recorder_submit(&recorder, other));
    CHECK_EQ_LONG(recorder.count, 2);

    recorder_free(&recorder);
}

int main(void)
{
    test_perfect_fitness();
    test_finds_solutions();
    test_reproducible();
    test_awkward_populations();
    test_dedupe();
    return CHECK_REPORT();
}
