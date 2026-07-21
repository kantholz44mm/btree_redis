# General information & Hardware specs
All benchmarks were conducted on a virtual machine (VM). Therefore, the results may be affected by virtualization overhead, shared host resources, and variations in CPU and I/O performance.

| Component | Specification |
|---|---|
| Machine Type | KVM virtual machine |
| CPU | AMD EPYC 7702P 64-Core Processor |
| Allocated CPU Cores | 4 vCPUs |
| Threads per Core | 1 |
| CPU Architecture | x86-64 |
| L1 Data Cache | 256 KiB total, 64 KiB per core |
| L1 Instruction Cache | 256 KiB total, 64 KiB per core |
| L2 Cache | 2 MiB total, 512 KiB per core |
| L3 Cache | 64 MiB total |
| RAM | 62 GiB |
| Swap | None |
| Storage | 70 GB virtual QEMU disk |
| File System | ext4 |
| GPU | Red Hat Virtio 1.0 virtual GPU |
| NUMA Nodes | 1 |
| Virtualization | KVM with AMD-V |
| Operating System | Debian 13 |
| Linux Kernel | 7.0.4 |

# Setup & Benchmarking steps
## 1. Install the necessary dependencies
General dependencies:
```sh
sudo apt update
sudo apt install -y make cmake clang g++ cargo rustc r-base r-base-dev parallel coreutils gzip build-essential libcurl4-openssl-dev libssl-dev libxml2-dev libfontconfig1-dev
```

R-script dependencies:
```sh
R -e 'install.packages(
  c(
    "ggplot2",
    "sqldf",
    "ggh4x",
    "dplyr",
    "stringr",
    "scales",
    "tidyr",
    "patchwork",
    "forcats",
    "readr",
    "RColorBrewer",
    "ggpattern",
    "fs",
    "vroom"
  ),
  repos = "https://cloud.r-project.org"
)'
```

## 2. Build the project 
Build the project + ycsb parts + project with different node and leaf sizes using the following three commands:
```sh
make all
make ycsb-all
./build_all_var_page_size.sh
```

## 3. Set additional commands & environmental variables
- Before running any benchmarks, please execute the following commands from the **`./R`** folder to avoid errors
```sh
export LD_LIBRARY_PATH=$PWD:$LD_LIBRARY_PATH
sudo sysctl -w kernel.perf_event_paranoid=0
```

## 4. Run the Benchmark(s)
All the available benchmarks are found inside the R subfolders as Python scripts. Benchmarks were run using Python 3.13.5. 

