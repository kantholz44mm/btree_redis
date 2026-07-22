#!/usr/bin/env python3
"""Recreate dense-partition.png with optional comparison data.

Example:
  python3 R/eval-dense/dense_partition_matplotlib.py \
    --ours path/to/our-measurement.csv.gz \
    --output R/eval-dense/out/dense-partition-comparison.png
"""

from __future__ import annotations

import argparse
import csv
import gzip
from collections import defaultdict
from pathlib import Path
from statistics import median


DEFAULT_ORIGINAL = [
    "R/eval-dense/paper-sigmod25-dense-tasks-op2.csv.gz",
    "R/eval-dense/paper-sigmod25-sorted.csv",
    "R/eval-dense/paper-sigmod25-partition-id-hint.csv",
]

DEFAULT_OURS = [
    "R/eval-dense/dense-tasks.csv.gz",
    "R/eval-dense/task-sorted-insert.csv.gz",
    "R/eval-dense/partition-id-hint.csv.gz",
]

CONFIG_LABELS = {
    "hints": "hints",
    "dense3": "FDLs",
}

CONFIG_COLORS = {
    "hints": "#1b9e77",
    "dense1": "#7570b3",
    "dense2": "#e7298a",
    "dense3": "#d95f02",
}

def open_text(path: str | Path):
    path = Path(path)
    if path.suffix == ".gz":
        return gzip.open(path, "rt", newline="")
    return path.open("r", newline="")


def clean_key(key: str) -> str:
    return key.strip()


def clean_value(value: str | None) -> str:
    return "" if value is None else value.strip()


def to_float(row: dict[str, str], name: str, default: float = 0.0) -> float:
    value = clean_value(row.get(name))
    if value == "":
        return default
    try:
        return float(value)
    except ValueError:
        return default


def read_broken_csv(path: str | Path) -> list[dict[str, str]]:
    """Read the benchmark CSV format and skip repeated header rows."""
    rows: list[dict[str, str]] = []
    with open_text(path) as fh:
        reader = csv.DictReader(fh, skipinitialspace=True)
        if not reader.fieldnames:
            return rows

        fieldnames = [clean_key(name) for name in reader.fieldnames]
        first_field = fieldnames[0]

        for raw in reader:
            row = {
                clean_key(key): clean_value(value)
                for key, value in raw.items()
                if key is not None
            }
            if row.get(first_field) == first_field:
                continue
            rows.append(row)

    return rows


def normalize_data_name(name: str) -> str:
    return {
        "data/urls": "urls-full",
        "data/urls-short": "urls",
        "data/wiki": "wiki",
        "rng8": "sparse64",
        "rng4": "sparse",
        "partitioned_id": "partitioned_id",
        "int": "ints",
    }.get(name, name)


def aggregate_dataset(
    label: str,
    files: list[str],
    space_records: float,
    run_limit: int,
) -> dict[tuple[str, str, float], dict[str, float | str]]:
    values: dict[tuple[str, str, float], dict[str, list[float]]] = defaultdict(
        lambda: {"txs": [], "space": []}
    )

    for file_name in files:
        for row in read_broken_csv(file_name):
            if normalize_data_name(row.get("data_name", "")) != "partitioned_id":
                continue
            if row.get("config_name") not in CONFIG_LABELS:
                continue
            if run_limit >= 0 and to_float(row, "run_id", 0) >= run_limit:
                continue

            time = to_float(row, "time")
            scale = to_float(row, "scale")
            data_size = to_float(row, "data_size")
            range_len = to_float(row, "ycsb_range_len")
            if time <= 0 or scale <= 0 or data_size <= 0 or range_len <= 0:
                continue

            leaf_count = (
                to_float(row, "nodeCount_Leaf")
                + to_float(row, "nodeCount_Hash")
                + to_float(row, "nodeCount_Dense")
                + to_float(row, "nodeCount_Dense2")
            )
            inner_count = (
                to_float(row, "nodeCount_Inner")
                + to_float(row, "nodeCount_Head4")
                + to_float(row, "nodeCount_Head8")
            )
            node_count = leaf_count + inner_count
            x_value = data_size / range_len
            key = (label, row["config_name"], x_value)
            values[key]["txs"].append(scale / time / 1e6)
            values[key]["space"].append(node_count * 4096 / space_records)

    return {
        key: {
            "dataset": key[0],
            "config": key[1],
            "x": key[2],
            "txs": median(metric_values["txs"]),
            "space": median(metric_values["space"]),
        }
        for key, metric_values in values.items()
        if metric_values["txs"] and metric_values["space"]
    }


