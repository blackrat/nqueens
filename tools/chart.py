# Copyright (c) 1992-2026 Paul McKibbin
"""Render the benchmark CSVs as SVG, plus an HTML report with table views.

Charts are written by hand rather than through a plotting library so the repo
needs nothing beyond python3 to reproduce them.

Palette, mark specs and the accessibility rules follow the house data-viz
guidance: categorical hues in fixed slot order, hairline recessive chrome, a
legend plus direct labels so identity is never colour alone, dark-mode steps
chosen for the dark surface rather than flipped, and a table view of every
number in the HTML report.
"""

from __future__ import annotations

import csv
import html
import math
import statistics
from collections import defaultdict
from pathlib import Path

# Categorical slots 1-5, (light, dark). Validated as a set in both modes.
SERIES = [
    ("#2a78d6", "#3987e5"),
    ("#eb6834", "#d95926"),
    ("#1baf7a", "#199e70"),
    ("#eda100", "#c98500"),
    ("#e87ba4", "#d55181"),
]

INK = {
    "surface": ("#fcfcfb", "#1a1a19"),
    "primary": ("#0b0b0b", "#ffffff"),
    "secondary": ("#52514e", "#c3c2b7"),
    "muted": ("#898781", "#898781"),
    "grid": ("#e1e0d9", "#2c2c2a"),
    "axis": ("#c3c2b7", "#383835"),
}

FONT = 'system-ui, -apple-system, "Segoe UI", sans-serif'


def load(path: Path) -> list[dict]:
    if not path.exists():
        return []
    with path.open(newline="") as handle:
        rows = list(csv.DictReader(handle))
    for row in rows:
        row["n"] = int(row["n"])
        row["population"] = int(row["population"])
        row["solved"] = int(row["solved"])
        row["seconds"] = float(row["seconds"])
        row["work"] = int(row["work"])
    return rows


# --------------------------------------------------------------------------
# Aggregation
# --------------------------------------------------------------------------

def group(rows, key) -> dict:
    out = defaultdict(list)
    for row in rows:
        out[key(row)].append(row)
    return dict(out)


def summarise(runs: list[dict]) -> dict:
    """Mean time over the successful runs, and the cost of getting one
    *including* the failures you have to sit through first."""
    wins = [r["seconds"] for r in runs if r["solved"]]
    spent = sum(r["seconds"] for r in runs)
    return {
        "attempts": len(runs),
        "wins": len(wins),
        "rate": len(wins) / len(runs) if runs else 0.0,
        "mean": statistics.mean(wins) if wins else None,
        "median": statistics.median(wins) if wins else None,
        # Restart-on-failure expectation: everything spent, per solution won.
        "expected": spent / len(wins) if wins else None,
    }


def crossover_data(rows: list[dict]) -> dict:
    backtrack, genetic = {}, {}
    for n, runs in sorted(group([r for r in rows if r["algorithm"] == "backtrack"],
                                lambda r: r["n"]).items()):
        wins = [r["seconds"] for r in runs if r["solved"]]
        if wins:
            backtrack[n] = statistics.median(wins)
    for n, runs in sorted(group([r for r in rows if r["algorithm"] == "genetic"],
                                lambda r: r["n"]).items()):
        genetic[n] = summarise(runs)
    return {"backtrack": backtrack, "genetic": genetic}


def find_crossover(data: dict) -> int | None:
    """Smallest n past which the genetic mean beats backtracking at every size
    measured. A single crossing point would be a lie: backtracking's cost
    saw-tooths, so it wins several sizes back after first losing."""
    shared = sorted(set(data["backtrack"]) & {n for n, s in data["genetic"].items()
                                              if s["mean"] is not None})
    if not shared:
        return None
    for index, n in enumerate(shared):
        if all(data["genetic"][m]["mean"] < data["backtrack"][m] for m in shared[index:]):
            return n
    return None


def population_data(rows: list[dict]) -> dict:
    out = defaultdict(dict)
    for (n, population), runs in group(rows, lambda r: (r["n"], r["population"])).items():
        out[n][population] = summarise(runs)
    return {n: dict(sorted(v.items())) for n, v in sorted(out.items())}


