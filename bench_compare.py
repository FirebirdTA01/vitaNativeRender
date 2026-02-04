#!/usr/bin/env python3
"""
bench_compare.py

Compare nativeRenderBench CSV files containing multiple runs (# Run N).
Generates OVERLAID LINE PLOTS so multiple builds can be compared visually.

Example:
  python bench_compare.py nativeRenderBench_3k.csv nativeRenderBench_original.csv --metric FPS
"""

from __future__ import annotations

import argparse
import csv
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

RUN_RE = re.compile(r"^\s*#\s*Run\s*(\d+)\s*$")


@dataclass
class ParsedRun:
    run_number: int
    df: pd.DataFrame


def sanitize_filename(s: str) -> str:
    s = re.sub(r"[^\w.\- ]+", "_", s)
    s = s.strip().replace(" ", "_")
    return s or "plot"


def parse_runs(path: str | Path) -> List[ParsedRun]:
    path = Path(path)
    lines = path.read_text(errors="replace").splitlines()

    runs: List[ParsedRun] = []
    i = 0

    while i < len(lines):
        m = RUN_RE.match(lines[i])
        if not m:
            i += 1
            continue

        run_number = int(m.group(1))
        i += 1

        # Find CSV header
        while i < len(lines) and (lines[i].strip() == "" or lines[i].lstrip().startswith("#")):
            i += 1
        if i >= len(lines):
            break

        header = next(csv.reader([lines[i]]))
        i += 1

        rows = []
        while i < len(lines):
            if RUN_RE.match(lines[i]):
                break
            if lines[i].strip() == "" or lines[i].lstrip().startswith("#"):
                i += 1
                continue
            row = next(csv.reader([lines[i]]))
            if len(row) == len(header):
                rows.append(row)
            i += 1

        df = pd.DataFrame(rows, columns=header)

        # Pandas 3.x safe numeric conversion
        for c in df.columns:
            if c == "Section":
                df[c] = df[c].astype(str)
                continue

            converted = pd.to_numeric(df[c], errors="coerce")
            if converted.notna().any():
                df[c] = converted
            else:
                df[c] = df[c].astype(str)

        runs.append(ParsedRun(run_number, df))

    if not runs:
        df = pd.read_csv(path, comment="#")
        runs = [ParsedRun(1, df)]

    return runs


def section_order_from_firstfile(runs: List[ParsedRun]) -> List[str]:
    seen = set()
    order = []
    for r in runs:
        for s in r.df["Section"]:
            if s not in seen:
                seen.add(s)
                order.append(s)
    return order


def per_file_section_average(
    runs: List[ParsedRun], metric: str
) -> pd.Series:
    per_run_means = []
    for r in runs:
        per_run_means.append(r.df.groupby("Section")[metric].mean())

    aligned = pd.concat(per_run_means, axis=1)
    return aligned.mean(axis=1, skipna=True)


def main() -> None:
    ap = argparse.ArgumentParser(description="Compare nativeRenderBench CSVs with overlaid line plots.")
    ap.add_argument("csv_files", nargs="+", help="Input CSV files")
    ap.add_argument("--metric", default="FPS", help="Metric column to plot")
    ap.add_argument("--outdir", default="bench_plots", help="Output directory")
    ap.add_argument("--labels", nargs="*", help="Optional labels (same count as CSVs)")
    ap.add_argument("--no-markers", action="store_true", help="Disable point markers")
    args = ap.parse_args()

    if args.labels and len(args.labels) != len(args.csv_files):
        raise SystemExit("--labels must match number of input files")

    labels = args.labels or [Path(p).stem for p in args.csv_files]
    outdir = Path(args.outdir)
    outdir.mkdir(parents=True, exist_ok=True)

    parsed: Dict[str, List[ParsedRun]] = {}
    for label, path in zip(labels, args.csv_files):
        parsed[label] = parse_runs(path)

    # Section order based on first file (camera flight order)
    sections = section_order_from_firstfile(parsed[labels[0]])
    x = np.arange(len(sections))

    plt.figure(figsize=(10, 5))

    for label in labels:
        series = per_file_section_average(parsed[label], args.metric)
        y = [series.get(s, np.nan) for s in sections]
        plt.plot(
            x,
            y,
            marker=None if args.no_markers else "o",
            label=label,
        )

    plt.xticks(x, sections, rotation=35, ha="right")
    plt.ylabel(args.metric)
    plt.title(f"{args.metric} by Section (overlaid comparison)")
    plt.legend()
    plt.tight_layout()

    outpath = outdir / f"{sanitize_filename(args.metric)}_by_section_lines.png"
    plt.savefig(outpath, dpi=160)
    plt.close()

    print(f"✔ Plot written to: {outpath}")


if __name__ == "__main__":
    main()
