// Create stimulus and Drive the interface
class DriverStim;
  localparam NUM_TXNS = 10;
  protected virtual example_if v_if;

  task run();
    bit[7:0] x;
    bit[7:0] y;

    v_if.reset();
    for(int i=0; i<NUM_TXNS; i++)begin
      x++; 
      y++; 

      $display("[DriverStim] initiating calculation, x: %8X y: %8X", x, y);
      v_if.initiate_calculation(x, y);
    end
  endtask: run

  function void bind_if(virtual example_if v_if);
    this.v_if = v_if;
  endfunction: bind_if
endclass: DriverStim
