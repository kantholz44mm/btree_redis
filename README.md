# Redis, but with BTrees!

## Project structure

We have subfolders for each "part" of the implementation:

- **libbtree**
    The "meat and potatoes" of the original paper. This contains source/header files that compile into a static library, implementing (fast) BTrees.
    This library is then used for benchmarking and for the server/client implementation.
- **redis_server**
    Contains the implementation of the redis protocol using libbtree as a backing data structure.
- **benchmark**
    Contains everything related to the benchmarking part, used to verify performance of the whole thing, compared to actual redis.

## How to build:

```
mkdir -p build
cd build
cmake ..
make
```

## Testing

libbtree has unit tests. To run all of them, in your build folder, execute

```
ctest -V
```