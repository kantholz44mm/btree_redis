for run_id in range(1):
    for d in [['int', 25_000_000], ['rng4', 25_000_000], ['data/urls-short', 3766956], ['data/wiki', 13816679]]:
        for config in ["wh"]:
            path = (f'named-build/{config}-n3-ycsb')
            print(f'env LD_LIBRARY_PATH=. DATA={d[0]} KEY_COUNT={d[1]} YCSB_VARIANT=6 SCAN_LENGTH=50 RUN_ID={run_id} OP_COUNT=5e6 PAYLOAD_SIZE=8 ZIPF=0.99 DENSITY=1 {path}')
