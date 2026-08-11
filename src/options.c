/* Copyright (c) 1992-2026 Paul McKibbin */

#include "options.h"

#include "runtime.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

Options options_defaults(void)
{
    Options options = {
        .n = 8,
        .algorithm = ALGORITHM_BACKTRACK,
        .max_solutions = 0,
        .time_limit = 0.0,
        .print_boards = true,
        .show_heatmap = false,
        .genetic = {0},
    };
    options.genetic = genetic_default_params();
    options.genetic.progress_every = 1000;
    /* Randomised unless the caller asks for a specific seed. The library
       default stays fixed so tests stay reproducible without arguing. */
    options.genetic.seed = queens_seed_from_clock();
    return options;
}

void options_usage(FILE *out, const char *program)
{
    fprintf(out,
            "usage: %s [options]\n"
            "\n"
            "Place n non-attacking queens on an n-by-n board.\n"
            "\n"
            "  -n, --size N            board size (default 8, max %d)\n"
            "  -a, --algorithm NAME    backtrack (exhaustive) or genetic\n"
            "                          (default backtrack)\n"
            "  -m, --max-solutions N   stop after N solutions (default: all)\n"
            "  -t, --time-limit SECS   wall-clock budget (default: none)\n"
            "  -c, --count-only        report totals, do not print boards\n"
            "  -H, --heatmap           print how often each square was used\n"
            "  -h, --help              this message\n"
            "\n"
            "genetic only:\n"
            "  -p, --population N      individuals per generation\n"
            "                          (default 10n, at least %ld)\n"
            "      --mutation-rate F   per-child mutation probability (default %.4g)\n"
            "      --seed N|clock      RNG seed (default: from the clock, so runs\n"
            "                          differ); pass the seed printed in the\n"
            "                          summary to repeat a run exactly\n"
            "      --generations N     stop after N generations (default: no limit)\n"
            "      --stall N           give up after N generations without a\n"
            "                          fitness gain, 0 to never (default %ld)\n"
            "      --progress N        progress line every N generations, 0 to\n"
            "                          silence (default 1000)\n"
            "\n"
            "While a search runs: any key shows where it has got to, ESC or q stops\n"
            "it, and Ctrl-C stops it too. Either way you still get the summary.\n",
            program, QUEENS_MAX_SIZE, genetic_default_population(1),
            genetic_default_params().mutation_rate, genetic_default_params().stall_limit);
}

static bool parse_long(const char *text, long min, long max, const char *what, long *out,
                       FILE *err)
{
    errno = 0;
    char *end = NULL;
    const long value = strtol(text, &end, 10);

    if (end == text || *end != '\0' || errno == ERANGE || value < min || value > max) {
        fprintf(err, "queens: %s must be an integer between %ld and %ld, got '%s'\n", what, min,
                max, text);
        return false;
    }
    *out = value;
    return true;
}

static bool parse_double(const char *text, double min, double max, const char *what, double *out,
                         FILE *err)
{
    errno = 0;
    char *end = NULL;
    const double value = strtod(text, &end);

    if (end == text || *end != '\0' || errno == ERANGE || !(value >= min && value <= max)) {
        fprintf(err, "queens: %s must be a number between %g and %g, got '%s'\n", what, min, max,
                text);
        return false;
    }
    *out = value;
    return true;
}

/* Matches --name, --name=value and -x, filling `value` from the attached or the
   following argument when the option takes one. */
typedef struct {
    char **argv;
    int argc;
    int index;
    FILE *err;
} Parser;

static bool option_is(const char *arg, const char *long_name, const char *short_name)
{
    const size_t long_len = strlen(long_name);
    if (strncmp(arg, long_name, long_len) == 0 && (arg[long_len] == '\0' || arg[long_len] == '=')) {
        return true;
    }
    return short_name != NULL && strcmp(arg, short_name) == 0;
}

static const char *option_value(Parser *parser, const char *long_name)
{
    const char *arg = parser->argv[parser->index];
    const char *attached = strchr(arg, '=');

    if (attached != NULL && strncmp(arg, long_name, strlen(long_name)) == 0) {
        return attached + 1;
    }
    if (parser->index + 1 >= parser->argc) {
        fprintf(parser->err, "queens: %s needs a value\n", arg);
        return NULL;
    }
    return parser->argv[++parser->index];
}

