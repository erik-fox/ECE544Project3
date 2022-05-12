module tachometer_timer(
    input   logic   clock,
    input   logic   system_reset,
    input   logic   tachometer_ready,
    output  logic   timer_on,
    output  logic   timer_reset
);
    
    parameter  PERCENT_SECOND   = 1000;
    parameter  CLOCK_FREQ       = 100000000;   
    localparam NUM_CLOCKS       = CLOCK_FREQ/PERCENT_SECOND;

    logic   [31:0]  counter;

    always_ff @(posedge clock)
        begin
            if(!system_reset)
                begin
                    timer_on    <= '1;
                    timer_reset <= '0;
                    counter     <= '0;
                end
            else
                begin
                    if(tachometer_ready)
                        begin
                            timer_reset <= '0;
                            timer_on    <= '1;
                            counter     <= counter + 1'b1;
                            if(counter == NUM_CLOCKS)
                                begin
                                    timer_on    <= '0;
                                end
                        end
                    else
                        begin
                            timer_reset <= '1;
                            counter     <= '0;
                        end

                end
        end