RELIABLE = 2 / 3


def best_population(series: dict, reliable_only: bool = False) -> dict:
    """Cheapest population per board size, by expected cost including retries.

    With `reliable_only`, restrict to populations that succeed at least two
    times in three. A cell that won once in nine runs has an expected cost
    estimated from a single sample, and the raw minimum is happy to pick it.
    """
    best = {}
    for n, by_population in series.items():
        usable = {p: s for p, s in by_population.items()
                  if s["expected"] is not None
                  and (not reliable_only or s["rate"] >= RELIABLE)}
        if usable:
            population = min(usable, key=lambda p: usable[p]["expected"])
            best[n] = (population, usable[population])
    return best


# --------------------------------------------------------------------------
# SVG
# --------------------------------------------------------------------------

class LogScale:
    def __init__(self, low, high, pixel_low, pixel_high):
        self.a, self.b = math.log10(low), math.log10(high)
        self.pa, self.pb = pixel_low, pixel_high

    def __call__(self, value):
        t = (math.log10(value) - self.a) / (self.b - self.a) if self.b > self.a else 0.5
        return self.pa + t * (self.pb - self.pa)


class LinearScale:
    def __init__(self, low, high, pixel_low, pixel_high):
        self.a, self.b, self.pa, self.pb = low, high, pixel_low, pixel_high

    def __call__(self, value):
        t = (value - self.a) / (self.b - self.a) if self.b > self.a else 0.5
        return self.pa + t * (self.pb - self.pa)


def seconds_label(value: float) -> str:
    for scale, unit in ((1.0, "s"), (1e-3, "ms"), (1e-6, "µs")):
        if value >= scale * 0.999:
            shown = value / scale
            return f"{shown:.0f}{unit}" if shown >= 1 else f"{shown:.3g}{unit}"
    return f"{value * 1e9:.0f}ns"


def decade_ticks(low: float, high: float) -> list[float]:
    start, stop = math.floor(math.log10(low)), math.ceil(math.log10(high))
    return [10.0 ** power for power in range(start, stop + 1)]


def esc(text) -> str:
    return html.escape(str(text), quote=True)


