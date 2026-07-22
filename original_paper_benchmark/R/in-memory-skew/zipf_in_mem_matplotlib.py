#!/usr/bin/env python3
"""Recreate zipf-in-mem.png with paper-vs-local comparison data."""

from __future__ import annotations

import argparse
import csv
import gzip
from collections import defaultdict
from pathlib import Path
from statistics import median


DEFAULT_ORIGINAL = [
    "R/in-memory-skew/paper-sigmod25-skew3b.csv.gz",
    "R/in-memory-skew/paper-sigmod25-lits.csv.gz",
]

DEFAULT_OURS = [
    "R/in-memory-skew/skew3.csv.gz",
    "R/in-memory-skew/lits.csv.gz",
]

DATA_ORDER = ["ints", "sparse", "urls", "wiki"]
DATA_LABELS = {
    "int": "ints",
    "rng4": "sparse",
    "data/urls-short": "urls",
    "data/wiki": "wiki",
}
CONFIG_LABELS = {
    "art": "ART",
    "hot": "HOT",
    "tlx": "TLX",
    "wh": "WH",
    "lits": "LITS",
}
CONFIG_COLORS = {
    "art": "#1b9e77",
    "hot": "#d95f02",
    "tlx": "#7570b3",
    "wh": "#e7298a",
    "lits": "#66a61e",
}


def open_text(path: str | Path):
    path = Path(path)
    if path.suffix == ".gz":
        return gzip.open(path, "rt", newline="")
    return path.open("r", newline="")


def to_float(row: dict[str, str], name: str, default: float = 0.0) -> float:
    value = row.get(name, "").strip()
    if not value:
        return default
    try:
        return float(value)
    except ValueError:
        return default


def read_broken_csv(path: str | Path) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    with open_text(path) as fh:
        reader = csv.DictReader(fh, skipinitialspace=True)
        if not reader.fieldnames:
            return rows

        fieldnames = [name.strip() for name in reader.fieldnames]
        first_field = fieldnames[0]
        for raw in reader:
            row = {
                key.strip(): ("" if value is None else value.strip())
                for key, value in raw.items()
                if key is not None
            }
            if row.get(first_field) == first_field:
                continue
            rows.append(row)
    return rows


def aggregate(files: list[str], label: str) -> list[dict[str, float | str]]:
    grouped: dict[tuple[str, str, float], list[tuple[float, float]]] = defaultdict(list)

    for file_name in files:
        path = Path(file_name)
        if not path.exists():
            continue
        for row in read_broken_csv(path):
            if row.get("op") != "ycsb_c":
                continue
            data_name = DATA_LABELS.get(row.get("data_name", ""))
            config_name = row.get("config_name", "")
            if not data_name or config_name not in {*CONFIG_LABELS, "adapt2"}:
                continue
            time = to_float(row, "time")
            scale = to_float(row, "scale")
            zipf = to_float(row, "ycsb_zipf")
            if time <= 0 or scale <= 0:
                continue
            grouped[(data_name, config_name, zipf)].append(
                (to_float(row, "run_id"), scale / time)
            )

    txs: dict[tuple[str, str, float], float] = {}
    for key, values in grouped.items():
        first_runs = [value for _run_id, value in sorted(values, key=lambda x: x[0])[:5]]
        if first_runs:
            txs[key] = median(first_runs)

    rows: list[dict[str, float | str]] = []
    for (data_name, config_name, zipf), value in txs.items():
        if config_name == "adapt2":
            continue
        reference = txs.get((data_name, "adapt2", zipf))
        if not reference:
            continue
        rows.append(
            {
                "dataset": label,
                "data_name": data_name,
                "config_name": config_name,
                "zipf": zipf,
                "value": value / reference,
            }
        )
    return rows


def format_ratio(value: float, _pos=None) -> str:
    labels = {0.25: "1/4", 0.5: "1/2", 1.0: "1", 2.0: "2", 4.0: "4"}
    rounded = round(float(value), 2)
    return labels.get(rounded, f"{rounded:g}")