def format_records(value: float, _pos=None) -> str:
    if value >= 1000:
        return f"{value / 1000:g}k"
    return f"{value:g}"


def format_bytes(value: float, _pos=None) -> str:
    return f"{value:g} B"


def plot(results: list[dict[str, float | str]], output: str, title: str | None) -> None:
    try:
        import matplotlib.pyplot as plt
    except ModuleNotFoundError as exc:
        raise SystemExit(
            "matplotlib is required to generate the plot. Install it with "
            "`python3 -m pip install matplotlib`."
        ) from exc

    datasets = list(dict.fromkeys(str(row["dataset"]) for row in results))
    fig, axes = plt.subplots(
        len(datasets),
        2,
        figsize=(6.2, 1.65 * len(datasets)),
        dpi=1500,
        squeeze=False,
        sharex=True,
    )
    columns = [("txs", "Minsert/s"), ("space", "Space/Record")]

    for row_index, dataset in enumerate(datasets):
        for col_index, (metric, ylabel) in enumerate(columns):
            ax = axes[row_index][col_index]
            y_values = []
            for config in CONFIG_LABELS:
                series = [
                    row
                    for row in results
                    if row["dataset"] == dataset and row["config"] == config
                ]
                if not series:
                    continue
                series.sort(key=lambda row: float(row["x"]))
                ax.plot(
                    [float(row["x"]) for row in series],
                    [float(row[metric]) for row in series],
                    color=CONFIG_COLORS[config],
                    linewidth=1.2,
                )
                y_values.extend(float(row[metric]) for row in series)
                last = series[-1]
                ax.annotate(
                    CONFIG_LABELS[config],
                    xy=(float(last["x"]), float(last[metric])),
                    xytext=(-8, 0),
                    textcoords="offset points",
                    color=CONFIG_COLORS[config],
                    fontsize=7,
                    ha="right",
                    va="center",
                )

            ax.set_xscale("log")
            ax.set_xlim(10, 1e5)
            if y_values:
                ax.set_ylim(0, max(y_values) * 1.08)
            ax.set_ylabel(None)
            ax.grid(True, which="major", color="#dddddd", linewidth=0.45)
            ax.tick_params(axis="both", labelsize=8)
            ax.spines["top"].set_visible(False)
            ax.spines["right"].set_visible(False)
            ax.xaxis.set_major_formatter(format_records)
            if metric == "space":
                ax.yaxis.set_major_formatter(format_bytes)

    if title:
        fig.suptitle(title, fontsize=10)
    fig.supxlabel("Records/Partition", y=0.15, fontsize=8)
    fig.text(0.04, 0.55, "Minsert/s", rotation=90, va="center", fontsize=8)
    fig.text(0.930, 0.55, "Space/Record", rotation=270, va="center", fontsize=8)
    fig.tight_layout(rect=(0.04, 0.075, 0.965, 1), w_pad=1.4, h_pad=0.45)
    Path(output).parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate a Matplotlib version of R/eval-dense/out/dense-partition.png."
    )
    parser.add_argument(
        "--original",
        nargs="+",
        default=DEFAULT_ORIGINAL,
        help="CSV/CSV.GZ files for the original paper dataset.",
    )
    parser.add_argument(
        "--ours",
        nargs="*",
        default=DEFAULT_OURS,
        help="CSV/CSV.GZ files for our own measurement dataset.",
    )
    parser.add_argument("--original-label", default="Original Paper")
    parser.add_argument("--ours-label", default="Unsere Messung")
    parser.add_argument(
        "--output",
        default="R/eval-dense/out/dense-partition-comparison.png",
    )
    parser.add_argument("--title", default=None)
    parser.add_argument(
        "--space-records",
        type=float,
        default=1e7,
        help="Divisor for Space/Record, matching the original R plot default.",
    )
    parser.add_argument(
        "--run-limit",
        type=int,
        default=5,
        help="Keep rows with run_id < this value. Use -1 to disable.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    datasets = [(args.original_label, args.original)]
    if args.ours:
        datasets.append((args.ours_label, args.ours))

    combined: list[dict[str, float | str]] = []
    for label, files in datasets:
        combined.extend(
            aggregate_dataset(label, files, args.space_records, args.run_limit).values()
        )

    if not combined:
        raise SystemExit("No partitioned_id rows found in the provided CSV files.")

    plot(combined, args.output, args.title)
    print(f"Saved plot: {args.output}")


if __name__ == "__main__":
    main()