def render_chart(*, title, subtitle, series, x_label, y_label, x_ticks, x_scale_kind,
                 width=980, height=520, annotation=None, chart_id="chart",
                 dark_mode=False) -> str:
    """series: list of dicts {name, points: [(x, y, tooltip)], slot}.

    Colours go in as literal presentation attributes in their light-mode values.
    An earlier draft drove them from CSS custom properties, which made the
    standalone files render as a black rectangle in librsvg and anything else
    that does not resolve `var()`.

    `dark_mode` adds a stylesheet carrying the dark steps. It is on for the
    charts inlined into the HTML report, where a browser resolves the media
    query properly, and off for the standalone .svg files: SVG rasterisers vary
    in how much of that query they honour, and librsvg applies the overrides
    unconditionally, which is worse than having no dark mode at all.
    """
    left, right, top, bottom = 74, 232, 104, 62
    plot_left, plot_right = left, width - right
    plot_top, plot_bottom = top, height - bottom

    xs = [x for s in series for x, _, _ in s["points"]]
    ys = [y for s in series for _, y, _ in s["points"]]
    if not xs or not ys:
        return f"<!-- no data for {esc(title)} -->"

    y_scale = LogScale(min(ys) * 0.7, max(ys) * 1.4, plot_bottom, plot_top)
    if x_scale_kind == "log":
        x_scale = LogScale(min(xs) * 0.85, max(xs) * 1.18, plot_left, plot_right)
    else:
        span = (max(xs) - min(xs)) or 1
        x_scale = LinearScale(min(xs) - span * 0.02, max(xs) + span * 0.02,
                              plot_left, plot_right)

    light = {k: v[0] for k, v in INK.items()}
    out = []
    add = out.append

    overrides = [
        f"#{chart_id} .surface{{fill:{INK['surface'][1]};}}",
        f"#{chart_id} .grid{{stroke:{INK['grid'][1]};}}",
        f"#{chart_id} .axis{{stroke:{INK['axis'][1]};}}",
        f"#{chart_id} .ring{{stroke:{INK['surface'][1]};}}",
        f"#{chart_id} .ink1{{fill:{INK['primary'][1]};}}",
        f"#{chart_id} .ink2{{fill:{INK['secondary'][1]};}}",
        f"#{chart_id} .ink3{{fill:{INK['muted'][1]};}}",
    ]
    for index, (_, dark_hex) in enumerate(SERIES, start=1):
        overrides.append(f"#{chart_id} .fill{index}{{fill:{dark_hex};}}"
                         f"#{chart_id} .line{index}{{stroke:{dark_hex};}}")
    dark_css = "".join(overrides)

    add(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" '
        f'width="{width}" height="{height}" role="img" '
        f'aria-label="{esc(title)}. {esc(subtitle)}" class="viz-root" id="{chart_id}">')
    add("<style>")
    add(f'#{chart_id} text{{font-family:{FONT};}}'
        f'#{chart_id} .tick{{font-size:12px;font-variant-numeric:tabular-nums;}}'
        f'#{chart_id} .title{{font-size:17px;font-weight:600;}}'
        f'#{chart_id} .sub,#{chart_id} .legend,#{chart_id} .axis-label'
        f'{{font-size:12.5px;}}'
        f'#{chart_id} .end-label{{font-size:12px;font-weight:600;}}'
        f'#{chart_id} .note{{font-size:11px;}}')
    if dark_mode:
        add('@media (prefers-color-scheme: dark){'
            ':root:where(:not([data-theme="light"])) ' + dark_css + '}')
        add(':root[data-theme="dark"] ' + dark_css)
    add("</style>")

    add(f'<rect class="surface" width="{width}" height="{height}" '
        f'fill="{light["surface"]}"/>')
    add(f'<text class="title ink1" x="{left}" y="30" fill="{light["primary"]}">'
        f'{esc(title)}</text>')
    add(f'<text class="sub ink2" x="{left}" y="52" fill="{light["secondary"]}">'
        f'{esc(subtitle)}</text>')

    # A legend is always present for two or more series; the end-of-line labels
    # are the second, redundant channel so identity is never colour alone.
    cursor = left
    for entry in series:
        slot, name = entry["slot"], entry["name"]
        add(f'<rect class="fill{slot}" x="{cursor:.0f}" y="70" width="11" height="11" '
            f'rx="2.5" fill="{SERIES[slot - 1][0]}"/>')
        add(f'<text class="legend ink2" x="{cursor + 17:.0f}" y="80" '
            f'fill="{light["secondary"]}">{esc(name)}</text>')
        cursor += 32 + 7.2 * len(name)

    for value in decade_ticks(min(ys), max(ys)):
        y = y_scale(value)
        if not plot_top - 2 <= y <= plot_bottom + 2:
            continue
        add(f'<line class="grid" x1="{plot_left}" y1="{y:.1f}" x2="{plot_right}" '
            f'y2="{y:.1f}" stroke="{light["grid"]}" stroke-width="1"/>')
        add(f'<text class="tick ink3" x="{plot_left - 10}" y="{y + 4:.1f}" '
            f'text-anchor="end" fill="{light["muted"]}">{seconds_label(value)}</text>')

    for value in x_ticks:
        add(f'<text class="tick ink3" x="{x_scale(value):.1f}" y="{plot_bottom + 22}" '
            f'text-anchor="middle" fill="{light["muted"]}">{value}</text>')

    add(f'<line class="axis" x1="{plot_left}" y1="{plot_bottom}" x2="{plot_right}" '
        f'y2="{plot_bottom}" stroke="{light["axis"]}" stroke-width="1"/>')
    add(f'<text class="axis-label ink2" x="{(plot_left + plot_right) / 2:.0f}" '
        f'y="{height - 16}" text-anchor="middle" fill="{light["secondary"]}">'
        f'{esc(x_label)}</text>')
    add(f'<text class="axis-label ink2" transform="translate(20,'
        f'{(plot_top + plot_bottom) / 2:.0f}) rotate(-90)" text-anchor="middle" '
        f'fill="{light["secondary"]}">{esc(y_label)}</text>')

    if annotation:
        value, text = annotation
        x = x_scale(value)
        add(f'<line class="axis" x1="{x:.1f}" y1="{plot_top}" x2="{x:.1f}" '
            f'y2="{plot_bottom}" stroke="{light["axis"]}" stroke-width="1"/>')
        flip = x > plot_right - 170
        add(f'<text class="note ink3" x="{x + (-6 if flip else 6):.1f}" '
            f'y="{plot_top - 9:.1f}" text-anchor="{"end" if flip else "start"}" '
            f'fill="{light["muted"]}">{esc(text)}</text>')

    for entry in series:
        slot = entry["slot"]
        colour = SERIES[slot - 1][0]
        points = entry["points"]
        path = " ".join(f'{"M" if i == 0 else "L"}{x_scale(x):.1f},{y_scale(y):.1f}'
                        for i, (x, y, _) in enumerate(points))
        add(f'<path class="line{slot}" d="{path}" fill="none" stroke="{colour}" '
            f'stroke-width="2" stroke-linejoin="round" stroke-linecap="round"/>')
        for x, y, tip in points:
            # 2px surface ring, so overlapping markers stay separable.
            add(f'<circle class="fill{slot} ring" cx="{x_scale(x):.1f}" '
                f'cy="{y_scale(y):.1f}" r="4" fill="{colour}" '
                f'stroke="{light["surface"]}" stroke-width="2">'
                f'<title>{esc(tip)}</title></circle>')
        last_x, last_y, _ = points[-1]
        add(f'<text class="end-label fill{slot}" x="{x_scale(last_x) + 12:.1f}" '
            f'y="{y_scale(last_y) + 4:.1f}" fill="{colour}">{esc(entry["name"])}</text>')

    add("</svg>")
    return "\n".join(out)