Benchmarks are executed in the following manner:
```sh
python3 R/<subfolder>/<benchmarkName>.py| shuf |parallel -j1 --timeout 600 --joblog joblog-<benchmarkName> -- {1}| tee R/<subfolder>/<benchmarkName>.csv
```
Example:
```sh
python3 R/eval-2/re-eval.py| shuf |parallel -j1 --timeout 600 --joblog joblog-re-eval -- {1}| tee R/eval-2/re-eval.csv
```
*Note: It is recommended to take a look at the `/R/chrismas-run.sh` file (as it contains a list of all commands used to execute all benchmarks) as well as the approximate benchmark running times found at the [end of the document](#estimate-benchmark-running-time)*

---

### Benchmark outputs
While running *any* benchmark, the following files are created and continously written to:
- <u>joblog-\<benchmarkName\></u>: This file contains additional information related to the benchmark, including the command being run, when the command was run (in UNIX time), and the runtime of each command. Name Example: `joblog-re-eval`

- <u>\<benchmarkName>.csv</u>: benchmark output containing all the necessary data for the R script to create a visualization.

## 6. Compress the produced csv files in csv.gz files
R-Script makes use of csv.gz files to generate the visualization
```sh
gzip <benchmarkName>.csv
```
use the flag `-d` if it is necesary to keep the original csv file

## 7. Visualize the benchmark data
Benchmark data is visualized with the help of R scripts located on each subfolder. Not all graphs can be created due to missing benchmark data 

Additionally the file `run_all.R` was created to execute each single R script. It can be run by using the command
```sh
Rscript -f ./R/run_all.R
```
In the case csv files are missing, it will just jump to the next .R file.

# Benchmarking Problems & Solutions
**Problem**:
```sh
env: ‘named-build/art-n3-ycsb’: No such file or directory
env: ‘named-build/baseline-n3-ycsb’: No such file or directory
env: ‘named-build/dense1-n3-ycsb’: No such file or directory
env: ‘named-build/dense2-n3-ycsb’: No such file or directory
env: ‘named-build/dense3-n3-ycsb’: No such file or directory
env: ‘named-build/hash-n3-ycsb’: No such file or directory
env: ‘named-build/heads-n3-ycsb’: No such file or directory
env: ‘named-build/hints-n3-ycsb’: No such file or directory
env: ‘named-build/hot-n3-ycsb’: No such file or directory
env: ‘named-build/inner-n3-ycsb’: No such file or directory
env: ‘named-build/prefix-n3-ycsb’: No such file or directory
env: ‘named-build/tlx-n3-ycsb’: No such file or directory
```
**Solution**: build the project using `make all`

---

**Problem**:
```sh
env: ‘page-size-builds/_DPS_I_1024__DPS_L_4096/dense2-n3-ycsb’: No such file or directory
env: ‘page-size-builds/_DPS_I_16384__DPS_L_4096/hash-n3-ycsb’: No such file or directory
env: ‘page-size-builds/_DPS_I_256__DPS_L_4096/hints-n3-ycsb’: No such file or directory
env: ‘page-size-builds/_DPS_I_32768__DPS_L_4096/baseline-n3-ycsb’: No such file or directory
env: ‘page-size-builds/_DPS_I_4096__DPS_L_2048/prefix-n3-ycsb’: No such file or directory
env: ‘page-size-builds/_DPS_I_4096__DPS_L_256/heads-n3-ycsb’: No such file or directory
env: ‘page-size-builds/_DPS_I_4096__DPS_L_4096/baseline-n3-ycsb’: No such file or directory
env: ‘page-size-builds/_DPS_I_4096__DPS_L_512/dense3-n3-ycsb’: No such file or directory
env: ‘page-size-builds/_DPS_I_4096__DPS_L_8192/hints-n3-ycsb’: No such file or directory
env: ‘page-size-builds/_DPS_I_512__DPS_L_4096/prefix-n3-ycsb’: No such file or directory
env: ‘page-size-builds/_DPS_I_8192__DPS_L_4096/dense3-n3-ycsb’: No such file or directory
```
**Solution**: execute the shell script `./build_all_var_page_size.sh` to build the project multiple times with different node sizes

---

**Problem**: "Error opening counter cycle"

**Solution**: Give full access to hardware counters by running the command `sudo sysctl -w kernel.perf_event_paranoid=0`

---

**Problem**: Error while loading shared libraries when they exist, e.g.:
```sh
python3 R/eval-2/re-eval-dense.py | shuf | parallel -j1 --timeout 600 --joblog joblog-re-eval-dense -- {1} | tee R/eval-2/re-eval-dense.csv
named-build/dense2-n3-ycsb: error while loading shared libraries: libwh.so: cannot open shared object file: No such file or directory
named-build/dense3-n3-ycsb: error while loading shared libraries: libwh.so: cannot open shared object file: No such file or directory
named-build/dense1-n3-ycsb: error while loading shared libraries: libwh.so: cannot open shared object file: No such file or directory
```

**Solution**: `export LD_LIBRARY_PATH=$PWD:$LD_LIBRARY_PATH`

# Miscallaneous

## Estimate benchmark running time

| Benchmark generator | Duration | Note |
|---|---|---|
| `R/avg-fanout/avg-fanout.py` | — | This command cannot be execute on the VM, as it causes all the RAM (64 gb) to be used|
| `R/eval-dense/dense-tasks-op2.py` | — | |
| `R/eval-dense/dense-tasks.py` | 02:45:56 | | 
| `R/eval-2/eval-wormhole.py` | 00:06:58 | | 
| `R/in-mem-size/in-mem-size.py` | 01:26:21 | | 
| `R/in-mem-size/in-mem-size-lits.py` | 00:32:22 | | 
| `R/size3/leaf-size-density.py` | 00:39:27 | |
| `R/eval-dense/partition-id-hint.py` | 01:23:56 | |
| `R/eval-2/re-eval.py` | 00:00:10 | |
| `R/eval-2/re-eval-dense.py` | 00:20:40 | |
| `R/eval-2/re-eval-rng8.py` | 00:33:22 | |
| `R/in-memory-skew/skew.py` | 2 days 22:19:26 | |
| `R/in-memory-skew/skew2-wh.py` | 1 day 07:14:22 | |
| `R/in-memory-skew/skew3.py` | 1 day 10:36:02 | |
| `R/space-0-payload/space-0-payload.py` | 01:09:46 | |
| `R/eval-dense/task-sorted-insert.py` | 01:27:01 | |
| `R/size3/vary1.py` | 09:21:44 | Possibly useless, v7 |
| `R/size3/vary2.py` | 07:13:06 | |
| `R/size3/vary3.py` | 02:04:30 | |
| `R/size3/vary4.py` | 01:35:25 | |
| `R/size3/vary5.py` | 04:41:44 | |
| `R/size3/vary6.py` | 00:00:10 | |
| `R/size3/vary7.py` | 01:32:42 | |
| `R/eval-dense/var-density.py` | |
| `R/eval-dense/var-density-e.py` | |

## Matplotlib Comparison Plots

The ⁠ *matplotlib.py ⁠is an additional python script used to visualize and compare two groups of benchmark results:
- <u>Original Paper:</u> Measurements provided by the authors of the original paper. These input files generally use the prefix ⁠`paper-sigmod25-`
- <u>⁠Our Measurements:</u> Benchmark results produced locally on our virtual machine.

The scripts aggregate repeated measurements using the median and display the original and locally reproduced results in separate plot rows.

The following comparison scripts are available:

| Script | Comparison | Default Output Path|
|---|---|---|
| ⁠`R/in-mem-size/in_mem_size_matplotlib.py `⁠ | Memory consumption of the evaluated data structures | ⁠`R/in-mem-size/out/in-mem-size-comparison.png` ⁠ |
| ⁠`R/eval-dense/dense_partition_matplotlib.py` ⁠ | Insert throughput and space consumption for partitioned workloads | ⁠`R/eval-dense/out/dense-partition-comparison.png`⁠ |
| `R/in-memory-skew/zipf_in_mem_matplotlib.py` | Lookup performance under different Zipf distributions, normalized against Adapt | `R/in-memory-skew/out/zipf-in-mem-comparison.png` ⁠ |

To run these additional graphs, it is necessary to install Matplotlib:

```sh
python3 -m pip install matplotlib
```

Run the scripts from the repository root:
```bash
python3 R/in-mem-size/in_mem_size_matplotlib.py
python3 R/eval-dense/dense_partition_matplotlib.py
python3 R/in-memory-skew/zipf_in_mem_matplotlib.py
```

The default input files can be replaced using the ⁠ --original ⁠ and ⁠ --ours ⁠ options:

```bash
python3 R/eval-dense/dense_partition_matplotlib.py \
  --original path/to/original.csv.gz \
  --ours path/to/our-results.csv.gz \
  --output path/to/comparison.png
```

## Self-made data
During the benchmarking process, we decided to create our own data (urls, urls-short, wiki), due to the original data not being included on the paper's repository. However at a late date, the original data was given to use by the paper's author. Even if the data we created was not used to create the final benchmark results, it is stored under `./data/self-made-data` just in case. 

## Missing CSVs for existing R-Scripts
This table provides an overview of the existing R script files in each subfolder, as well as the CSV files that are expected but can or cannot be generated. The reasons for certain CSV files to be missing are that:
- There is no benchmark that produces that csv file (benchmark name = csv file name) 
- The author renamed the outputted csv files, making it harder/impossible to trace the file back to the benchmark that generated it (e.g. on the original repository the author renamed the file vary7.csv.gz to v7.csv.gz)

|R-Skript | Missing CSV file(s) | Existing used CSV file(s) | Existing not used CSV file(s)
|---|---|---|--|
`adapt/adapt.R` |adapt-cmp-seq.csv, adapt-cmp-seq-hot-fixed.csv.gz | - | - 
`alignment/alignment.R` | - | alignment.csv | - 
`art-btree-initial/R.R` | d2.csv | - | -
`avg-fanout/avg-fanout.R` | avg-fanout-2.csv.gz | avg-fanout.csv.gz | -
`eval-2/eval-2.R` | - | re-eval-rng8.csv.gz, re-eval-dense.csv.gz, eval-wormhole.csv.gz | -    
`hash-capacity-change/R.R` | d1.csv | keine CSV-Dateien
`head-distinct-count/head-distinct-count.R` | wiki.csv.gz, urls-short.csv.gz | keine CSV-Dateien 
`heads-int-size/his2.R` | pl-2.csv | keine CSV-Dateien
`heads-int-size/insert-log.R` | ours-fixed-pl/heads-25e6.csv.gz, ours-fixed-pl/prefix-25e6.csv.gz | keine CSV-Dateien
`in-mem-size/in-mem-size.R` | lits.csv.gz | in-mem-size.csv.gz, in-mem-size-lits.csv.gz
`in-memory-skew/in-memory-skew.R` | skew3b.csv.gz, lits.csv.gz | skew.csv.gz, skew2-wh.csv.gz, skew3.csv.gz
`int-lookup-small/R.R` | d1.csv, d1l.csv, d1d.csv | keine CSV-Dateien
`var-adapt-threshold/var-adapt-threshold.csv.R` | seq.csv | keine CSV-Dateien
`ycsb-2/R.R` | full-seq.csv, dense-broken2.csv | keine CSV-Dateien

## Benchmark Loops
The original repository performed the same benchmark several times. This made it possible to build the average across all loops at the cost of increasing execution time. Because of time limitations we altered the amount of loops performed from 100/20/10/5 to 1.