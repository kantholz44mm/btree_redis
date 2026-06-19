import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys
import time
import datetime

from ycsb2 import BTREE_PORT, REDIS_PORT, run_ycsb, YCSB_EXECUTABLE, DATA, OUT_DIR

MIN = int(sys.argv[3] or 10)
MAX = int(sys.argv[4] or 20)

def ycsb2c_insert(dbs: list[str]):
    dfs: list[pd.DataFrame] = []
    for keyCount in map(lambda n: 2 ** n, range(MIN, MAX)):
        for type in dbs:
            port = BTREE_PORT if type == 'btree' else REDIS_PORT
            data = run_ycsb(YCSB_EXECUTABLE, port, DATA, keyCount, 0)
            data = data[['op', 'duration']]
            data['type'] = type
            data['key_count'] = keyCount

            dfs.append(data)

            time.sleep(1)

    data = pd.concat(dfs)


    print(data)

    # The collected `data` DataFrame has columns: ['op', 'duration', 'type', 'key_count']
    # Normalize and prepare for plotting. Rename 'type' -> 'source' to match earlier semantics.
    data['duration'] = pd.to_numeric(data['duration'], errors='coerce')
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
    long_df = data[['op', 'source', 'duration', 'key_count']].reset_index(drop=True)
    print('\nLong-format combined results (op, source, duration, key_count):')
    print(long_df.head())

    # Compute mean duration per key_count and source
    pivot = long_df.groupby(['key_count', 'source'])['duration'].mean().unstack(fill_value=0)
    pivot = pivot.sort_index()

    out_path = OUT_DIR / f'insert_duration_by_key_count_{datetime.datetime.now().isoformat().replace(':', '-')}.png'
    out_path.parent.mkdir(parents=True, exist_ok=True)

    fig, ax = plt.subplots(figsize=(12, 6))

    labels = list(pivot.index.astype(str))
    x = np.arange(len(labels))
    num_sources = len(pivot.columns)
    width = 0.8 / max(1, num_sources)

    for i, col in enumerate(pivot.columns):
        ax.bar(x + i * width, pivot[col].values, width, label=str(col))

    ax.set_xlabel('Inserted key count')
    ax.set_ylabel('Duration')
    ax.set_title('Duration for key_count key insertions')
    ax.set_xticks(x + width * (num_sources - 1) / 2)
    ax.set_xticklabels(labels, rotation=45, ha='right')
    ax.legend(title='source')
    plt.tight_layout()

    plt.savefig(out_path)
    print(f"Saved bar chart to: {out_path}")
    plt.close(fig)