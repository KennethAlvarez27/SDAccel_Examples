Burst Read/Write (CL)
======================

This is simple example of using AXI4-master interface for burst read and write

***KEY CONCEPTS:*** burst access

***KEYWORDS:*** param:compiler.interfaceWrBurstLen, param:compiler.interfaceRdBurstLen

## SUPPORTED PLATFORMS
Board | Software Version
------|-----------------
Xilinx Alveo U200|SDx 2018.3
Xilinx Virtex UltraScale+ VCU1525|SDx 2018.3
Xilinx Alveo U250|SDx 2018.3


##  DESIGN FILES
Application code is located in the src directory. Accelerator binary files will be compiled to the xclbin directory. The xclbin directory is required by the Makefile and its contents will be filled during compilation. A listing of all the files in this example is shown below

```
src/host.cpp
src/vadd.cl
```

##  COMMAND LINE ARGUMENTS
Once the environment has been configured, the application can be executed by
```
./host <vadd XCLBIN>
```

