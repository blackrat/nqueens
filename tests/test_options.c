/* Copyright (c) 1992-2026 Paul McKibbin */

#include "check.h"
#include "options.h"

#include <stdio.h>

static FILE *devnull(void)
{
    FILE *out = fopen("/dev/null", "w");
    return out != NULL ? out : stderr;
}

#define PARSE(options, err, ...)                                                         \
    options_parse((options), (int)(sizeof((char *[]){__VA_ARGS__}) / sizeof(char *)),     \
                  (char *[]){__VA_ARGS__}, (err))

static void test_defaults(void)
{
    const Options options = options_defaults();
    CHECK_EQ_LONG(options.n, 8);
    CHECK(options.algorithm == ALGORITHM_BACKTRACK);
    CHECK_EQ_LONG(options.max_solutions, 0);
    CHECK(options.print_boards);
    CHECK(!options.show_heatmap);
}

static void test_long_and_short_forms(void)
{
    FILE *err = devnull();
    Options options;

    CHECK(PARSE(&options, err, "queens", "-n", "12") == OPTIONS_OK);
    CHECK_EQ_LONG(options.n, 12);

    CHECK(PARSE(&options, err, "queens", "--size", "10") == OPTIONS_OK);
    CHECK_EQ_LONG(options.n, 10);

    CHECK(PARSE(&options, err, "queens", "--size=6") == OPTIONS_OK);
    CHECK_EQ_LONG(options.n, 6);

    CHECK(PARSE(&options, err, "queens", "-a", "genetic", "-p", "50", "--seed", "3")
          == OPTIONS_OK);
    CHECK(options.algorithm == ALGORITHM_GENETIC);
    CHECK_EQ_LONG(options.genetic.population, 50);
    CHECK_EQ_LONG((long)options.genetic.seed, 3);

    CHECK(PARSE(&options, err, "queens", "-c", "-H") == OPTIONS_OK);
    CHECK(!options.print_boards);
    CHECK(options.show_heatmap);

    CHECK(PARSE(&options, err, "queens", "--help") == OPTIONS_HELP);
    CHECK(PARSE(&options, err, "queens", "-h") == OPTIONS_HELP);

    if (err != stderr) {
        fclose(err);
    }
}

static void test_rejects_bad_input(void)
{
    FILE *err = devnull();
    Options options;

    CHECK(PARSE(&options, err, "queens", "--size", "0") == OPTIONS_ERROR);
    CHECK(PARSE(&options, err, "queens", "--size", "banana") == OPTIONS_ERROR);
    CHECK(PARSE(&options, err, "queens", "--size", "8x") == OPTIONS_ERROR);
    CHECK(PARSE(&options, err, "queens", "--size", "99999") == OPTIONS_ERROR);
    CHECK(PARSE(&options, err, "queens", "--size") == OPTIONS_ERROR);
    CHECK(PARSE(&options, err, "queens", "--algorithm", "psychic") == OPTIONS_ERROR);
    CHECK(PARSE(&options, err, "queens", "--mutation-rate", "2") == OPTIONS_ERROR);
    CHECK(PARSE(&options, err, "queens", "--population", "1") == OPTIONS_ERROR);
    CHECK(PARSE(&options, err, "queens", "--time-limit", "-1") == OPTIONS_ERROR);
    CHECK(PARSE(&options, err, "queens", "--nonsense") == OPTIONS_ERROR);

    if (err != stderr) {
        fclose(err);
    }
}

int main(void)
{
    test_defaults();
    test_long_and_short_forms();
    test_rejects_bad_input();
    return CHECK_REPORT();
}
