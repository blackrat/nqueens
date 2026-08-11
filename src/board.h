/* Copyright (c) 1992-2026 Paul McKibbin */

/* Board primitives.

   A placement is an array of `n` ints: queens[row] is the column occupied by
   the queen on that row. One queen per row is baked into the representation,
   so both solvers speak it and neither can produce a row conflict. */
#pragma once

#include <stdbool.h>
#include <stdio.h>

/* How many other queens attack the one on `row` (columns and diagonals). */
int queens_attackers(const int *queens, int n, int row);

/* True when no queen attacks any other. */
bool queens_is_solution(const int *queens, int n);

/* n lines of 'Q' and '.', as the original printed them. */
void queens_print(FILE *out, const int *queens, int n);

/* Cumulative occupancy across every placement recorded, which is what the
   original called the "cum_board": how often each square held a queen. */
typedef struct {
    int n;
    unsigned long *counts; /* n * n, row-major */
} Heatmap;

void heatmap_init(Heatmap *heatmap, int n);
void heatmap_free(Heatmap *heatmap);
void heatmap_add(Heatmap *heatmap, const int *queens);
void heatmap_print(FILE *out, const Heatmap *heatmap);
