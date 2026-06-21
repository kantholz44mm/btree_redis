import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys
import time
import datetime

from ycsb2 import BTREE_PORT, REDIS_PORT, run_ycsb, YCSB_EXECUTABLE, DATA, OUT_DIR

MIN = int(sys.argv[3] or 1)
MAX = int(sys.argv[4] or 50)

def ycsb2c_insert(dbs: list[str]):
    dfs: list[pd.DataFrame] = []
    key_batch_count = 10000
    for batches in range(MIN, MAX):
        keyCount = batches * key_batch_count
        for type in dbs:
            port = BTREE_PORT if type == 'btree' else REDIS_PORT
            data = run_ycsb(YCSB_EXECUTABLE, port, DATA, keyCount=keyCount, keyBatchCount=key_batch_count, opCount=0)
            data = data[['op', 'duration']]
            data['type'] = type
            data['key_count'] = keyCount
            data['avg_per_key'] = data['duration'] / keyCount

            dfs.append(data)

            time.sleep(1)

    data = pd.concat(dfs)


    print(data)

    # The collected `data` DataFrame has columns: ['op', 'duration', 'type', 'key_count']
    # Normalize and prepare for plotting. Rename 'type' -> 'source' to match earlier semantics.
    data['duration'] = pd.to_numeric(data['duration'], errors='coerce')
    data['avg_per_key'] = pd.to_numeric(data['avg_per_key'], errors='coerce')
    data = data.dropna(subset=['op', 'duration', 'type'])
    data = data.rename(columns={'type': 'source'})

    print(f"Prepared data for plotting: {data.shape[0]} rows")
    print(data.head())

    data = data[data['op'] == 'ycsb_c_init']

    # Ensure key_count is numeric and present
    data['key_count'] = pd.to_numeric(data['key_count'], errors='coerce')
    data = data.dropna(subset=['key_count'])
    data['key_count'] = data['key_count'].astype(int)

    # Long-format DataFrame with required columns
    long_df = data[['op', 'source', 'avg_per_key', 'key_count']].reset_index(drop=True)
    print('\nLong-format combined results (op, source, avg_per_key, key_count):')
    print(long_df.head())

    # Compute mean avg_per_key per key_count and source
    pivot = long_df.groupby(['key_count', 'source'])['avg_per_key'].mean().unstack(fill_value=0)
    pivot = pivot.sort_index()

    out_path = OUT_DIR / f'insert_avg_per_key_by_key_count_{datetime.datetime.now().isoformat().replace(':', '-')}.png'
    out_path.parent.mkdir(parents=True, exist_ok=True)

    fig, ax = plt.subplots(figsize=(12, 6))

    for col in pivot.columns:
        ax.plot(pivot.index, pivot[col].values, marker='o', label=str(col), linewidth=2, markersize=8)

    ax.set_xlabel('Inserted key count')
    ax.set_ylabel('Avg. duration per key (s)')
    ax.set_title('Average duration for key_count key insertions')
    # ax.set_xscale('log')
    # ax.set_yscale('log')
    ax.legend(title='source')
    ax.grid(True, which='both', alpha=0.3)
    plt.tight_layout()

    plt.savefig(out_path)
    print(f"Saved bar chart to: {out_path}")
    plt.close(fig)
