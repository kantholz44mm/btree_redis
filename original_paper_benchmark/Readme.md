# Steps
1. Install the necessary dependencies
```sh
sudo apt install make cmake clang g++ cargo rustc r-base r-base-dev parallel coreutils
```

2. build the project using the command
```sh
make all
make ycsb-all
```

3. Run the build_all_var_page_size script
```sh
./build_all_var_page_size.sh
```

4. Run the chrismas-run script
```sh
./R/chrismas-run.sh
```

# Possible `/R/chrismas-run.sh` problems
5 scripts are run on the shellscript
1. python3 R/eval-2/re-eval.py  
2. python3 R/in-memory-skew/skew-op2.py
3. python3 R/size3/vary6.py 
4. python3 R/eval-dense/var-density-new-op.py
5. python3 R/eval-dense/dense-tasks-op2.py 

## Scripts 1 & 2
Python scripts 1 and 2 may throw the following errors:
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
if the project was not build (`make all`)

## Scripts 3, 4 and 5
Python scripts 3, 4 and 5 may throw the following errors...
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
if the shell script `./build_all_var_page_size.sh` was not executed (it builds the project multiple times with different node sizes)

# problem "Error opening counter cycle"
If you get a problem "Error opening counter cycle", it apparently comes from perf. 
`sudo sysctl -w kernel.perf_event_paranoid=0` is the solution, it gives full access to HW counters

# Benchmark duration

Hardware Specs: ...TODO

|Benchmark file | Starting time | End time | Duration |
|---|---|---|---|
| R/eval-2/re-eval.py | 05.05.2026 10:05:24 | 06.05.2026 01:21:31 | 15:16:07 |
| R/in-memory-skew/skew-op2.py | 06.05.2026 01:21:32 | 08.05.2026 15:22:32 | 62:01:00
| R/size3/vary6.py | 08.05.2026 15:22:32 | 10.05.2026 15:18:26 | 47:55:54 |
| R/eval-dense/var-density-new-op.py | 10.05.2026 15:18:54 | 11.05.2026 11:21:07 | 20:02:13 |
| R/eval-dense/dense-tasks-op2.py | 11.05.2026 11:21:41 | 11.05.2026 19:35:42 | 8:14:01 |

# Old readme content
This repository contains suplementary data for our paper "B-Trees Are Back: Engineering Fast and Pageable Node Layouts".
The B-Tree implementation contained is to help others reproduce our findings.
It is not suitable for production use.

# Benchmarking Binaries
The main artifacts produced are benchmarking binaries produced from the makefile target `ycsb-all`.
Each binary implements benchmarking of one data structure configuration and follows the naming scheme `<config>-<debug_assertions><optimization_level>-ycsb`.
`config` specifies corresponds to one of the configurations in the `named-configs` directory.
There are configurations for the B-Tree with various optimizations enabled, as well as for different in-memory data structures.
Each binary is configured via a set of environment variables, which are processed in `ycsb2.cpp`

# Variable Page size
By default, all B-Trees use a node size of 4KiB.
The script `build_var_page_size.sh` is used to build B-Tree configurations with different node sizes.
Examples can be found as comments in the script file.

# R
Each subdirectory of the `R` directory contains python scripts to generate benchmarks, results of said benchmarks as csv files, and R code to analyze the results.
The output of each python script is a sequence of program invocations, intended to be piped into GNU parallel.
The csv file containing the results is generally named after the python script that produced it.




# Gespäch mit Piepmeyer

## Erkenntnisse:
- Reproduzierbarkeit einzelner Benchmarkingergebnisse möglich
- Reproduzierbarkeit aller Graphen nicht möglich:
  - Unübersichtlichkeit und Inhomogenität der Build-Strukturen:
    - 32 Python Skripte, welche "Benchmarks generieren"
      - Siehe joblog
  - Nichtverfolgbarkeit der Herkunft & Benennung der Graphen im Paper

- Neuschreiben der tests in python selbe logik besser verpackt (niemand versteht R)
- Anzweifel der sinnhaftigheit (random number generator in ycsb.cpp:448)
--> eigene Bechmarks weil:
    zu komplexe Strucktur / Nameschema zu komplex um sich einzuarbeiten 
    Reproduzierbarkeit Zitat Matthias "Nein"

Wie weiter arbeiten?

Größe der Datensätze und deren länge sehr wählerich skaliert nicht mit