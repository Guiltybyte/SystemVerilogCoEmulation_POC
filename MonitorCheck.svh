// Monitor returns from interface and check them
class MonitorCheck;
  localparam NUM_TXNS = 10;
  protected virtual example_if v_if;

  task run();
    logic[7:0] result;
    int txns_received = 0;

    forever begin
      v_if.wait_for_result(result);
      $display(
        "[MonitorCheck] (%d) result %X", 
        txns_received+1, result
        );
      if(++txns_received == NUM_TXNS) 
        $finish("Simulation Finished");
    end
  endtask: run

  function void bind_if(virtual example_if v_if);
    this.v_if = v_if;
  endfunction: bind_if
endclass: MonitorCheck
