#!/usr/bin/env python3
import gzip
import io
import json
import os
import sys
import time
import urllib.error
import urllib.request
import zlib
from pathlib import Path

COLLINFO_URL = "https://index.commoncrawl.org/collinfo.json"
CC_BASE_URL  = "https://data.commoncrawl.org/"
OUTPUT_PATH  = Path("data/commoncrawl_urls.txt")
TARGET_BYTES = 300 * 1024 * 1024
CHUNK_SIZE   = 256 * 1024

USER_AGENT = "Mozilla/5.0 (compatible; research-btree-benchmark/1.0)"


# CDX files use ZipNum / block-gzip: many independent gzip members concatenated.
# Python's gzip module only reads the first member from a raw socket, so we
# handle member boundaries manually with zlib.
class MultiGzipStream:
    WBITS = zlib.MAX_WBITS | 16  # +16 → accept gzip wrapper

    def __init__(self, resp):
        self._resp      = resp
        self._cbuf      = b""
        self._dec       = zlib.decompressobj(self.WBITS)
        self._tbuf      = b""
        self._net_eof   = False
        self.downloaded = 0

    def _pull(self):
        if self._net_eof:
            return
        chunk = self._resp.read(CHUNK_SIZE)
        if chunk:
            self._cbuf      += chunk
            self.downloaded += len(chunk)
        else:
            self._net_eof = True

    def _decompress_more(self):
        if not self._cbuf:
            return
        try:
            out = self._dec.decompress(self._cbuf)
            self._cbuf = b""
            if out:
                self._tbuf += out
            if self._dec.unused_data:
                self._cbuf = self._dec.unused_data
                self._dec  = zlib.decompressobj(self.WBITS)
        except zlib.error:
            self._cbuf = b""
            self._dec  = zlib.decompressobj(self.WBITS)

    def __iter__(self):
        return self

    def __next__(self) -> str:
        while True:
            nl = self._tbuf.find(b"\n")
            if nl != -1:
                line       = self._tbuf[:nl]
                self._tbuf = self._tbuf[nl + 1:]
                return line.decode("utf-8", errors="replace")

            if self._cbuf:
                self._decompress_more()
                continue

            if self._net_eof:
                try:
                    tail = self._dec.flush()
                    if tail:
                        self._tbuf += tail
                        continue
                except zlib.error:
                    pass
                if self._tbuf:
                    line       = self._tbuf
                    self._tbuf = b""
                    return line.decode("utf-8", errors="replace")
                raise StopIteration

            self._pull()


def http_get(url: str, timeout: int = 60):
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    return urllib.request.urlopen(req, timeout=timeout)


def get_latest_crawl_id() -> str:
    print("Fetching crawl list ...")
    with http_get(COLLINFO_URL) as resp:
        collections = json.loads(resp.read())
    crawl_id = collections[0]["id"]
    print(f"  Latest crawl: {crawl_id}")
    return crawl_id


def get_cdx_file_list(crawl_id: str) -> list[str]:
    paths_url = f"{CC_BASE_URL}crawl-data/{crawl_id}/cc-index.paths.gz"
    print("Fetching CDX file list ...")
    with http_get(paths_url) as resp:
        raw = resp.read()
    with gzip.open(io.BytesIO(raw), "rt", encoding="utf-8") as f:
        paths = [l.strip() for l in f if l.strip().endswith(".gz") and "cdx-" in l]
    print(f"  {len(paths)} CDX shards available\n")
    return paths


def stream_urls_from_cdx(
    cdx_path: str,
    out_file,
    bytes_written: int,
    url_count: int,
    total_url_bytes: int,
) -> tuple[int, int, int]:
    url = CC_BASE_URL + cdx_path
    try:
        req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
        with urllib.request.urlopen(req, timeout=120) as resp:
            stream = MultiGzipStream(resp)
            last_t = 0.0
            for line in stream:
                now = time.monotonic()
                if now - last_t >= 0.25:
                    print(f"  {bytes_written / 1024**2:.1f} / {TARGET_BYTES / 1024**2:.0f} MB", end="\r")
                    last_t = now
                line = line.strip()
                if not line:
                    continue
                try:
                    parts = line.split(" ", 2)
                    if len(parts) < 3:
                        continue
                    record   = json.loads(parts[2])
                    page_url = record.get("url", "")
                except (json.JSONDecodeError, KeyError):
                    continue
                if not page_url:
                    continue
                url_bytes        = len(page_url.encode("utf-8"))
                encoded          = (page_url + "\n").encode("utf-8")
                out_file.write(encoded)
                bytes_written   += len(encoded)
                url_count       += 1
                total_url_bytes += url_bytes
                if bytes_written >= TARGET_BYTES:
                    return bytes_written, url_count, total_url_bytes
    except (urllib.error.URLError, OSError) as e:
        print(f"\n  Warning: failed - {e}", file=sys.stderr)
    return bytes_written, url_count, total_url_bytes


def main():
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)

    crawl_id  = get_latest_crawl_id()
    cdx_files = get_cdx_file_list(crawl_id)

    print(f"Writing to {OUTPUT_PATH}  (target {TARGET_BYTES // 1024**2} MB)\n")

    bytes_written   = 0
    url_count       = 0
    total_url_bytes = 0

    with open(OUTPUT_PATH, "wb") as out_file:
        for i, cdx_path in enumerate(cdx_files, 1):
            shard = cdx_path.split("/")[-1]
            print(f"Shard {i}/{len(cdx_files)}: {shard}")

            bytes_written, url_count, total_url_bytes = stream_urls_from_cdx(
                cdx_path, out_file, bytes_written, url_count, total_url_bytes
            )
            print()

            if bytes_written >= TARGET_BYTES:
                break
            time.sleep(0.5)

    size_mb   = os.path.getsize(OUTPUT_PATH) / 1024 / 1024
    avg_bytes = total_url_bytes / url_count if url_count else 0
    print(f"\nDone!  {url_count:,} URLs  --  {size_mb:.1f} MB  ->  {OUTPUT_PATH}")
    print(f"Average URL length: {avg_bytes:.1f} bytes")


if __name__ == "__main__":
    main()