# --------------------------------------------------------------------------
# Report
# --------------------------------------------------------------------------

def crossover_chart(data: dict, dark_mode: bool = False) -> tuple[str, int | None]:
    backtrack = [(n, t, f"backtracking, n={n}: {seconds_label(t)}")
                 for n, t in sorted(data["backtrack"].items())]
    mean = [(n, s["mean"],
             f"genetic, n={n}: mean {seconds_label(s['mean'])} over "
             f"{s['wins']} of {s['attempts']} runs that succeeded")
            for n, s in sorted(data["genetic"].items()) if s["mean"] is not None]
    expected = [(n, s["expected"],
                 f"genetic, n={n}: {seconds_label(s['expected'])} per solution "
                 f"including retries ({s['rate'] * 100:.0f}% of runs succeed)")
                for n, s in sorted(data["genetic"].items()) if s["expected"] is not None]

    crossing = find_crossover(data)
    every = sorted({n for n, _, _ in backtrack} | {n for n, _, _ in mean})
    # Log x: the story is n=8 to 40, and a linear axis stretched to the genetic
    # search's own ceiling would squash it into the left quarter.
    ticks = [n for n in (8, 10, 12, 15, 20, 25, 30, 40, 60, 80, 120, 160, 200, 250)
             if every[0] <= n <= every[-1]]

    svg = render_chart(
        title="Time to a first solution: backtracking vs genetic search",
        subtitle="Both axes log; each horizontal gridline is 10x the one below. "
                 "Backtracking is absent at sizes where it blew the 3-minute cap.",
        series=[
            {"name": "backtracking", "slot": 1, "points": backtrack},
            {"name": "genetic mean", "slot": 2, "points": mean},
            {"name": "genetic + retries", "slot": 3, "points": expected},
        ],
        x_label="board size n", y_label="time to first solution",
        x_ticks=ticks, x_scale_kind="log",
        annotation=(crossing, f"genetic ahead from n={crossing}") if crossing else None,
        chart_id="crossover", dark_mode=dark_mode)
    return svg, crossing


