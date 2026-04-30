#!/usr/bin/env python3
import random
from pathlib import Path

TARGET_BYTES = 300 * 1024 * 1024
DENSE_PATH   = Path("data/integer_dense.txt")
SPARSE_PATH  = Path("data/integer_sparse.txt")


def write_dense(path: Path):
    print(f"Writing dense dataset -> {path}")
    written = 0
    count = 0
    with open(path, "w") as f:
        while written < TARGET_BYTES:
            line = f"{count}\n"
            f.write(line)
            written += len(line)
            count += 1
            if count % 1_000_000 == 0:
                print(f"  {written / 1024**2:.1f} / {TARGET_BYTES / 1024**2:.0f} MB", end="\r")
    print(f"  Done: {count:,} integers, {written / 1024**2:.1f} MB       ")


def write_sparse(path: Path):
    print(f"Writing sparse dataset -> {path}")
    # A random 32-bit int averages ~9.3 chars + newline ≈ 10.3 bytes.
    estimate = int(TARGET_BYTES / 10.3) + 100_000
    print(f"  Sampling ~{estimate:,} unique values ...")
    values = random.sample(range(2**32), estimate)
    values.sort()

    written = 0
    count = 0
    with open(path, "w") as f:
        for v in values:
            line = f"{v}\n"
            f.write(line)
            written += len(line)
            count += 1
            if count % 1_000_000 == 0:
                print(f"  {written / 1024**2:.1f} / {TARGET_BYTES / 1024**2:.0f} MB", end="\r")
            if written >= TARGET_BYTES:
                break

    print(f"  Done: {count:,} integers, {written / 1024**2:.1f} MB       ")


def main():
    DENSE_PATH.parent.mkdir(parents=True, exist_ok=True)
    write_dense(DENSE_PATH)
    write_sparse(SPARSE_PATH)
    print(f"\nDense:  {DENSE_PATH.stat().st_size / 1024**2:.1f} MB")
    print(f"Sparse: {SPARSE_PATH.stat().st_size / 1024**2:.1f} MB")


if __name__ == "__main__":
    main()
