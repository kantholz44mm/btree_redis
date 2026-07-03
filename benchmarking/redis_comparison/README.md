# Benchmarking script for comparing redis with btrees

## Setup

Configure the build with Release mode:

```shell
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

Build the YCSB Target:

```shell
cmake --build ./build --target ycsb2 -- -j 14
```

Build the redis_server:

```shell
cmake --build ./build --target redis_server -- -j 14
```

Setup python:

```shell
python3 -m venv .venv
.venv/bin/pip install -r benchmarking/redis_comparison/requirements.txt
```

### Starting redis and redis_server (btree implementation)

For development, the `docker-compose.yml` can be used (not representative due to potential container overhead).
The Redis server image can be built with:
```shell
docker build -f redis_server/Dockerfile -t btree-redis-server:latest .
```

For benchmarking on the VM:

Starting a redis instance:
```shell
sudo apt install redis-server
sudo cp ./benchmarking/redis_comparison/redis-override.conf /etc/redis/redis.conf
sudo systemctl start redis-server
```

Starting a BTree Server instance (default port 3000):

```shell
build/redis_server/redis_server
```

Checking both instances are running:
```shell
redis-cli ping
redis-cli -p 3000 ping
```

## Starting a benchmark

Arguments:
* `dbs`: Comma seperated list of DB-Names (z.B. `redis`, `btree`, `btree,redis`)
* `min-db-size`: Starting number of keys for the first measurement
* `max-db-size`: Last number of keys for the last measurement
* `db-size-step`: Step size for db size key count between measurements
* `scan-length`: Number of keys retrieved per range query. This is the max value for a uniform distribution from which random scan lengths are chosen.
* `op-count`: Number of range queries to do for each measurement

GET-Benchmark

```shell
.venv/bin/python3 benchmarking/redis_comparison/main.py YCSB2_GET <dbs> <min-db-size> <max-db-size> <db-size-step>
Example:
.venv/bin/python3 benchmarking/redis_comparison/main.py YCSB2_GET btree,redis 10000 1000000 10000
```

SET-Benchmark

```shell
.venv/bin/python3 benchmarking/redis_comparison/main.py YCSB2_SET <dbs> <min-db-size> <max-db-size> <db-size-step>
Example:
.venv/bin/python3 benchmarking/redis_comparison/main.py YCSB2_SET btree,redis 10000 1000000 10000
```

Memory-Benchmark

Der `INFO MEMORY`-Befehl muss den `used_memory_dataset` Wert zurückgeben.
```shell
.venv/bin/python3 benchmarking/redis_comparison/main.py YCSB2_MEMORY <dbs> <min-db-size> <max-db-size> <db-size-step>
Example:
.venv/bin/python3 benchmarking/redis_comparison/main.py YCSB2_MEMORY btree,redis 10000 1000000 10000
```

Range-Scan-Benchmark

```shell
.venv/bin/python3 benchmarking/redis_comparison/main.py YCSB2_SET <dbs> <min-db-size> <max-db-size> <db-size-step> <scan-length> <op-count>
Example:
.venv/bin/python3 benchmarking/redis_comparison/main.py YCSB2_SET btree,redis 10000 1000000 10000 1000 10000
```

## Profiling

```shell
perf record -g ./build/redis_server/redis_server
perf script > out.perf
../FlameGraph/stackcollapse-perf.pl ./out.perf > "out-$(date +%F_%H-%M-%S).collapsed"
```

## Adjusting benchmark targets

The `ops` argument to the benchmarking scripts is a comma seperated list of database names, which are defined in [get_port_for_db in ycsb2.py](ycsb2.py).