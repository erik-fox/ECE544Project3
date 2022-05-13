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
    logic           tachometer_reset;
    logic           tachometer_ready;

    always_ff @(posedge clock)
        begin
            if(!system_reset)  //On system reset set counter and data out to zero and flag tachometer for reset
                begin
                    counter             <= '0;
                    data_out            <= '0;
                    tachometer_reset    <= '1;
                end
            else
                begin
                    if(tachometer_ready)// don't deassert tachometer reset until its occured
                        begin
                            tachometer_reset    <= '0;
                        end
                    counter <= counter + 1'b1;//increment counter
                    if(counter == NUM_CLOCKS)// when count has reached time limit reset counter, output current pulse count, and initiate tachometer reset
                        begin
                            counter             <= 0;
                            data_out            <= pulse_counter;
                            tachometer_reset    <= '1;

                        end
                end
        end

    always_ff @( posedge encoder_in )
        begin
            if(tachometer_reset) //wait for reset from timer process
                begin
                    pulse_counter       <= 32'h00000001;// set pulse count to 1
                    tachometer_ready    <= '1; // tell timer process that you have reset so it can deassert the signal
                end
            else
                begin
                    pulse_counter       <= pulse_counter + 1'b1;// increment pulse counter
                    tachometer_ready    <= '0; // deassert tachometer ready signal to prepare for next tachometer reset
                end
        end

endmodule