for config in ["dense3", "dense2", "dense1"]:
    for run_id in range(1):
        for d in [['int', 25_000_000], ['rng4', 25_000_000], ['data/urls-short', 3766956], ['data/wiki', 13816679]]:
            path = (f'named-build/{config}-n3-ycsb')
            print(f'env DATA={d[0]} KEY_COUNT={d[1]} YCSB_VARIANT=6 SCAN_LENGTH=50 RUN_ID={run_id} OP_COUNT=5e6 PAYLOAD_SIZE=8 ZIPF=0.99 DENSITY=1 {path}')
