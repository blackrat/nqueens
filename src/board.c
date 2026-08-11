/* Copyright (c) 1992-2026 Paul McKibbin */

#include "board.h"

#include "runtime.h"

#include <stdlib.h>
#include <string.h>

int queens_attackers(const int *queens, int n, int row)
{
    const int col = queens[row];
    int attackers = 0;

    for (int other = 0; other < n; other++) {
        if (other == row) {
            continue;
        }
        const int other_col = queens[other];
        if (other_col == col                     /* same column   */
            || other - other_col == row - col    /* same diagonal */
            || other + other_col == row + col) { /* same anti-diagonal */
            attackers++;
        }
    }
    return attackers;
}

bool queens_is_solution(const int *queens, int n)
{
    for (int row = 0; row < n; row++) {
        if (queens_attackers(queens, n, row) != 0) {
            return false;
        }
    }
    return true;
}

void queens_print(FILE *out, const int *queens, int n)
{
    char *line = queens_alloc((size_t)n + 1, sizeof *line);

    line[n] = '\0';
    for (int row = 0; row < n; row++) {
        memset(line, '.', (size_t)n);
        line[queens[row]] = 'Q';
        fprintf(out, "%s\n", line);
    }
    free(line);
}

void heatmap_init(Heatmap *heatmap, int n)
{
    heatmap->n = n;
    heatmap->counts = queens_alloc((size_t)n * (size_t)n, sizeof *heatmap->counts);
}

void heatmap_free(Heatmap *heatmap)
{
    free(heatmap->counts);
    heatmap->counts = NULL;
    heatmap->n = 0;
}

void heatmap_add(Heatmap *heatmap, const int *queens)
{
    for (int row = 0; row < heatmap->n; row++) {
        heatmap->counts[(size_t)row * (size_t)heatmap->n + (size_t)queens[row]]++;
    }
}

void heatmap_print(FILE *out, const Heatmap *heatmap)
{
    unsigned long widest = 0;
    for (int i = 0; i < heatmap->n * heatmap->n; i++) {
        if (heatmap->counts[i] > widest) {
            widest = heatmap->counts[i];
        }
    }

    int width = 1;
    for (unsigned long scale = 10; scale <= widest; scale *= 10) {
        width++;
    }

    for (int row = 0; row < heatmap->n; row++) {
        for (int col = 0; col < heatmap->n; col++) {
            fprintf(out, "%*lu%s", width,
                    heatmap->counts[(size_t)row * (size_t)heatmap->n + (size_t)col],
                    col + 1 < heatmap->n ? " " : "");
        }
        fputc('\n', out);
    }
}
