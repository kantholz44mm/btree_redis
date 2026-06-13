import ycsb2c_get
import ycsb2c_insert
import os
import sys

BENCHMARK = sys.argv[1]

if __name__ == '__main__':
    if BENCHMARK == 'YCSB2_GET':
        ycsb2c_get.ycsb2c_get()
    elif BENCHMARK == 'YCSB2_SET':
        ycsb2c_insert.ycsb2c_insert()
