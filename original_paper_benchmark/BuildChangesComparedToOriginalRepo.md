# Local Build Adjustments

This file documents the local adjustments made to get `make all` working
again in this checkout.

## Initial Situation

The build initially failed in `test.cpp` because the benchmark code was still
using the old API:

- `DataStructureWrapper` was constructed without an argument.
- `BTreeCppPerfEventBlock` was constructed with only `BTreeCppPerfEvent` and
  `scale`.

However, the current headers expect:

- `DataStructureWrapper(bool isInt)`
- `BTreeCppPerfEventBlock(BTreeCppPerfEvent& e, DataStructureWrapper& ds, uint64_t scale = 1)`

After that, linker errors occurred for Wormhole and TLX because some targets
compile `wh_adapter.cpp` or the TLX adapter without linking the corresponding
libraries.

## Changes

### `test.cpp`

`DataStructureWrapper` is now constructed with a flag indicating whether
integer keys are used:

```cpp
DataStructureWrapper t(dataName == "int");
```

All measured blocks now also pass the data structure to
`BTreeCppPerfEventBlock`. This satisfies the new API and allows the performance
event block to read node statistics:

```cpp
BTreeCppPerfEventBlock b(e, t, count);
```

### `Makefile`

Targets that use `wh_adapter.cpp` now link against the locally built Wormhole
library:

```make
wh_link_arg = -L. -lwh
```

The named-test and TPCC targets also link against the TLX wrapper because
configurations such as `tlx` would otherwise produce undefined-symbol errors
for `TlxWrapper`.

The local `make all` target now builds:

- `test.elf`
- `optimized.elf`
- all named-test binaries
- all YCSB binaries

TPCC was removed from the default target because it requires additional system
dependencies. In an environment with TBB, it can still be built with:

```bash
make all-with-tpcc
```

If a target such as `named-build/adapt-d3-tpcc` is built, a TPCC target was
explicitly requested. Without TBB, the build now intentionally fails early
with a clear Makefile error message. To run the local benchmark build without
TPCC, use:

```bash
make all
```

To build TPCC, first install the TBB development package so that the following
header is available:

```text
tbb/parallel_for.h
```

The TPCC code was also updated to use the modern TBB API. Older TBB versions
used:

```cpp
#include <tbb/task_scheduler_init.h>
tbb::task_scheduler_init init(nthreads);
```

Newer TBB versions no longer provide this header. The following API is used
instead:

```cpp
#include <tbb/global_control.h>
tbb::global_control tbbThreads(tbb::global_control::max_allowed_parallelism, nthreads);
```

### `tpcc/newbm.cpp`

The `libaio.h` include was removed. Although the header was included, the TPCC
code did not use any `libaio` API, so this unnecessary system dependency is no
longer required.

TPCC was also updated to use the new `DataStructureWrapper` and
`BTreeCppPerfEventBlock` APIs:

```cpp
tree = new DataStructureWrapper(false);
BTreeCppPerfEventBlock b(e, *warehouse.tree, 0);
```

Here, `false` means that TPCC uses folded byte keys rather than the dedicated
integer-key path. An existing TPCC table is passed to the performance event
block to satisfy the new signature and allow node statistics to be read.
