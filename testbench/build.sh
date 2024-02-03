#!/bin/sh
verilator --binary -j 0 *.sv dpi2socket.c | tee log.log
