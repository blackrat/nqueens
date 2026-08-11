/* Copyright (c) 1992-2026 Paul McKibbin */

#include "backtrack.h"
#include "board.h"
#include "genetic.h"
#include "keys.h"
#include "options.h"
#include "recorder.h"
#include "runtime.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static const char *stop_reason(const Deadline *deadline, const Recorder *recorder)
{
    if (queens_interrupted != 0) {
        return "interrupted with Ctrl-C";
    }
    if (queens_quit_key) {
        return "stopped from the keyboard";
    }
    if (deadline_expired(deadline)) {
        return "time limit reached";
    }
    if (recorder_limit_reached(recorder)) {
        return "solution limit reached";
    }
    return "stopped early";
}

/* Returns true when the run finished on its own terms. */
static bool run_backtrack(const Options *options, Recorder *recorder, const Deadline *deadline)
{
    const BacktrackStats stats = backtrack_solve(options->n, recorder, deadline);

    printf("%ld solution%s for n=%d in %.6fs (%llu nodes)\n", recorder->count,
           recorder->count == 1 ? "" : "s", options->n, deadline_elapsed(deadline), stats.nodes);
    if (stats.stopped) {
        printf("search incomplete: %s\n", stop_reason(deadline, recorder));
    }
    return !stats.stopped || recorder_limit_reached(recorder);
}

static bool run_genetic(const Options *options, Recorder *recorder, const Deadline *deadline)
{
    const GeneticStats stats = genetic_solve(options->n, &options->genetic, recorder, deadline);

    printf("%ld solution%s for n=%d in %.6fs (%ld generations, %lld evaluations, "
           "best fitness %ld of %ld)\n",
           recorder->count, recorder->count == 1 ? "" : "s", options->n,
           deadline_elapsed(deadline), stats.generations, stats.evaluations, stats.best_fitness,
           stats.perfect_fitness);
    /* The seed is random unless asked otherwise, so print it: it is the only
       way back to this particular run. */
    printf("seed %" PRIu64 "\n", options->genetic.seed);
    if (stats.stagnated) {
        printf("population stagnated: no fitness gain in %ld generations\n",
               options->genetic.stall_limit);
    } else if (stats.stopped) {
        printf("search stopped: %s\n",
               options->genetic.max_generations > 0
                       && stats.generations >= options->genetic.max_generations
                   ? "generation limit reached"
                   : stop_reason(deadline, recorder));
    }
    return !stats.stopped || recorder_limit_reached(recorder);
}

int main(int argc, char **argv)
{
    Options options;

    switch (options_parse(&options, argc, argv, stderr)) {
    case OPTIONS_HELP:
        options_usage(stdout, argv[0]);
        return EXIT_SUCCESS;
    case OPTIONS_ERROR:
        return EXIT_FAILURE;
    case OPTIONS_OK:
        break;
    }

    queens_catch_interrupt();
    keys_begin();

    Heatmap heatmap = {0};
    if (options.show_heatmap) {
        heatmap_init(&heatmap, options.n);
    }

    Recorder recorder;
    recorder_init(&recorder, options.n, stdout);
    recorder.limit = options.max_solutions;
    recorder.print_boards = options.print_boards;
    recorder.dedupe = options.algorithm == ALGORITHM_GENETIC;
    recorder.heatmap = options.show_heatmap ? &heatmap : NULL;

    options.genetic.progress_out = stderr;

    const Deadline deadline = deadline_start(options.time_limit);
    const bool complete = options.algorithm == ALGORITHM_GENETIC
                              ? run_genetic(&options, &recorder, &deadline)
                              : run_backtrack(&options, &recorder, &deadline);

    if (options.show_heatmap) {
        printf("\nsquare occupancy across %ld solution%s:\n", recorder.count,
               recorder.count == 1 ? "" : "s");
        heatmap_print(stdout, &heatmap);
        heatmap_free(&heatmap);
    }

    keys_end();
    recorder_free(&recorder);
    return complete ? EXIT_SUCCESS : EXIT_FAILURE;
}
