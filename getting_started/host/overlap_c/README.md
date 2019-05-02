Overlap Host and HLS kernels (C)
======================

This examples demonstrates techniques that allow user to overlap Host(CPU) and FPGA computation in an application. It will cover asynchronous operations and event object.

***KEY CONCEPTS:*** OpenCL API, Synchronize Host and FPGA, Asynchronous Processing, Events, Asynchronous memcpy

***KEYWORDS:*** cl_event, clCreateCommandQueue, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, clEnqueueMigrateMemObjects

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
src/vector_addition.cpp
```

##  COMMAND LINE ARGUMENTS
Once the environment has been configured, the application can be executed by
```
./overlap <vector_addition XCLBIN>
```

