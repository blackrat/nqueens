/* Copyright (c) 1992-2026 Paul McKibbin */

#include "backtrack.h"

#include "keys.h"

#include <stdlib.h>

/* Checking the clock costs more than a search node, so only look every so
   often. The mask is one less than a power of two. */
#define CLOCK_CHECK_MASK 0xFFFFu

typedef struct {
    int n;
    int *queens;
    bool *column;
    bool *diagonal;      /* indexed by row + col,         2n-1 entries */
    bool *anti_diagonal; /* indexed by row - col + n - 1, 2n-1 entries */
    Recorder *recorder;
    const Deadline *deadline;
    unsigned long long nodes;
    bool stopped;
} Search;

/* Any key shows the last solution found; ESC or q gives up. This is the
   original's kbhit() peek, which was the only way to see a long run's progress
   on a machine that took minutes to reach n=12. */
static bool handle_key(Search *search)
{
    const int key = keys_poll();

    if (key == KEY_NONE) {
        return false;
    }
    if (key_is_quit(key)) {
        queens_quit_key = true;
        return true;
    }
    recorder_show_last(search->recorder);
    return false;
}

static bool should_stop(Search *search)
{
    return queens_interrupted != 0 || deadline_expired(search->deadline) || handle_key(search);
}

static void place(Search *search, int row)
{
    if (row == search->n) {
        recorder_submit(search->recorder, search->queens);
        if (recorder_limit_reached(search->recorder)) {
            search->stopped = true;
        }
        return;
    }

    for (int col = 0; col < search->n; col++) {
        const int diagonal = row + col;
        const int anti_diagonal = row - col + search->n - 1;

        if (search->column[col] || search->diagonal[diagonal]
            || search->anti_diagonal[anti_diagonal]) {
            continue;
        }

        search->column[col] = true;
        search->diagonal[diagonal] = true;
        search->anti_diagonal[anti_diagonal] = true;
        search->queens[row] = col;

        if ((++search->nodes & CLOCK_CHECK_MASK) == 0 && should_stop(search)) {
            search->stopped = true;
        }
        if (!search->stopped) {
            place(search, row + 1);
        }

        search->column[col] = false;
        search->diagonal[diagonal] = false;
        search->anti_diagonal[anti_diagonal] = false;

        if (search->stopped) {
            return;
        }
    }
}

BacktrackStats backtrack_solve(int n, Recorder *recorder, const Deadline *deadline)
{
    const size_t diagonals = (size_t)(2 * n - 1);
    Search search = {
        .n = n,
        .queens = queens_alloc((size_t)n, sizeof(int)),
        .column = queens_alloc((size_t)n, sizeof(bool)),
        .diagonal = queens_alloc(diagonals, sizeof(bool)),
        .anti_diagonal = queens_alloc(diagonals, sizeof(bool)),
        .recorder = recorder,
        .deadline = deadline,
    };

    if (!recorder_limit_reached(recorder) && !should_stop(&search)) {
        place(&search, 0);
    }

    const BacktrackStats stats = {
        .nodes = search.nodes,
        .stopped = search.stopped, /* i.e. the space was not exhausted */
    };

    free(search.queens);
    free(search.column);
    free(search.diagonal);
    free(search.anti_diagonal);
    return stats;
}
