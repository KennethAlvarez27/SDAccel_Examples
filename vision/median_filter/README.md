Median Filter
======================

This is an optimized implementation of a median filter being used to remove noise in images targeting execution on an SDAccel supported FPGA acceleration card.

### PERFORMANCE
Board|Image Size|Frames / Second
-----|-----|-----
xilinx:adm-pcie-7v3:1ddr|128 x 128|22,222
## SUPPORTED PLATFORMS
Board | Software Version
------|-----------------
Xilinx Alveo U200|SDx 2018.3
Xilinx Virtex UltraScale+ VCU1525|SDx 2018.3
Xilinx Alveo U250|SDx 2018.3


##  DESIGN FILES
Application code is located in the src directory. Accelerator binary files will be compiled to the xclbin directory. The xclbin directory is required by the Makefile and its contents will be filled during compilation. A listing of all the files in this example is shown below

```
data/goldenImage.bmp
data/inputImage.bmp
src/krnl_medianFilter.cl
src/medianFilter.cpp
```

##  COMMAND LINE ARGUMENTS
Once the environment has been configured, the application can be executed by
```
./median <krnl_median XCLBIN> ./data/inputImage.bmp ./data/goldenImage.bmp
```

