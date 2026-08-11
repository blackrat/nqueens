/* Copyright (c) 1992-2026 Paul McKibbin */

#include "board.h"

#include "check.h"

#include <stdlib.h>
#include <string.h>

static void test_attackers(void)
{
    /* Every queen on the leading diagonal attacks every other. */
    const int diagonal[4] = {0, 1, 2, 3};
    for (int row = 0; row < 4; row++) {
        CHECK_EQ_LONG(queens_attackers(diagonal, 4, row), 3);
    }
    CHECK(!queens_is_solution(diagonal, 4));

    /* Same column, no diagonal contact. */
    const int column[4] = {2, 2, 2, 2};
    CHECK_EQ_LONG(queens_attackers(column, 4, 0), 3);

    /* The two solutions of the 4-queens board. */
    const int solved[4] = {1, 3, 0, 2};
    for (int row = 0; row < 4; row++) {
        CHECK_EQ_LONG(queens_attackers(solved, 4, row), 0);
    }
    CHECK(queens_is_solution(solved, 4));

    const int mirrored[4] = {2, 0, 3, 1};
    CHECK(queens_is_solution(mirrored, 4));

    /* Anti-diagonal contact only. */
    const int anti[3] = {0, 2, 1};
    CHECK_EQ_LONG(queens_attackers(anti, 3, 1), 1);
    CHECK_EQ_LONG(queens_attackers(anti, 3, 2), 1);
    CHECK_EQ_LONG(queens_attackers(anti, 3, 0), 0);
}

static void test_print(void)
{
    const int solved[4] = {1, 3, 0, 2};
    char buffer[64] = {0};
    FILE *out = tmpfile();

    CHECK(out != NULL);
    if (out == NULL) {
        return;
    }
    queens_print(out, solved, 4);
    rewind(out);
    const size_t read = fread(buffer, 1, sizeof buffer - 1, out);
    buffer[read] = '\0';
    CHECK(strcmp(buffer, ".Q..\n...Q\nQ...\n..Q.\n") == 0);
    fclose(out);
}

static void test_heatmap(void)
{
    Heatmap heatmap;
    heatmap_init(&heatmap, 4);

    const int a[4] = {1, 3, 0, 2};
    const int b[4] = {2, 0, 3, 1};
    heatmap_add(&heatmap, a);
    heatmap_add(&heatmap, b);

    unsigned long total = 0;
    for (int i = 0; i < 16; i++) {
        total += heatmap.counts[i];
    }
    CHECK_EQ_LONG(total, 8);
    CHECK_EQ_LONG(heatmap.counts[0 * 4 + 1], 1); /* row 0, col 1: from a only */
    CHECK_EQ_LONG(heatmap.counts[0 * 4 + 2], 1); /* row 0, col 2: from b only */
    CHECK_EQ_LONG(heatmap.counts[0 * 4 + 0], 0);

    heatmap_free(&heatmap);
    CHECK(heatmap.counts == NULL);
}

int main(void)
{
    test_attackers();
    test_print();
    test_heatmap();
    return CHECK_REPORT();
}
