`timescale 1ns / 1ns
`include "DriverStim.svh"
`include "MonitorCheck.svh"

`define DPI
module tb;

  example_if example_if_inst();

  `ifndef DPI
  example DUT(
    .clk (example_if_inst.clk),
    .rstn(example_if_inst.rstn),
    .x   (example_if_inst.x),
    .y   (example_if_inst.y),
    .z   (example_if_inst.z)
    );
  `endif


  initial begin: main
    DriverStim   driverStim = new();
    MonitorCheck monitorCheck = new();

    driverStim.bind_if(example_if_inst);
    monitorCheck.bind_if(example_if_inst);

    fork
      driverStim.run();
      monitorCheck.run();
    join_none
  end: main

endmodule: tb