def population_chart(series: dict, dark_mode: bool = False) -> str:
    entries = []
    for slot, (n, by_population) in enumerate(sorted(series.items()), start=1):
        points = [(p, s["expected"],
                   f"n={n}, population {p}: {seconds_label(s['expected'])} per "
                   f"solution ({s['wins']} of {s['attempts']} runs succeeded)")
                  for p, s in by_population.items() if s["expected"] is not None]
        if points:
            entries.append({"name": f"n={n}", "slot": min(slot, len(SERIES)),
                            "points": points})
    if not entries:
        return ""

    populations = sorted({p for e in entries for p, _, _ in e["points"]})
    return render_chart(
        title="Genetic search: population size against time to a first solution",
        subtitle="Both axes log. Failed runs are counted, so this is what a "
                 "solution really costs.",
        series=entries,
        x_label="population", y_label="time per solution found",
        x_ticks=populations, x_scale_kind="log", chart_id="population",
        dark_mode=dark_mode)


def table(headers: list[str], rows: list[list], caption: str) -> str:
    head = "".join(f"<th>{esc(h)}</th>" for h in headers)
    body = "".join("<tr>" + "".join(f"<td>{esc(c)}</td>" for c in r) + "</tr>"
                   for r in rows)
    return (f"<details><summary>{esc(caption)}</summary>"
            f"<table><thead><tr>{head}</tr></thead><tbody>{body}</tbody></table>"
            f"</details>")


REPORT_CSS = """
:root { color-scheme: light dark; }
body { font-family: system-ui, -apple-system, "Segoe UI", sans-serif;
       background: #f9f9f7; color: #0b0b0b; margin: 0; padding: 40px 24px 80px; }
main { max-width: 1010px; margin: 0 auto; }
h1 { font-size: 24px; margin: 0 0 6px; }
p.lede { color: #52514e; margin: 0 0 32px; max-width: 68ch; line-height: 1.55; }
figure { margin: 0 0 10px; background: #fcfcfb; border: 1px solid rgba(11,11,11,.10);
         border-radius: 10px; padding: 8px; }
figure svg { display: block; width: 100%; height: auto; }
details { margin: 0 0 40px; }
summary { cursor: pointer; color: #52514e; font-size: 13px; padding: 6px 2px; }
table { border-collapse: collapse; font-size: 13px; margin-top: 8px;
        font-variant-numeric: tabular-nums; }
th, td { text-align: right; padding: 5px 12px; border-bottom: 1px solid #e1e0d9; }
th:first-child, td:first-child { text-align: left; }
.finding { background: #fcfcfb; border: 1px solid rgba(11,11,11,.10);
           border-radius: 10px; padding: 16px 20px; margin: 0 0 36px; line-height: 1.55; }
.finding p { margin: 12px 0 0; }
@media (prefers-color-scheme: dark) {
  body { background: #0d0d0d; color: #fff; }
  p.lede, summary { color: #c3c2b7; }
  figure, .finding { background: #1a1a19; border-color: rgba(255,255,255,.10); }
  th, td { border-bottom-color: #2c2c2a; }
}
"""


