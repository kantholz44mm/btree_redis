import datetime
import sys

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

from benchmarking.redis_comparison.ycsb2 import get_port_for_db
from ycsb2 import YCSB_EXECUTABLE, DATA, run_ycsb, OUT_DIR


def ycsb2_memory(dbs: list[str]):
    MIN = int(sys.argv[3] or 10000)
    MAX = int(sys.argv[4] or 1000000)
    STEP = int(sys.argv[5] or 10000)

    key_batch_count = 10000
    dfs: list[pd.DataFrame] = []
    keyCounts = list(range(MIN, MAX+STEP, STEP))
    for (i, keyCount) in enumerate(keyCounts):
        print(f"{i}/{len(keyCounts)} - {i/len(keyCounts)*100:.0f}%")
        for db in dbs:
            data = run_ycsb(YCSB_EXECUTABLE, get_port_for_db(db), DATA, keyCount=keyCount, keyBatchCount=key_batch_count, variant=1001)
            data = data[['op', 'mem']]
            data['type'] = db
            data['key_count'] = keyCount

            dfs.append(data)

    data = pd.concat(dfs)



    print(data)

    # The collected `data` DataFrame has columns: ['op', 'mem', 'type', 'key_count']
    # Normalize and prepare for plotting. Rename 'type' -> 'source' to match earlier semantics.
    data['mem'] = pd.to_numeric(data['mem'], errors='coerce')
    data = data.dropna(subset=['op', 'mem', 'type'])
    data = data.rename(columns={'type': 'source'})

    print(f"Prepared data for plotting: {data.shape[0]} rows")
    print(data.head())

    data = data[data['op'] == 'ycsb_memory_measure']

    # Ensure key_count is numeric and present
    data['key_count'] = pd.to_numeric(data['key_count'], errors='coerce')
    data = data.dropna(subset=['key_count'])
    data['key_count'] = data['key_count'].astype(int)

    # Long-format DataFrame with required columns
    long_df = data[['op', 'source', 'mem', 'key_count']].reset_index(drop=True)
    print('\nLong-format combined results (op, source, mem, key_count):')
    print(long_df.head())

    # Compute mean mem per key_count and source
    pivot = long_df.groupby(['key_count', 'source'])['mem'].mean().unstack(fill_value=0)
    pivot = pivot.sort_index()

    out_path = OUT_DIR / f'get_mem_by_key_count_{datetime.datetime.now().isoformat().replace(':', '-')}.png'
    out_path.parent.mkdir(parents=True, exist_ok=True)

    fig, ax = plt.subplots(figsize=(12, 6))

    labels = list(pivot.index.astype(str))
    x = np.arange(len(labels))

    for i, col in enumerate(pivot.columns):
        ax.plot(x, pivot[col].values, marker='o', label=str(col))

    ax.set_xlabel('DB size (key count)')
    ax.set_ylabel('Memory (B)')
    ax.set_title(f'Memory by db size')

    tick_interval = max(1, len(labels) // 20)
    tick_indices = np.arange(0, len(labels), tick_interval)
    ax.set_xticks(tick_indices)
    ax.set_xticklabels([labels[i] for i in tick_indices], rotation=45, ha='right')
    ax.legend(title='source')
    plt.tight_layout()

    plt.savefig(out_path)
    print(f"Saved bar chart to: {out_path}")
    plt.close(fig)