def plot(rows: list[dict[str, float | str]], output: str) -> None:
    try:
        import matplotlib.pyplot as plt
        from matplotlib.ticker import FixedLocator, FuncFormatter, NullFormatter
    except ModuleNotFoundError as exc:
        raise SystemExit(
            "matplotlib is required to generate the plot. Install it with "
            "`python3 -m pip install matplotlib`."
        ) from exc

    datasets = list(dict.fromkeys(str(row["dataset"]) for row in rows))
    fig, axes = plt.subplots(
        len(datasets),
        len(DATA_ORDER),
        figsize=(8.2, 1.85 * len(datasets)),
        dpi=750,
        squeeze=False,
        sharex=True,
        sharey=False,
    )

    for row_index, dataset in enumerate(datasets):
        for col_index, data_name in enumerate(DATA_ORDER):
            ax = axes[row_index][col_index]
            panel_rows = [
                row
                for row in rows
                if row["dataset"] == dataset and row["data_name"] == data_name
            ]

            ax.axhline(1.0, color="#b2182b", linestyle="--", linewidth=0.5)
            for config_name in CONFIG_LABELS:
                series = [
                    row for row in panel_rows if row["config_name"] == config_name
                ]
                if not series:
                    continue
                series.sort(key=lambda row: float(row["zipf"]))
                ax.plot(
                    [float(row["zipf"]) for row in series],
                    [float(row["value"]) for row in series],
                    color=CONFIG_COLORS[config_name],
                    linewidth=0.8,
                    alpha=0.95,
                )
                last = series[-1]
                ax.annotate(
                    CONFIG_LABELS[config_name],
                    xy=(float(last["zipf"]), float(last["value"])),
                    xytext=(-2, 0),
                    textcoords="offset points",
                    ha="right",
                    va="center",
                    color=CONFIG_COLORS[config_name],
                    fontsize=6.5,
                    bbox={
                        "facecolor": "white",
                        "edgecolor": "none",
                        "alpha": 0.7,
                        "pad": 0.7,
                    },
                )

            if row_index == 0:
                ax.set_title(data_name, fontsize=8)
            ax.set_yscale("log")
            ax.set_xlim(0.5, 1.5)
            if row_index == 0:
                ax.set_ylim(0.25, 4)
                y_ticks = [0.25, 0.5, 1.0, 2.0, 4.0]
            else:
                ax.set_ylim(0.65, 1.35)
                y_ticks = [0.7, 0.85, 1.0, 1.15, 1.3]
            ax.set_xticks([0.5, 1.0, 1.5])
            ax.yaxis.set_major_locator(FixedLocator(y_ticks))
            ax.yaxis.set_major_formatter(FuncFormatter(format_ratio))
            ax.yaxis.set_minor_formatter(NullFormatter())
            ax.grid(True, which="major", color="#d9d9d9", linewidth=0.55)
            ax.grid(True, which="minor", color="#eeeeee", linewidth=0.35)

    fig.supxlabel("Zipf-Parameter", y=0.15, fontsize=9)
    fig.text(
        0.015,
        0.65,
        "Normalisierte Lookups/s (log)",
        rotation=90,
        va="center",
        fontsize=9,
    )
    fig.tight_layout(rect=(0.035, 0.1, 1, 0.98), w_pad=0.6, h_pad=0.7)
    Path(output).parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate a Matplotlib comparison version of zipf-in-mem.png."
    )
    parser.add_argument("--original", nargs="+", default=DEFAULT_ORIGINAL)
    parser.add_argument("--ours", nargs="*", default=DEFAULT_OURS)
    parser.add_argument("--original-label", default="Originales Paper")
    parser.add_argument("--ours-label", default="Unsere Messung")
    parser.add_argument(
        "--output",
        default="R/in-memory-skew/out/zipf-in-mem-comparison.png",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    rows = aggregate(args.original, args.original_label)
    if args.ours:
        rows.extend(aggregate(args.ours, args.ours_label))
    if not rows:
        raise SystemExit("No usable ycsb_c rows found in the provided CSV files.")
    plot(rows, args.output)
    print(f"Saved plot: {args.output}")


if __name__ == "__main__":
    main()
