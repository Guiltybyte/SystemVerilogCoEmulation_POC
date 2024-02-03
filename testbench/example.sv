`timescale 1ns / 1ns
module example(
  input  logic clk,
  input  logic rstn,
  input  logic[7:0] x,
  input  logic[7:0] y,
  output logic[8:0] z 
  );

  initial begin: hello
    $display("Hello, from example.sv inst");
  end: hello
  
  // 8 bit full adder
  always_ff @(posedge clk) begin
    if(!rstn) z <= '0;
    else      z <= x + y;
  end

endmodule: example
