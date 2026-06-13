# Benchmarking script for comparing redis with btrees

## Setup

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

For development, the `docker-compose.yml` can be used.

For benchmarking on the VM:

Starting a redis instance:
```shell
sudo apt install redis-server
sudo cp ./benchmarking/redis_comparison/redis-override.conf /etc/redis/redis.conf
sudo systemctl start redis-server
```

Starting a BTree Server instance:

```shell
build/redis_server/redis_server
```

Checking both instances:
```shell
redis-cli ping
redis-cli -p 3000 ping
```

## Starting a benchmark

```shell
.venv/bin/python3 benchmarking/redis_comparison/main.py YCSB2_GET 10 25
.venv/bin/python3 benchmarking/redis_comparison/main.py YCSB2_SET 10 25
```