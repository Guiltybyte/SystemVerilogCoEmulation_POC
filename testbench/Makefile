#!/bin/sh
simulation:
	verilator --binary -j 0 *.sv dpi2socket.c && ./obj_dir/Vexample
hardware: 
	verilator -D=DPI --binary -j 0 *.sv dpi2socket.c  && sudo ./obj_dir/Vexample_if
