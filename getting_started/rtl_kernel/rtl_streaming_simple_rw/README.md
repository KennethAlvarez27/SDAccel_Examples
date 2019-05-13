Stream Vector Increment (RTL)
======================

This is a simple Vector Increment RTL Kernel design with 1 Stream input and 1 Stream output that demonstrates on how to process an input stream of data for computation in an application.

***KEY CONCEPTS:*** Read/Write Stream, Create/Release Stream, RTL Kernel

***KEYWORDS:*** cl_stream, CL_STREAM_EOT

## SUPPORTED PLATFORMS
Board | Software Version
------|-----------------
Xilinx Alveo U200|SDx 2019.1


##  DESIGN FILES
Application code is located in the src directory. Accelerator binary files will be compiled to the xclbin directory. The xclbin directory is required by the Makefile and its contents will be filled during compilation. A listing of all the files in this example is shown below

```
src/host.cpp
```

##  COMMAND LINE ARGUMENTS
Once the environment has been configured, the application can be executed by
```
./vadd_stream <myadder1 XCLBIN>
```

