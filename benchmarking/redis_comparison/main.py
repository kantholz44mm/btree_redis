import ycsb2c_get
import ycsb2c_insert
import ycsb2_memory
import ycsb2_scan
import sys

BENCHMARK = sys.argv[1]
DBS = str(sys.argv[2]).split(',')

if __name__ == '__main__':
    if BENCHMARK == 'YCSB2_GET':
        ycsb2c_get.ycsb2c_get(DBS)
    elif BENCHMARK == 'YCSB2_SET':
        ycsb2c_insert.ycsb2c_insert(DBS)
    elif BENCHMARK == 'YCSB2_MEMORY':
        ycsb2_memory.ycsb2_memory(DBS)
    elif BENCHMARK == 'YCSB2_SCAN':
        ycsb2_scan.ycsb2c_scan(DBS)
