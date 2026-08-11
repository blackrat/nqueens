/* Copyright (c) 1992-2026 Paul McKibbin */

/* A test harness small enough to read in one sitting: CHECK records a failure
   and carries on, so one run reports every problem rather than the first. */
#pragma once

#include <stdio.h>

static int check_failures = 0;

#define CHECK(condition)                                                                 \
    do {                                                                                 \
        if (!(condition)) {                                                              \
            check_failures++;                                                            \
            fprintf(stderr, "%s:%d: FAIL %s\n", __FILE__, __LINE__, #condition);         \
        }                                                                                \
    } while (0)

#define CHECK_EQ_LONG(actual, expected)                                                  \
    do {                                                                                 \
        const long check_a = (long)(actual);                                             \
        const long check_e = (long)(expected);                                           \
        if (check_a != check_e) {                                                        \
            check_failures++;                                                            \
            fprintf(stderr, "%s:%d: FAIL %s: expected %ld, got %ld\n", __FILE__,         \
                    __LINE__, #actual, check_e, check_a);                                \
        }                                                                                \
    } while (0)

#define CHECK_REPORT()                                                                   \
    (check_failures == 0 ? (printf("%s: all checks passed\n", __FILE__), 0)              \
                         : (fprintf(stderr, "%s: %d check(s) failed\n", __FILE__,        \
                                    check_failures),                                     \
                            1))
