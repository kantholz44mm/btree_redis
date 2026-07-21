#!/usr/bin/env python3
"""Generate an in-memory-size comparison plot."""

from __future__ import annotations

import argparse
import csv
import gzip
from pathlib import Path
from statistics import median


DEFAULT_ORIGINAL = [
    "R/in-mem-size/paper-sigmod25-in-mem-size.csv.gz",
    "R/in-mem-size/paper-sigmod25-lits.csv.gz",
]

DEFAULT_OURS = [
    "R/in-mem-size/in-mem-size.csv",
    "R/in-mem-size/in-mem-size-lits.csv",
]

DATA_ORDER = ["ints", "sparse", "urls", "wiki"]
DATA_LABELS = {
    "int": "ints",
    "rng4": "sparse",
    "data/urls-short": "urls",
    "data/wiki": "wiki",
}
CONFIG_ORDER = ["baseline", "adapt2", "art", "hot", "tlx", "wh", "lits"]
CONFIG_LABELS = {
    "baseline": "Base",
    "adapt2": "Adapt",
    "art": "ART",
    "hot": "HOT",
    "tlx": "TLX",
    "wh": "WH",
    "lits": "LITS",
}
CONFIG_COLORS = {
    "baseline": "#2166ac",
    "adapt2": "#b2182b",
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


def read_mem_size(path: str | Path) -> list[dict[str, str | float]]:
    rows: list[dict[str, str | float]] = []
    if not Path(path).exists():
        return rows

    with open_text(path) as fh:
        lines = [line for line in fh if line.startswith("__mem_size")]

    for row in csv.reader(lines, skipinitialspace=True):
        if len(row) != 6:
            continue
        _tag, config_name, data_name, _before, _after, diff = [value.strip() for value in row]
        normalized_data = DATA_LABELS.get(data_name)
        if not normalized_data or config_name not in CONFIG_LABELS:
            continue
        try:
            diff_value = float(diff)
        except ValueError:
            continue
        rows.append(
            {
                "config_name": config_name,
                "data_name": normalized_data,
                "size_gb": diff_value / 1e9,
            }
        )
    return rows


def aggregate(files: list[str], label: str) -> list[dict[str, str | float]]:
    values: dict[tuple[str, str], list[float]] = {}
    for file_name in files:
        for row in read_mem_size(file_name):
            key = (str(row["data_name"]), str(row["config_name"]))
            values.setdefault(key, []).append(float(row["size_gb"]))

    return [
        {
            "dataset": label,
            "data_name": data_name,
            "config_name": config_name,
            "size_gb": median(size_values),
        }
        for (data_name, config_name), size_values in values.items()
    ]


def plot(rows: list[dict[str, str | float]], output: str) -> None:
    try:
        import matplotlib.pyplot as plt
    except ModuleNotFoundError as exc:
        raise SystemExit(
            "matplotlib is required to generate the plot. Install it with "
            "`python3 -m pip install matplotlib`."
        ) from exc

    datasets = list(dict.fromkeys(str(row["dataset"]) for row in rows))
    fig, axes = plt.subplots(
        len(datasets),
        len(DATA_ORDER),
        figsize=(8.2, 1.7 * len(datasets)),
        dpi=750,
        squeeze=False,
        sharey=False,
    )
    x_positions = list(range(len(CONFIG_ORDER)))

    for row_index, dataset in enumerate(datasets):
        for col_index, data_name in enumerate(DATA_ORDER):
            ax = axes[row_index][col_index]
            panel_rows = {
                str(row["config_name"]): float(row["size_gb"])
                for row in rows
                if row["dataset"] == dataset and row["data_name"] == data_name
            }
            bar_x = [
                x_positions[index]
                for index, config_name in enumerate(CONFIG_ORDER)
                if config_name in panel_rows
            ]
            bar_y = [
                panel_rows[config_name]
                for config_name in CONFIG_ORDER
                if config_name in panel_rows
            ]
            bar_colors = [
                CONFIG_COLORS[config_name]
                for config_name in CONFIG_ORDER
                if config_name in panel_rows
            ]
            ax.bar(bar_x, bar_y, color=bar_colors, width=0.7)

            if row_index == 0:
                ax.set_title(data_name, fontsize=8)
            ax.set_xticks(x_positions)
            ax.set_xticklabels([CONFIG_LABELS[name] for name in CONFIG_ORDER], rotation=90, fontsize=6)
            ax.tick_params(axis="y", labelsize=7)
            ax.grid(True, axis="y", color="#dddddd", linewidth=0.45)
            ax.set_axisbelow(True)
            ax.spines["top"].set_visible(False)
            ax.spines["right"].set_visible(False)
            if bar_y:
                ax.set_ylim(0, max(bar_y) * 1.12)

    fig.text(0.015, 0.55, "Größe (GB)", rotation=90, va="center", fontsize=8)
    fig.tight_layout(rect=(0.035, 0.05, 1, 0.98), w_pad=0.6, h_pad=0.6)
    Path(output).parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate a Matplotlib comparison version of in-mem-size.png."
    )
    parser.add_argument("--original", nargs="+", default=DEFAULT_ORIGINAL)
    parser.add_argument("--ours", nargs="*", default=DEFAULT_OURS)
    parser.add_argument("--original-label", default="Originales Paper")
    parser.add_argument("--ours-label", default="Unsere Messung")
    parser.add_argument(
        "--output",
        default="R/in-mem-size/out/in-mem-size-comparison.png",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    rows = aggregate(args.original, args.original_label) 
    if args.ours:
        rows.extend(aggregate(args.ours, args.ours_label))
    if not rows:
        raise SystemExit("No __mem_size rows found in the provided input files.")
    plot(rows, args.output)
    print(f"Saved plot: {args.output}")


if __name__ == "__main__":
    main()
