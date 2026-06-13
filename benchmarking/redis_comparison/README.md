# Benchmarking script for comparing redis with btrees

## Setup

Build the YCSB Target:

```shell
cmake --build ./build --target ycsb2 -- -j 14
```

Setup python:

```shell
python3 -m venv .venv
.venv/bin/pip install -r benchmarking/redis_comparison/requirements.txt
```

### Starting redis and redis_server (btree implementation)

For development, the `docker-compose.yml` can be used.

For benchmarking on the VM:

```shell

````

## Starting a benchmark

```shell
.venv/bin/python3 benchmarking/redis_comparison/main.py YCSB2_GET 10 25
.venv/bin/python3 benchmarking/redis_comparison/main.py YCSB2_SET 10 25
```