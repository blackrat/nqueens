/* Copyright (c) 1992-2026 Paul McKibbin */

#include "backtrack.h"
#include "board.h"
#include "check.h"
#include "recorder.h"
#include "runtime.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    char (*rows)[16]; /* one digit per row, so n <= 15 here */
    size_t len;
    size_t cap;
    int invalid;
    int n;
} Collected;

static void collect(void *context, const int *queens, int n, long index)
{
    Collected *collected = context;
    (void)index;

    if (!queens_is_solution(queens, n)) {
        collected->invalid++;
    }
    if (collected->len == collected->cap) {
        collected->cap = collected->cap != 0 ? collected->cap * 2 : 128;
        collected->rows = realloc(collected->rows, collected->cap * sizeof *collected->rows);
        if (collected->rows == NULL) {
            abort();
        }
    }
    char *row = collected->rows[collected->len++];
    for (int i = 0; i < n && i < 15; i++) {
        row[i] = (char)('0' + queens[i]);
    }
    row[n < 15 ? n : 15] = '\0';
}

static Recorder make_recorder(int n, Collected *collected)
{
    Recorder recorder;
    recorder_init(&recorder, n, stdout);
    recorder.print_boards = false;
    recorder.hook = collect;
    recorder.hook_context = collected;
    collected->n = n;
    return recorder;
}

static long count_solutions(int n)
{
    Collected collected = {0};
    Recorder recorder = make_recorder(n, &collected);
    const Deadline deadline = deadline_start(0.0);

    backtrack_solve(n, &recorder, &deadline);
    const long count = recorder.count;

    CHECK_EQ_LONG(collected.invalid, 0);
    CHECK_EQ_LONG((long)collected.len, count);
    free(collected.rows);
    recorder_free(&recorder);
    return count;
}

static void test_known_counts(void)
{
    /* OEIS A000170. */
    static const long expected[] = {0, 1, 0, 0, 2, 10, 4, 40, 92, 352, 724};

    for (int n = 1; n <= 10; n++) {
        CHECK_EQ_LONG(count_solutions(n), expected[n]);
    }
}

static void test_solution_limit(void)
{
    Collected collected = {0};
    Recorder recorder = make_recorder(8, &collected);
    recorder.limit = 5;
    const Deadline deadline = deadline_start(0.0);

    const BacktrackStats stats = backtrack_solve(8, &recorder, &deadline);

    CHECK_EQ_LONG(recorder.count, 5);
    CHECK(stats.stopped);
    CHECK(recorder_limit_reached(&recorder));

    free(collected.rows);
    recorder_free(&recorder);
}

/* The 1990s program's output is checked in as tests/data/queens8.txt. The
   rewrite must reproduce it solution for solution, in the same order. */
static void test_matches_historical_output(const char *fixture_path)
{
    FILE *fixture = fopen(fixture_path, "r");
    CHECK(fixture != NULL);
    if (fixture == NULL) {
        fprintf(stderr, "cannot open fixture %s\n", fixture_path);
        return;
    }

    Collected collected = {0};
    Recorder recorder = make_recorder(8, &collected);
    const Deadline deadline = deadline_start(0.0);
    backtrack_solve(8, &recorder, &deadline);

    char line[64];
    size_t index = 0;
    while (fgets(line, sizeof line, fixture) != NULL) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0') {
            continue;
        }
        if (index >= collected.len) {
            check_failures++;
            fprintf(stderr, "fixture has more solutions than the solver produced\n");
            break;
        }
        if (strcmp(line, collected.rows[index]) != 0) {
            check_failures++;
            fprintf(stderr, "solution %zu: fixture %s, solver %s\n", index + 1, line,
                    collected.rows[index]);
        }
        index++;
    }
    CHECK_EQ_LONG((long)index, (long)collected.len);
    CHECK_EQ_LONG((long)index, 92);

    fclose(fixture);
    free(collected.rows);
    recorder_free(&recorder);
}

int main(int argc, char **argv)
{
    test_known_counts();
    test_solution_limit();
    if (argc > 1) {
        test_matches_historical_output(argv[1]);
    } else {
        fprintf(stderr, "no fixture path given; skipping historical comparison\n");
        return 2;
    }
    return CHECK_REPORT();
}
