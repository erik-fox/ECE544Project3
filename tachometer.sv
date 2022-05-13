module tachometer(
    input   logic           clock,
    input   logic           system_reset,
    output  logic   [31:0]  data_out
);


    parameter  PERCENT_SECOND   = 1000;
    parameter  CLOCK_FREQ       = 100000000;   
    localparam NUM_CLOCKS       = CLOCK_FREQ/PERCENT_SECOND;

    logic   [31:0]  counter;
    logic   [31:0]  pulse_counter;
    logic   [31:0]  previous_pulse;

    always_ff @(posedge clock)
        begin
            if(!system_reset)  
                begin
                    counter         <= '0;
                    data_out        <= '0;
                    previous_pulse  <= '0;
                end
            else
                begin
                    counter <= counter + 1'b1;
                    if(counter == NUM_CLOCKS)
                        begin
                            counter         <= 0;
                            previous_pulse  <= pulse_counter;
                            data_out        <= pulse_counter - previous_pulse;

                        end
                end
        end

    always_ff @( posedge encoder_in ||  !system_reset)
        begin
            if(!system_reset) 
                begin
                    pulse_counter       <= '0;
                end
            else
                begin
                    pulse_counter   <= pulse_counter + 1'b1;
                end
        end

endmodule