def render_all(args) -> int:
    out_dir = args.out_dir
    cross_rows = load(args.crossover_csv)
    pop_rows = load(args.population_csv)
    sections, crossing, ratio_note = [], None, ""

    if cross_rows:
        data = crossover_data(cross_rows)
        svg, crossing = crossover_chart(data)
        (out_dir / "crossover.svg").write_text(svg)
        svg = crossover_chart(data, dark_mode=True)[0]
        rows = []
        for n in sorted(set(data["backtrack"]) | set(data["genetic"])):
            summary = data["genetic"].get(n)
            rows.append([
                n,
                seconds_label(data["backtrack"][n]) if n in data["backtrack"]
                else "over the limit",
                seconds_label(summary["mean"]) if summary and summary["mean"]
                else "never solved",
                seconds_label(summary["expected"]) if summary and summary["expected"]
                else "-",
                f"{summary['wins']}/{summary['attempts']}" if summary else "-",
            ])
        sections.append(
            f"<figure>{svg}</figure>"
            + table(["n", "backtracking", "genetic mean", "genetic with retries",
                     "genetic runs solved"], rows, "Table view: crossover data"))

    if pop_rows:
        series = population_data(pop_rows)
        svg = population_chart(series)
        if svg:
            (out_dir / "population.svg").write_text(svg)
            svg = population_chart(series, dark_mode=True)
        rows = []
        for n, by_population in series.items():
            for population, summary in by_population.items():
                rows.append([
                    n, population, f"{population / n:.1f}",
                    seconds_label(summary["mean"]) if summary["mean"] else "never solved",
                    seconds_label(summary["expected"]) if summary["expected"] else "-",
                    f"{summary['wins']}/{summary['attempts']}",
                ])

        cheapest = best_population(series)
        reliable = best_population(series, reliable_only=True)
        if cheapest:
            ratios = [p / n for n, (p, _) in cheapest.items()]
            safe_ratios = [p / n for n, (p, _) in reliable.items()]
            median_ratio = statistics.median(ratios)
            median_safe = statistics.median(safe_ratios) if safe_ratios else median_ratio
            ratio_note = (f" The cheapest population runs about "
                          f"{median_ratio:.0f}x the board size, or "
                          f"{median_safe:.0f}x if you want it to succeed most of "
                          f"the time.")
            finding = ["<div class='finding'><strong>Population that pays best, "
                       "per board size</strong><table><thead><tr><th>n</th>"
                       "<th>cheapest</th><th>/ n</th><th>per solution</th>"
                       "<th>cheapest that wins 2 runs in 3</th><th>/ n</th>"
                       "<th>per solution</th></tr></thead><tbody>"]
            for n, (population, summary) in sorted(cheapest.items()):
                safe = reliable.get(n)
                cells = (f"<td>{safe[0]}</td><td>{safe[0] / n:.1f}</td>"
                         f"<td>{seconds_label(safe[1]['expected'])}</td>"
                         if safe else "<td>-</td><td>-</td><td>-</td>")
                finding.append(f"<tr><td>{n}</td><td>{population}</td>"
                               f"<td>{population / n:.1f}</td>"
                               f"<td>{seconds_label(summary['expected'])}</td>"
                               f"{cells}</tr>")
            finding.append(f"</tbody></table><p>Median ratio "
                           f"<strong>{median_ratio:.1f} x n</strong> "
                           f"(range {min(ratios):.1f} to {max(ratios):.1f}); "
                           f"restricted to populations that win two runs in three, "
                           f"<strong>{median_safe:.1f} x n</strong>.</p></div>")
            sections.append("".join(finding))

        sections.append(
            (f"<figure>{svg}</figure>" if svg else "")
            + table(["n", "population", "population / n", "mean of successes",
                     "per solution with retries", "runs solved"], rows,
                    "Table view: population data"))

    lede = ("Time to a <em>first</em> solution, measured by the solver's own clock. "
            "Backtracking is deterministic, so each size is the median of repeated "
            "runs. The genetic search is not, and dead-ends often, so each size is "
            "several runs: both the mean of the wins and the honest cost per "
            "solution including the failures are plotted.")
    if crossing:
        lede += f" Genetic search pulls ahead for good at n={crossing}."
    lede += ratio_note

    report = ("<!doctype html><html lang='en'><head><meta charset='utf-8'>"
              "<meta name='viewport' content='width=device-width,initial-scale=1'>"
              f"<title>queens benchmarks</title><style>{REPORT_CSS}</style></head>"
              "<body><main><h1>queens: backtracking vs genetic search</h1>"
              f"<p class='lede'>{lede}</p>" + "".join(sections) + "</main></body></html>")
    (out_dir / "report.html").write_text(report)

    print(f"wrote {out_dir / 'report.html'}")
    for name in ("crossover.svg", "population.svg"):
        if (out_dir / name).exists():
            print(f"wrote {out_dir / name}")
    if crossing:
        print(f"genetic search overtakes backtracking from n={crossing}")
    return 0
