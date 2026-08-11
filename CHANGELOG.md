# Changelog

Copyright (c) 1992-2026 Paul McKibbin

## 2026 - rewritten for C17 and CMake

The program was five near-identical `.c` files selected by editing `#define`s.
It is one program with runtime options now, built by CMake, with a test suite.
See [HISTORY.md](HISTORY.md) for what the original looked like.

### Removed

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

### Random source

Randomness comes from the C library's `rand()`, as it did originally. Two
drafts of this rewrite implemented a generator instead - PCG32, then an additive
lagged Fibonacci - and both were dropped so that no third-party algorithm
remains in the tree. Measured over twelve seeds per board size, `rand()` was the
most reliable of the three, winning 11 of 12 runs at n=20, 40, 80 and 120 where
PCG32 managed 9 of 12 at two of those.

Benchmark data recorded before that change does not replay from the same
`--seed`, since the generator differs. `bench/environment.txt` records the
SHA-256 of the binary that produced each set of results.

### Default population

The default population is `10n`, floored at 400, rather than a fixed 1000.
Cost per solution grows linearly with population once it is large enough to be
reliable, so the ideal is the smallest population that still wins - but this
program does not restart a failed search, so a default that stalls one run in
three is worse than one slightly slower that finds it. The win rate settles
around `10n`.

Measured against the old fixed 1000, twelve runs per size:

| n | fixed 1000 | 10n (floor 400) |
| --- | --- | --- |
| 20 | 11/12, 38ms | 10/12, 27ms |
| 40 | 11/12, 332ms | 11/12, 147ms |
| 80 | 12/12, 1.3s | 9/12, 2.6s |
| 120 | 7/12, 20s | 9/12, 15s |
| 160 | 9/12, 31s | 7/12, 45s |

Do not read more into the n=80 and n=160 rows than is there. A population of 800
measured 9 wins in 9 in one sweep and 9 in 12 in another - same population,
different seeds. That spread *is* the noise floor, and those two gaps sit inside
it. What survives is the small-board speed-up, which is large and consistent,
and that a population expressed as a multiple of n stays sane at board sizes
nobody benchmarked: a fixed 1000 is 125x n at n=8 and only 2x n at n=500.

### Licence

MIT, added 2026.
