Edge Detection
======================

Implementation of a Sobel Filter for edge detection.

## SUPPORTED PLATFORMS
Board | Software Version
------|-----------------
Xilinx Alveo U250|SDx 2018.3
Xilinx Alveo U200|SDx 2018.3
Xilinx Alveo U280 ES|SDx 2018.3
Xilinx Virtex UltraScale+ VCU1525|SDx 2018.3


##  DESIGN FILES
Application code is located in the src directory. Accelerator binary files will be compiled to the xclbin directory. The xclbin directory is required by the Makefile and its contents will be filled during compilation. A listing of all the files in this example is shown below

```
data/input/CF000221.jpg
data/input/eiffel.bmp
data/input/lola.bmp
data/input/vase.bmp
data/output/lola.bmp
src/edge.cpp
src/edge.h
src/krnl_sobelfilter.cl
```

##  COMMAND LINE ARGUMENTS
Once the environment has been configured, the application can be executed by
```
./edge <krnl_edge XCLBIN> ./data/input/eiffel.bmp
```

