Stream Vector Addition (Multiple Compute Units)
======================

This is a simple Vector Add C Kernel design with 2 Stream inputs and 1 Stream output that demonstrates on how to process an input stream of data for computation in an application using multiple compute units.

***KEY CONCEPTS:*** Read/Write Stream, Create/Release Stream

***KEYWORDS:*** cl_stream, CL_STREAM_EOT, Multiple Compute Units

## SUPPORTED PLATFORMS
Board | Software Version
------|-----------------
Xilinx Alveo U200|SDx 2019.1

##  DESIGN FILES
Application code is located in the src directory. Accelerator binary files will be compiled to the xclbin directory. The xclbin directory is required by the Makefile and its contents will be filled during compilation. A listing of all the files in this example is shown below

```
src/host.cpp
src/krnl_vadd.cpp
```

##  COMMAND LINE ARGUMENTS
Once the environment has been configured, the application can be executed by
```
./vadd_stream <krnl_vadd XCLBIN>
```

