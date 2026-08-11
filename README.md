# nqueens

Place *n* non-attacking queens on an *n*x*n* board, by exhaustive backtracking
or by genetic search, and measure which is faster where.

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

Requires CMake 3.20+ and a C17 compiler. Useful switches:

| Option | Default | Effect |
| --- | --- | --- |
| `-DNQUEENS_BUILD_TESTS=OFF` | `ON` | skip the test suite |
| `-DNQUEENS_WERROR=ON` | `OFF` | warnings become errors |
| `-DNQUEENS_SANITIZE=ON` | `OFF` | address + undefined-behaviour sanitizers |

## Use

```
usage: nqueens [options]

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
nqueens -n 8                      # all 92 solutions, printed
nqueens -n 12 -c                  # count them: 14200
nqueens -n 8 -c -H                # square occupancy across all 92
nqueens -a genetic -n 20 -m 1     # one 20-queens solution by evolution
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

`tools/bench.py` times how long each solver takes to reach its *first* solution.
It needs only python3; the charts are SVG, written directly.

```sh
tools/bench.py crossover     # time to first solution vs board size
tools/bench.py population    # time to first solution vs population
tools/bench.py chart         # render bench/*.svg and bench/report.html
```

Each run is capped by `--limit`, 180s by default, and each sweep by `--budget`.
Rows are flushed to CSV as they are measured, so an interrupted sweep still
leaves usable data and `--resume` continues it.

Backtracking is deterministic, so each size is the median of repeated runs. The
genetic search is not, and it dead-ends, so each size is several runs from fixed
seeds. It is reported two ways: the mean over runs that succeeded, and the total
time spent per solution actually obtained. The second includes the failures.

Results, and the machine and binary that produced them, are in `bench/`.

### Board size

Backtracking is about a thousand times faster at n=8 and keeps the lead to n=29.
Its cost does not grow smoothly. Whether the first solution sits early or late
in the depth-first order has nothing to do with board size, so n=30 takes 5s
while n=31 takes 1s, and n=36 exceeds 180s while n=37 finishes in 93s.

Genetic search leads from n=30, that being the smallest size past which it wins
at every size measured. Between n=20 and n=29 the two swap places repeatedly, so
a single crossing point would be misleading.

Past n=37 backtracking exceeds 180s at most sizes. Genetic search still returns
a solution in about 200ms, and runs out around n=160: 3 successes in 7,
averaging 14s.

### Population size

Population swept from 25 to 6400 across five board sizes, nine runs each.

| n | cheapest | ratio | per solution | cheapest winning 2 in 3 | ratio |
| --- | --- | --- | --- | --- | --- |
| 20 | 25 | 1.2x n | 4ms | 25 | 1.2x n |
| 40 | 50 | 1.2x n | 46ms | 50 | 1.2x n |
| 80 | 400 | 5.0x n | 724ms | 400 | 5.0x n |
| 120 | 25 | 0.2x n | 4s | 1600 | 13.3x n |
| 160 | 800 | 5.0x n | 19s | 800 | 5.0x n |

Above the knee, cost per solution rises linearly with population and buys
nothing: at n=40, populations of 800/1600/3200/6400 cost 208ms, 370ms, 401ms and
654ms. Below it the success rate collapses and the retries absorb the saving.
The knee moves right as n grows, from about 1x n up to n=40 to about 5x n from
n=80 on.

The low-population figures rest on few successes - population 25 at n=120 won
one run in nine - so that region is noisy. Anything from 1x n to 10x n sits
inside it.

Because a stalled run is abandoned cheaply, a small population that restarts on
failure competes with a large one that rarely fails. At n=120, population 25
costs about 4s per solution against 13s for population 3200.

The default is 10n, floored at 400: the smallest population that wins reliably,
since the program does not retry for you. `-p` overrides it.
