Nearest Neighbor Linear Search
======================

This file contains the following sections:

1. SUPPORTED PLATFORMS
2. DESIGN FILES
3. COMMAND LINE ARGUMENTS


## 1. SUPPORTED PLATFORMS
Board | Software Version
------|-----------------
Xilinx Alveo U200|SDx 2018.3
Xilinx Virtex UltraScale+ VCU1525|SDx 2018.3
Xilinx Alveo U250|SDx 2018.3


## 2. DESIGN FILES
Application code is located in the src directory. Accelerator binary files will be compiled to the xclbin directory. The xclbin directory is required by the Makefile and its contents will be filled during compilation. A listing of all the files in this example is shown below

```
data/queries.txt
data/targets.txt
src/krnl_linear_search.cpp
src/linear_search.cpp
src/linear_search.h
```

## 3. COMMAND LINE ARGUMENTS
Once the environment has been configured, the application can be executed by
```
./nearest <krnl_nearest XCLBIN> ./data/queries.txt ./data/targets.txt
```

