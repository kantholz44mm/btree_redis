#!/usr/bin/env python3
import gzip
import os
import re
import tempfile
import urllib.request
from pathlib import Path

DUMPS_INDEX_URL = "https://dumps.wikimedia.org/enwiki/latest/"
FILE_PATTERN    = re.compile(r'enwiki-latest-all-titles-in-ns0\.gz')
OUTPUT_PATH     = Path("data/wikipedia_titles.txt")


def find_dump_url(index_url: str) -> str:
    print(f"Fetching dump index: {index_url}")
    with urllib.request.urlopen(index_url) as resp:
        html = resp.read().decode("utf-8")
    matches = FILE_PATTERN.findall(html)
    if not matches:
        raise RuntimeError("Could not find all-titles-in-ns0 dump file on the index page.")
    return index_url + matches[0]


def download_to_tempfile(url: str) -> str:
    print(f"Downloading: {url}")
    tmp = tempfile.NamedTemporaryFile(delete=False, suffix=".gz")
    try:
        with urllib.request.urlopen(url) as resp:
            total = resp.headers.get("Content-Length")
            if total:
                total = int(total)
                print(f"  File size: {total / 1_000_000:.1f} MB")
            downloaded = 0
            chunk_size = 1024 * 256
            while True:
                chunk = resp.read(chunk_size)
                if not chunk:
                    break
                tmp.write(chunk)
                downloaded += len(chunk)
                if total:
                    pct = downloaded / total * 100
                    print(f"  {downloaded / 1_000_000:.1f} / {total / 1_000_000:.1f} MB ({pct:.1f}%)", end="\r")
        print()
    except Exception:
        tmp.close()
        os.unlink(tmp.name)
        raise
    tmp.close()
    return tmp.name


def generate_txt(gz_path: str, output_path: Path) -> int:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    print(f"Decompressing and writing to: {output_path}")
    row_count = 0
    written = 0
    with gzip.open(gz_path, "rt", encoding="utf-8") as gz_file:
        with open(output_path, "w", encoding="utf-8") as out_file:
            for line in gz_file:
                title = line.rstrip("\n")
                if not title:
                    continue
                out_file.write(title + "\n")
                row_count += 1
                written += len(title.encode()) + 1
                if row_count % 500_000 == 0:
                    print(f"  {written / 1024**2:.1f} MB written", end="\r")
    print()
    print(f"Average title length: {(written - row_count) / row_count:.1f} bytes")
    return row_count


def main():
    dump_url = find_dump_url(DUMPS_INDEX_URL)
    tmp_path = download_to_tempfile(dump_url)
    try:
        row_count = generate_txt(tmp_path, OUTPUT_PATH)
    finally:
        os.unlink(tmp_path)
        print(f"Cleaned up temp file: {tmp_path}")
    print(f"\nDone! Wrote {row_count:,} titles to {OUTPUT_PATH}")


if __name__ == "__main__":
    main()
