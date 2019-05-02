Multiple RTL Kernels (RTL)
======================

This example has two RTL Kernels. Both Kernel_0 and Kernel_1 perform vector addition. The Kernel_1 reads the output from Kernel_0 as one of two inputs.

***KEY CONCEPTS:*** Multiple RTL Kernels

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
src/ip_0/hdl/krnl_vadd_rtl_0.v
src/ip_0/hdl/krnl_vadd_rtl_adder.sv
src/ip_0/hdl/krnl_vadd_rtl_axi_read_master.sv
src/ip_0/hdl/krnl_vadd_rtl_axi_write_master.sv
src/ip_0/hdl/krnl_vadd_rtl_control_s_axi.v
src/ip_0/hdl/krnl_vadd_rtl_counter.sv
src/ip_0/hdl/krnl_vadd_rtl_int.sv
src/ip_1/hdl/krnl_vadd_rtl_1.v
src/ip_1/hdl/krnl_vadd_rtl_adder.sv
src/ip_1/hdl/krnl_vadd_rtl_axi_read_master.sv
src/ip_1/hdl/krnl_vadd_rtl_axi_write_master.sv
src/ip_1/hdl/krnl_vadd_rtl_control_s_axi.v
src/ip_1/hdl/krnl_vadd_rtl_counter.sv
src/ip_1/hdl/krnl_vadd_rtl_int.sv
src/kernel_0.xml
src/kernel_1.xml
```

##  COMMAND LINE ARGUMENTS
Once the environment has been configured, the application can be executed by
```
./host <vadd XCLBIN>
```

