/* Copyright (c) 1992-2026 Paul McKibbin */

/* Exhaustive depth-first search: the exact enumeration the original
   place_queens() performed, but with O(1) conflict tests instead of scanning
   the whole board for every candidate square. Solutions come out in the same
   order, so historical output still validates. */
#pragma once

#include "recorder.h"
#include "runtime.h"

typedef struct {
    unsigned long long nodes; /* queens placed, i.e. search-tree nodes visited */
    bool stopped;             /* ended early: limit, deadline or interrupt */
} BacktrackStats;

BacktrackStats backtrack_solve(int n, Recorder *recorder, const Deadline *deadline);
