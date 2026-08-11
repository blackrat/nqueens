# queens

Place *n* non-attacking queens on an *n*×*n* board, by exhaustive backtracking
or by genetic search.

Originally a Borland C / DJGPP / Watcom program from around 1992. See
[History](#history) for what changed and why, and [PROVENANCE.md](PROVENANCE.md)
for the evidence behind that date.

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

Requires CMake 3.20+ and a C17 compiler. Useful switches:

| Option | Default | Effect |
| --- | --- | --- |
| `-DQUEENS_BUILD_TESTS=OFF` | `ON` | skip the test suite |
| `-DQUEENS_WERROR=ON` | `OFF` | warnings become errors |
| `-DQUEENS_SANITIZE=ON` | `OFF` | address + undefined-behaviour sanitizers |

## Use

```
usage: queens [options]

  -n, --size N            board size (default 8, max 1000)
  -a, --algorithm NAME    backtrack (exhaustive) or genetic (default backtrack)
  -m, --max-solutions N   stop after N solutions (default: all)
  -t, --time-limit SECS   wall-clock budget (default: none)
  -c, --count-only        report totals, do not print boards
  -H, --heatmap           print how often each square was used
  -h, --help              this message

genetic only:
  -p, --population N      individuals per generation (default 10n, min 400)
      --mutation-rate F   per-child mutation probability (default 0.9)
      --seed N|clock      RNG seed (default: from the clock, so runs differ);
                          pass the seed printed in the summary to repeat a run
      --generations N     stop after N generations (default: no limit)
      --stall N           give up after N generations without a fitness gain,
                          0 to never (default 2000)
      --progress N        progress line every N generations, 0 to silence
```

While a search runs:

| Key | Effect |
| --- | --- |
| any key | show where it has got to — the last solution found, or for the genetic search the best board in the population |
| `q` or ESC | stop |
| Ctrl-C | stop |

However it ends, you still get the summary. This is the original's `kbhit()`
peek, which was the only way to watch a run on a machine that took minutes to
reach n=12. It needs a terminal: when stdin is a pipe or a test harness,
polling is switched off and nothing is read from stdin.

Genetic runs print the seed they used, because the default seed comes from the
clock. Pass it back with `--seed` to replay a run exactly.

```sh
queens -n 8                      # all 92 solutions, printed
queens -n 12 -c                  # count them: 14200
queens -n 8 -c -H                # square occupancy across all 92
queens -a genetic -n 20 -m 1     # one 20-queens solution by evolution
```

Exit status is 0 when the search ran to its own conclusion, 1 when it was cut
short by `--time-limit`, `q`/ESC or Ctrl-C. A board with no solutions (`-n 3`)
is a complete search, so it exits 0.

## How it works

**Backtracking** walks rows top to bottom, trying columns left to right, and
keeps occupancy flags for each column and each of the two diagonal families —
so rejecting a square is one array lookup rather than a scan of the board. It
enumerates in the same order the original did, and `tests/data/queens8.txt` is
the original program's own 8-queens output, held as a regression fixture.

**Genetic search** encodes a placement as one column index per row. Fitness
scores each row by how few queens attack it, doubling rows that are clean; a
solution scores `2n³`. Each generation the ranked bottom half is replaced by
one-point-crossover children of randomly chosen top-half parents, each child
mutating one row with probability `--mutation-rate`. It is not exhaustive: it
stops when it stalls (no fitness gain for `--stall` generations), when it runs
out of budget, or when it has found as many solutions as you asked for. At the defaults it finds a
solution for boards up to about n=40 in well under a second.

## Benchmarks

`tools/bench.py` times each solver's route to a *first* solution and charts the
result. It needs nothing but python3 — the charts are SVG written directly.

```sh
tools/bench.py crossover     # time to first solution vs board size, both solvers
tools/bench.py population    # time to first solution vs population, genetic only
tools/bench.py chart         # render bench/*.svg and bench/report.html
```

Every run is capped by `--limit` (3 minutes by default, which is what decides
how far the sweep goes) and each sweep has an overall `--budget`. Rows are
flushed to CSV as they are measured, so an interrupted sweep still leaves usable
data and `--resume` picks up where it stopped.

Backtracking is deterministic, so each size is the median of repeated runs. The
genetic search is neither deterministic nor reliable — it dead-ends — so each
size is several runs with fixed, reproducible seeds, and the charts carry two
genetic lines: the mean over the runs that succeeded (what you feel when it
works) and the total time spent per solution actually obtained (what it costs
you including the failures). The second is the one to compare against
backtracking.

Results, and the machine they came from, are in `bench/`. They were measured
before the random source was replaced on 2026-08-11 (see
[PROVENANCE.md](PROVENANCE.md)), so a given `--seed` no longer replays those
exact runs - re-run the sweeps to regenerate them. Aggregate behaviour was
re-measured after the change and is unaltered; see `bench/rng-change.txt`. On an Apple M-series
laptop, timing the route to a first solution:

*(Charts are being regenerated against the current build - the figures below
were measured with an earlier random source, as noted above. Run
`tools/bench.py crossover && tools/bench.py chart` to produce them yourself.)*

Backtracking starts a thousand times faster and stays ahead to about n=29. It
does not degrade smoothly, though: its cost saw-tooths violently, because
whether the first solution sits early or late in the depth-first order has
nothing to do with the size of the board. n=30 costs 5s while n=31 costs 1s;
n=36 blew the 3-minute cap while n=37 finished in 93s.

Genetic search overtakes it for good at **n=30**, meaning that is the smallest
size past which it wins at every size measured. Calling any single size "the"
crossover would be tidier than the data deserves - the two lines swap places
several times between n=20 and n=29.

Past n=37 the comparison ends: backtracking cannot find a first solution inside
3 minutes at most sizes, while the genetic search is still returning one in
about 200ms. It has its own ceiling, just much further out - around n=160, where
it drops to 3 successful runs in 7 and averages 14s per win.

### Population size

*(Chart regenerating - see the note above. `tools/bench.py population` then
`tools/bench.py chart` reproduces it.)*

Sweeping population from 25 to 6400 across five board sizes, nine runs each:

| n | cheapest | ratio | per solution | cheapest that wins 2 in 3 | ratio |
| --- | --- | --- | --- | --- | --- |
| 20 | 25 | 1.2x n | 4ms | 25 | 1.2x n |
| 40 | 50 | 1.2x n | 46ms | 50 | 1.2x n |
| 80 | 400 | 5.0x n | 724ms | 400 | 5.0x n |
| 120 | 25 | 0.2x n | 4s | 1600 | 13.3x n |
| 160 | 800 | 5.0x n | 19s | 800 | 5.0x n |

The curve is an L, not a U. **Above the knee, cost grows linearly with
population and buys nothing** - at n=40, populations of 800/1600/3200/6400 cost
208ms/370ms/401ms/654ms per solution. Cost per generation is proportional to
population, and the generations needed barely fall. **Below the knee the
success rate collapses**, and the retries eat the saving.

So the population you want is the smallest one that still wins, and that knee
moves right as n grows: about 1x n up to n=40, about 5x n from n=80 on.

The default does **not** follow that line, though, because this program does not
restart a failed search for you. The cost-per-solution column above assumes a
retry loop; without one, a default that stalls one run in three is a worse
experience than one that is slightly slower and actually finds the thing. The
win rate settles around 10x n, so that is the default, with a floor of 400 for
small boards. `-p` overrides it when you want the cheap-and-retry behaviour.

Measured against the old fixed 1000, twelve runs per size, that rule is a clear
win on small boards and a wash on large ones:

| n | fixed 1000 | 10n (floor 400) |
| --- | --- | --- |
| 20 | 11/12, 38ms | 10/12, 27ms |
| 40 | 11/12, 332ms | 11/12, 147ms |
| 80 | 12/12, 1.3s | 9/12, 2.6s |
| 120 | 7/12, 20s | 9/12, 15s |
| 160 | 9/12, 31s | 7/12, 45s |

Do not read more into the n=80 and n=160 rows than is there. A population of 800
measured 9 wins in 9 in the sweep above and 9 in 12 here - same population,
different seeds. That spread *is* the noise floor, and the gaps at n=80 and
n=160 sit inside it. The defensible claims are the small-board speed-up, where
the effect is large and consistent, and that a population expressed as a
multiple of n stays sane at board sizes nobody benchmarked: a fixed 1000 is 125x
n at n=8 and only 2x n at n=500, which is well under the knee.

Two caveats on those numbers. The n=120 "cheapest" row is an artefact worth
seeing: population 25 won one run in nine, so its cost per solution rests on a
single sample. That is why the table carries a second criterion. And the whole
low-population region is noisy for the same reason - the honest reading is that
anything from 1x n to 10x n is within the noise, not that 1.2x n is precisely
optimal.

The one genuinely interesting result: because stall detection cuts a failed run
off cheaply, **a small population that restarts on failure is competitive with a
large one that rarely fails**. At n=120, population 25 restarting costs about 4s
per solution against 13s for a population of 3200 that wins every time.

## Layout

```
src/runtime.*    clock, time budget, interrupt flag, allocation
src/board.*      placement primitives, rendering, occupancy heatmap
src/recorder.*   where solvers hand in solutions: dedupe, count, limit, print
src/backtrack.*  exhaustive depth-first search
src/genetic.*    evolutionary search
src/keys.*       non-blocking key polling (termios, _kbhit, or nothing)
src/options.*    command-line parsing
src/main.c       wiring and reporting
```

## History

The original was five `.c` files — `8qrecurs.c`, `8qalife.c`, `work.c`,
`gnuwork.c`, `test1.c`. Four of them are near-identical and carry *both*
solvers, recursive and genetic, in one source behind `#ifdef GENETIC`, differing
only in `#define`d constants and which branch was left live; `test1.c` is an
earlier genetic-only program with no recursive path at all. Board size,
population, time budget and the choice of algorithm were all compile-time, so
trying a different board size meant editing a source file, and trying a
different *combination* meant keeping another copy of the whole program. They
are one program with runtime options now; that is the bulk of the change.

The single-purpose filenames (`8qrecurs.c` versus `8qalife.c`) came later than
the dual-algorithm sources, splitting the program so that building one or the
other no longer meant toggling a flag. That ordering matters for dating, and is
set out in [PROVENANCE.md](PROVENANCE.md).

Also gone: `<alloc.h>` and `coreleft()`, a vendored 1990 BSD `qsort.c` (the C
library's is fine), and the Turbo C project and debugger files (`8q.prj`,
`8q.tfa`, `tdconfig.td`, `tfconfig.tf`, `delta.mvm`).

The `<conio.h>` interaction survives in `src/keys.*`, rebuilt on termios rather
than DOS. What did not survive is the original's `while (!kbhit()) {}`
busy-waits, which blocked after every solution until you pressed something.

Bugs fixed along the way:

- `srand(NULL)` — `SEED` was `#define`d to `NULL` unless `RANDSEED` was set, so
  the pointer constant was passed where a seed was wanted.
- Solutions were stored in a fixed `boards[92]` array sized for the 8-queens
  answer, while `HSIZE` was 18 or 24 in most variants. Finding a 93rd solution
  wrote past the end.
- `unique_board()` copied `NUMBEROFQUEENS + 1` bytes out of a
  `NUMBEROFQUEENS`-byte allocation, and ignored `malloc` failure before writing
  through the result.
- The breeding loop stepped `i += 2` and wrote to `gene[i + 1]` without checking
  the bound, so an odd population ran off the end of the array.
- `main()` was implicitly `int` with no return type, and prototypes disagreed
  with definitions (`main(void)` declared, `main()` defined).

Three deliberate behaviour changes, the first two to make the genetic search
actually finish:

- The mutation rate defaults to 0.9 per child rather than the original's
  ~0.0001. At the original rate the population converged almost immediately and
  then sat there.
- The original declared the run dead the moment the surviving half of the
  population shared one genome, and called `exit(-1)`. That is premature once
  mutation is doing real work, so the give-up test is now "no improvement in the
  best fitness for `--stall` generations" (default 2000), and it reports rather
  than aborts.

- The seed is taken from the clock unless `--seed` says otherwise, so runs differ
  by default and you have to opt in to repeating one. The original had this
  backwards: it seeded from a fixed `NULL` unless `RANDSEED` was `#define`d at
  compile time, so every run of a given build was identical.

`--mutation-rate 0.0001 --seed 0` gets you the historical tuning back,
stagnation and all.

## Copyright and licence

Copyright (c) 1992-2026 Paul McKibbin. Released under the MIT licence - see
[LICENSE](LICENSE). Use it, learn from it, build on it; the one thing the
licence asks is that the copyright notice travels with it, so the work stays
attributed.

The 1992 start date is evidenced rather than assumed - see
[PROVENANCE.md](PROVENANCE.md). No third-party code remains in the tree, so
the licence covers all of it cleanly.
