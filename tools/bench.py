#!/usr/bin/env python3
# Copyright (c) 1992-2026 Paul McKibbin
"""Time each solver's route to a *first* solution, and chart where they cross.

Two sweeps:

  crossover   time to first solution against board size, for both algorithms.
              Backtracking is deterministic, so one run per size says it all.
              The genetic search is not, and it dead-ends: it is run several
              times per size and only the successful runs are averaged.

  population  time to first solution against population size, for a handful of
              board sizes, to find the population that suits a given n.

Both write CSV as they go, so a sweep that is interrupted still leaves usable
data, and both can resume from an existing CSV.

  tools/bench.py crossover
  tools/bench.py population
  tools/bench.py chart
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import math
import platform
import re
import statistics
import subprocess
import sys
import time
from dataclasses import dataclass, asdict
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
DEFAULT_BINARY = REPO / "build" / "nqueens"
DEFAULT_OUT = REPO / "bench"

SUMMARY = re.compile(
    r"^(?P<solutions>\d+) solutions? for n=(?P<n>\d+) in (?P<seconds>[\d.eE+-]+)s "
    r"\((?P<detail>.*)\)$"
)
NODES = re.compile(r"(?P<nodes>\d+) nodes")
GENERATIONS = re.compile(r"(?P<generations>\d+) generations")

FIELDS = [
    "sweep",
    "algorithm",
    "n",
    "population",
    "trial",
    "seed",
    "solved",
    "outcome",
    "seconds",
    "work",
]


@dataclass
class Run:
    sweep: str
    algorithm: str
    n: int
    population: int
    trial: int
    seed: int
    solved: int
    outcome: str  # solved | stalled | timeout
    seconds: float
    work: int


class Recorder:
    """Appends rows to a CSV, flushing each one."""

    def __init__(self, path: Path, resume: bool):
        self.path = path
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.done: set[tuple] = set()

        if resume and self.path.exists():
            for row in read_csv(self.path):
                self.done.add(self._key(row))
            self.handle = self.path.open("a", newline="")
            self.writer = csv.DictWriter(self.handle, fieldnames=FIELDS)
        else:
            self.handle = self.path.open("w", newline="")
            self.writer = csv.DictWriter(self.handle, fieldnames=FIELDS)
            self.writer.writeheader()
            self.handle.flush()

    @staticmethod
    def _key(row: dict) -> tuple:
        return (row["sweep"], row["algorithm"], int(row["n"]),
                int(row["population"]), int(row["trial"]))

    def already_have(self, run_key: tuple) -> bool:
        return run_key in self.done

    def add(self, run: Run) -> None:
        self.writer.writerow(asdict(run))
        self.handle.flush()

    def close(self) -> None:
        self.handle.close()


def record_environment(out_dir: Path, binary: Path | None = None) -> None:
    """Timings mean nothing without the machine, so write it down beside them.

    The binary's checksum is included because results depend on which build
    produced them - changing the random source alone changes every genetic run.
    """
    lines = [f"platform: {platform.platform()}",
             f"machine: {platform.machine()}",
             f"python: {platform.python_version()}"]
    if binary is not None and binary.exists():
        digest = hashlib.sha256(binary.read_bytes()).hexdigest()
        lines.append(f"binary: {binary.name} sha256={digest}")
    for label, command in (("cpu", ["sysctl", "-n", "machdep.cpu.brand_string"]),
                           ("cores", ["sysctl", "-n", "hw.ncpu"])):
        try:
            value = subprocess.run(command, capture_output=True, text=True,
                                   timeout=5).stdout.strip()
            if value:
                lines.append(f"{label}: {value}")
        except (OSError, subprocess.SubprocessError):
            pass
    (out_dir / "environment.txt").write_text("\n".join(lines) + "\n")


def read_csv(path: Path) -> list[dict]:
    with path.open(newline="") as handle:
        return list(csv.DictReader(handle))


def solve(binary: Path, algorithm: str, n: int, limit: float,
          seed: int | None = None, population: int | None = None) -> tuple[bool, float, int]:
    """One run. Returns (solved, seconds, work)."""
    command = [str(binary), "-a", algorithm, "-n", str(n), "-m", "1", "-c",
               "-t", str(limit)]
    if algorithm == "genetic":
        command += ["--progress", "0", "--seed", str(seed if seed is not None else 1)]
        if population is not None:
            command += ["-p", str(population)]

    # A run that ignores --time-limit is a bug, but do not let it wedge a sweep.
    result = subprocess.run(command, capture_output=True, text=True,
                            timeout=limit + 60)

    for line in result.stdout.splitlines():
        match = SUMMARY.match(line)
        if not match:
            continue
        detail = match.group("detail")
        work = NODES.search(detail) or GENERATIONS.search(detail)
        return (
            int(match.group("solutions")) > 0,
            float(match.group("seconds")),
            int(work.group(1)) if work else 0,
        )

    raise RuntimeError(f"could not parse output of {' '.join(command)}:\n{result.stdout}")


def classify(solved: bool, seconds: float, limit: float) -> str:
    if solved:
        return "solved"
    # The solver polls the clock between steps, so it overshoots slightly.
    return "timeout" if seconds >= limit * 0.98 else "stalled"


def trial_seed(n: int, population: int, trial: int) -> int:
    """Distinct per cell, and stable across reruns so a resume is seamless."""
    return 1_000_003 * n + 1_009 * population + trial + 1


def crossover_sizes(start: int, stop: int) -> list[int]:
    """Every size while they are cheap, then coarser as runs get long."""
    sizes = list(range(start, min(stop, 40) + 1))
    for size in (45, 50, 60, 70, 80, 100, 120, 140, 160, 180, 200, 225, 250, 300):
        if start <= size <= stop:
            sizes.append(size)
    return sizes


def run_crossover(args) -> None:
    recorder = Recorder(args.csv, args.resume)
    started = time.monotonic()
    sizes = crossover_sizes(args.start, args.stop)

    backtrack_misses = 0
    backtrack_done = "backtrack" not in args.algorithms
    genetic_done = "genetic" not in args.algorithms

    for n in sizes:
        if backtrack_done and genetic_done:
            break
        if time.monotonic() - started > args.budget:
            print(f"budget of {args.budget}s spent; stopping at n={n}", file=sys.stderr)
            break

        if not backtrack_done:
            # Deterministic, so one run is the answer; repeat only cheap ones to
            # take the median and shake off machine noise.
            times, solved, work, outcome = [], False, 0, "timeout"
            for trial in range(args.backtrack_trials):
                key = ("crossover", "backtrack", n, 0, trial)
                if recorder.already_have(key):
                    break
                ok, seconds, nodes = solve(args.binary, "backtrack", n, args.limit)
                outcome = classify(ok, seconds, args.limit)
                recorder.add(Run("crossover", "backtrack", n, 0, trial, 0,
                                 int(ok), outcome, seconds, nodes))
                times.append(seconds)
                solved, work = ok, nodes
                if seconds > 5.0:  # too slow to repeat
                    break
            if times:
                shown = statistics.median(times)
                print(f"backtrack n={n:<4} {outcome:<8} {shown:9.4f}s  {work} nodes",
                      flush=True)
            backtrack_misses = backtrack_misses + 1 if not solved else 0
            if backtrack_misses >= 2:
                print(f"backtrack passed {args.limit}s twice running; done", flush=True)
                backtrack_done = True

        if not genetic_done:
            successes, attempts, consecutive_failures = [], [], 0
            for trial in range(args.trials):
                key = ("crossover", "genetic", n, 0, trial)
                if recorder.already_have(key):
                    continue
                seed = trial_seed(n, 0, trial)
                ok, seconds, generations = solve(args.binary, "genetic", n,
                                                 args.limit, seed=seed)
                recorder.add(Run("crossover", "genetic", n, 0, trial, seed,
                                 int(ok), classify(ok, seconds, args.limit),
                                 seconds, generations))
                attempts.append(seconds)
                if ok:
                    successes.append(seconds)
                    consecutive_failures = 0
                else:
                    # A size where it never succeeds would otherwise burn
                    # trials * limit seconds proving it.
                    consecutive_failures += 1
                    if consecutive_failures >= args.give_up_after:
                        print(f"genetic   n={n:<4} gave up after "
                              f"{consecutive_failures} failures in a row", flush=True)
                        break
            if attempts:
                rate = len(successes) / len(attempts)
                mean = statistics.mean(successes) if successes else float("nan")
                print(f"genetic   n={n:<4} {len(successes)}/{len(attempts)} solved  "
                      f"mean {mean:9.4f}s", flush=True)
                if not successes:
                    print("genetic solved nothing at this size; done", flush=True)
                    genetic_done = True
                elif rate < 0.5 or mean >= args.limit:
                    print("genetic past its useful range; done", flush=True)
                    genetic_done = True

    recorder.close()
    print(f"wrote {args.csv}")


def run_population(args) -> None:
    recorder = Recorder(args.csv, args.resume)
    started = time.monotonic()

    for n in args.sizes:
        for population in args.populations:
            if time.monotonic() - started > args.budget:
                print("budget spent; stopping", file=sys.stderr)
                recorder.close()
                return

            successes, attempts, consecutive_failures = [], [], 0
            for trial in range(args.trials):
                key = ("population", "genetic", n, population, trial)
                if recorder.already_have(key):
                    continue
                seed = trial_seed(n, population, trial)
                ok, seconds, generations = solve(args.binary, "genetic", n,
                                                 args.limit, seed=seed,
                                                 population=population)
                recorder.add(Run("population", "genetic", n, population, trial,
                                 seed, int(ok), classify(ok, seconds, args.limit),
                                 seconds, generations))
                attempts.append(seconds)
                if ok:
                    successes.append(seconds)
                    consecutive_failures = 0
                else:
                    consecutive_failures += 1
                    if consecutive_failures >= args.give_up_after:
                        break
            if attempts:
                mean = statistics.mean(successes) if successes else float("nan")
                print(f"n={n:<4} pop={population:<6} {len(successes)}/{len(attempts)} "
                      f"solved  mean {mean:9.4f}s", flush=True)

    recorder.close()
    print(f"wrote {args.csv}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--binary", type=Path, default=DEFAULT_BINARY)
    parser.add_argument("--out-dir", type=Path, default=DEFAULT_OUT)
    sub = parser.add_subparsers(dest="command", required=True)

    cross = sub.add_parser("crossover", help="time to first solution against board size")
    cross.add_argument("--limit", type=float, default=180.0,
                       help="seconds allowed per run (default 180, i.e. 3 minutes)")
    cross.add_argument("--trials", type=int, default=7,
                       help="genetic runs per size (default 7)")
    cross.add_argument("--backtrack-trials", type=int, default=3,
                       help="backtracking runs per size, for cheap sizes (default 3)")
    cross.add_argument("--start", type=int, default=8)
    cross.add_argument("--stop", type=int, default=300)
    cross.add_argument("--budget", type=float, default=5400.0,
                       help="seconds for the whole sweep (default 5400)")
    cross.add_argument("--give-up-after", type=int, default=3,
                       help="abandon a size after this many failures in a row")
    cross.add_argument("--algorithms", default="backtrack,genetic")
    cross.add_argument("--csv", type=Path)
    cross.add_argument("--resume", action="store_true")
    cross.set_defaults(func=run_crossover)

    pop = sub.add_parser("population", help="time to first solution against population")
    pop.add_argument("--limit", type=float, default=60.0)
    pop.add_argument("--trials", type=int, default=7)
    pop.add_argument("--sizes", type=int, nargs="+", default=[40, 80, 120, 160])
    pop.add_argument("--populations", type=int, nargs="+",
                     default=[25, 50, 100, 200, 400, 800, 1600, 3200, 6400])
    pop.add_argument("--give-up-after", type=int, default=2,
                     help="abandon a cell after this many failures in a row")
    pop.add_argument("--budget", type=float, default=5400.0)
    pop.add_argument("--csv", type=Path)
    pop.add_argument("--resume", action="store_true")
    pop.set_defaults(func=run_population)

    chart = sub.add_parser("chart", help="render the charts from the CSVs")
    chart.add_argument("--crossover-csv", type=Path)
    chart.add_argument("--population-csv", type=Path)
    chart.set_defaults(func=None)

    args = parser.parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=True)

    if args.command == "chart":
        from chart import render_all  # local module, kept out of the runner
        args.crossover_csv = args.crossover_csv or args.out_dir / "crossover.csv"
        args.population_csv = args.population_csv or args.out_dir / "population.csv"
        return render_all(args)

    args.csv = args.csv or args.out_dir / f"{args.command}.csv"
    record_environment(args.out_dir, args.binary)
    if not args.binary.exists():
        parser.error(f"{args.binary} not found; build it first (cmake --build build)")
    if args.command == "crossover":
        args.algorithms = set(args.algorithms.split(","))
    args.func(args)
    return 0


if __name__ == "__main__":
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    sys.exit(main())
