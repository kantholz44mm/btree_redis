import pandas as pd
import subprocess
import sys
import io
import uuid
import os
from pathlib import Path

PROJECT_ROOT = Path(__file__).parent.parent.parent
REDIS_COMPARISON_ROOT = PROJECT_ROOT / 'benchmarking' / 'redis_comparison'
OUT_DIR = REDIS_COMPARISON_ROOT / 'out'

YCSB_EXECUTABLE = os.getenv('YCSB_EXECUTABLE', str(PROJECT_ROOT / 'build' / 'benchmarking' / 'ycsb2' / 'ycsb2'))

REDIS_PORT = '6379'
BTREE_PORT = '3000'

DATA = 'rng4'
# DATA = PROJECT_ROOT / 'data' / 'wikipedia_titles.txt'

def run_ycsb(executable: str, port: str, data: str, keyCount: int, opCount: int = 0, opBatchCount: int = 200, keyBatchCount = 200, variant = 3) -> pd.DataFrame:
    env = os.environ.copy()
    env.update({
        'REDIS_HOST': '127.0.0.1',
        'REDIS_PORT': port,
        'DATA': data,
        'DENSITY': '0.5',
        'KEY_COUNT': str(keyCount),
        'KEY_BATCH_COUNT': str(keyBatchCount),
        'OP_COUNT': str(opCount),
        'OP_BATCH_COUNT': str(opBatchCount),
        'PAYLOAD_SIZE': '10',
        'RUN_ID': str(uuid.uuid4()),
        'SCAN_LENGTH': '100',
        'YCSB_VARIANT': str(variant),
        'ZIPF': '-1',
    })

    proc = subprocess.Popen(
        executable,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        env=env,
        text=True,
        bufsize=1,
    )
    collected_lines = []
    try:
        assert proc.stdout is not None
        for line in proc.stdout:
            collected_lines.append(line)
            sys.stdout.write(line)
            sys.stdout.flush()
    except KeyboardInterrupt:
        proc.kill()
        proc.wait()
        raise

    ret = proc.wait()
    if ret != 0:
        raise subprocess.CalledProcessError(ret, executable)

    output_text = ''.join(collected_lines)

    df = pd.read_csv(io.StringIO(output_text), skipinitialspace=True)

    df.columns = df.columns.str.strip()

    obj_cols = df.select_dtypes(include=['object']).columns
    for col in obj_cols:
        df[col] = df[col].astype(str).str.strip()

    if 'duration' in df.columns:
        df['duration'] = pd.to_numeric(df['duration'], errors='coerce')

    return df