OptionsResult options_parse(Options *options, int argc, char **argv, FILE *err)
{
    *options = options_defaults();
    Parser parser = {.argv = argv, .argc = argc, .index = 1, .err = err};
    bool population_given = false;

    for (; parser.index < argc; parser.index++) {
        const char *arg = argv[parser.index];
        const char *value = NULL;
        long number = 0;

        if (option_is(arg, "--help", "-h")) {
            return OPTIONS_HELP;
        }
        if (option_is(arg, "--count-only", "-c")) {
            options->print_boards = false;
            continue;
        }
        if (option_is(arg, "--heatmap", "-H")) {
            options->show_heatmap = true;
            continue;
        }
        if (option_is(arg, "--size", "-n")) {
            if ((value = option_value(&parser, "--size")) == NULL
                || !parse_long(value, 1, QUEENS_MAX_SIZE, "--size", &number, err)) {
                return OPTIONS_ERROR;
            }
            options->n = (int)number;
            continue;
        }
        if (option_is(arg, "--algorithm", "-a")) {
            if ((value = option_value(&parser, "--algorithm")) == NULL) {
                return OPTIONS_ERROR;
            }
            if (strcmp(value, "backtrack") == 0) {
                options->algorithm = ALGORITHM_BACKTRACK;
            } else if (strcmp(value, "genetic") == 0) {
                options->algorithm = ALGORITHM_GENETIC;
            } else {
                fprintf(err, "queens: unknown algorithm '%s' (want backtrack or genetic)\n", value);
                return OPTIONS_ERROR;
            }
            continue;
        }
        if (option_is(arg, "--max-solutions", "-m")) {
            if ((value = option_value(&parser, "--max-solutions")) == NULL
                || !parse_long(value, 0, LONG_MAX, "--max-solutions", &number, err)) {
                return OPTIONS_ERROR;
            }
            options->max_solutions = number;
            continue;
        }
        if (option_is(arg, "--time-limit", "-t")) {
            if ((value = option_value(&parser, "--time-limit")) == NULL
                || !parse_double(value, 0.0, 1e9, "--time-limit", &options->time_limit, err)) {
                return OPTIONS_ERROR;
            }
            continue;
        }
        if (option_is(arg, "--population", "-p")) {
            if ((value = option_value(&parser, "--population")) == NULL
                || !parse_long(value, 2, 100000000L, "--population", &number, err)) {
                return OPTIONS_ERROR;
            }
            options->genetic.population = number;
            population_given = true;
            continue;
        }
        if (option_is(arg, "--mutation-rate", NULL)) {
            if ((value = option_value(&parser, "--mutation-rate")) == NULL
                || !parse_double(value, 0.0, 1.0, "--mutation-rate", &options->genetic.mutation_rate,
                                 err)) {
                return OPTIONS_ERROR;
            }
            continue;
        }
        if (option_is(arg, "--seed", NULL)) {
            if ((value = option_value(&parser, "--seed")) == NULL) {
                return OPTIONS_ERROR;
            }
            if (strcmp(value, "clock") == 0) {
                options->genetic.seed = queens_seed_from_clock();
            } else if (!parse_long(value, 0, LONG_MAX, "--seed", &number, err)) {
                return OPTIONS_ERROR;
            } else {
                options->genetic.seed = (uint64_t)number;
            }
            continue;
        }
        if (option_is(arg, "--generations", NULL)) {
            if ((value = option_value(&parser, "--generations")) == NULL
                || !parse_long(value, 0, LONG_MAX, "--generations", &number, err)) {
                return OPTIONS_ERROR;
            }
            options->genetic.max_generations = number;
            continue;
        }
        if (option_is(arg, "--stall", NULL)) {
            if ((value = option_value(&parser, "--stall")) == NULL
                || !parse_long(value, 0, LONG_MAX, "--stall", &number, err)) {
                return OPTIONS_ERROR;
            }
            options->genetic.stall_limit = number;
            continue;
        }
        if (option_is(arg, "--progress", NULL)) {
            if ((value = option_value(&parser, "--progress")) == NULL
                || !parse_long(value, 0, LONG_MAX, "--progress", &number, err)) {
                return OPTIONS_ERROR;
            }
            options->genetic.progress_every = number;
            continue;
        }

        fprintf(err, "queens: unrecognised option '%s'\n", arg);
        return OPTIONS_ERROR;
    }

    /* Only now is the board size settled, so a population the caller did not
       choose can be sized against it. */
    if (!population_given) {
        options->genetic.population = genetic_default_population(options->n);
    }

    return OPTIONS_OK;
}
