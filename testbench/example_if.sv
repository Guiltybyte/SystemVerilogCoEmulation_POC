// interfaces with the DUT
`timescale 1ns / 1ns
interface example_if;
  localparam CLK_FREQ_MHz = 400;
  localparam CLK_PERIOD   = 1/((CLK_FREQ_MHz * 1e6) * (1e-12));

  logic      clk;
  logic      rstn;
  logic[7:0] x;
  logic[7:0] y;
  logic[8:0] z ;

  // TODO having two initial blocks gives the following error in an interface:
  `ifdef DPI
  import "DPI-C" function int init();
  `endif

  initial begin: clk_gen
    `ifdef DPI
    void'(init());
    `endif
    forever #(CLK_PERIOD/2) clk = !clk;
  end: clk_gen


  `ifdef DPI
    //export "DPI-C" task consume_sim_time;
    import "DPI-C" function void dpi_reset();  // just make it a dummy func for now
    import "DPI-C" function void dpi_init_calc(input int unsigned x_in, input int unsigned y_in);
    import "DPI-C" function int unsigned dpi_wait_rslt(/*ref int unsigned z_out*/);
    import "DPI-C" function int unsigned get_result();

    task consume_sim_time();
      #0; // timining control inside dpi exported function Not supported by verilator
    endtask: consume_sim_time

    task reset();
      dpi_reset();
    endtask: reset

    event calc_clkd;
    task initiate_calculation(
      input logic[7:0] x_in,
      input logic[7:0] y_in
      );
      // $display("[example_if] dpi_wait_rslt called");
      dpi_init_calc(
        .x_in({24'h0,x_in}), 
        .y_in({24'h0,y_in})
        );
      @(posedge clk);
      ->calc_clkd;
    endtask: initiate_calculation

    // problem here is that this is bound by initiate calc task, should just
    // tick every clock
    task wait_for_result(output logic[7:0] result);
      int unsigned intermediate_result;

      /* verilator lint_off WIDTHTRUNC */
      while(dpi_wait_rslt(/*.z_out(intermediate_result)*/)) begin
        $display("[example_if] dpi_wait_rslt called");
        //@(calc_clkd);
        @(posedge clk);
      end
      intermediate_result = get_result();
      /* verilator lint_on WIDTHTRUNC */
      $display("[example_if] dpi_wait_rslt returned intermediate_result: %0B", intermediate_result);
      $display("[example_if] dpi_wait_rslt returned intermediate_result: %0X", intermediate_result);
      result = intermediate_result[31-:8];
      $display("[example_if] dpi_wait_rslt returned result: %0B", result);
      $display("[example_if] dpi_wait_rslt returned result: %0X", result);

    endtask: wait_for_result
  `else

    task reset();
      rstn = 0;
      @(posedge clk);
      rstn = 1;
      @(posedge clk);
    endtask: reset

    event calc_clkd;
    task initiate_calculation(
      input logic[7:0] x_in,
      input logic[7:0] y_in
      );

      x = x_in;
      y = y_in;
      @(posedge clk);
      ->calc_clkd;
    endtask: initiate_calculation

    task wait_for_result(output logic[7:0] result);
      @(calc_clkd);
      result = z[7:0];
    endtask: wait_for_result
  `endif
endinterface: example_if
