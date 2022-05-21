module tachometer(
    input   logic           clock,
    input   logic           system_reset,
    input   logic           encoder_data,
    output  logic   [31:0]  data_out
);



    parameter  CLOCK_FREQ       = 100000000;   
    localparam NUM_CLOCKS       = CLOCK_FREQ;

    logic   [31:0]  counter;
    logic   [31:0]  pulse_counter;
    logic           edge_detect;

    always_ff @(posedge clock)
        begin
            if(!system_reset)  //on system reset, set counters, output and edge detection bit to zero.
                begin
                    counter             <= '0;
                    data_out            <= '0;
                    pulse_counter       <= '0;
                    edge_detect         <= '0;
                end
            else
                begin
                    edge_detect <= encoder_data; //store state of encoder pulse for next clock;
                    if({edge_detect,encoder_data}==2'b01)// check if encoder data was a zero and is now a 1
                        begin
                            pulse_counter <= pulse_counter + 1'b1; // if that happened then there was a positive edge so increment pulse counter
                        end
                    counter <= counter + 1'b1;//increment timer counter
                    if(counter == NUM_CLOCKS)// when count has reached time limit reset counters, and output current pulse count
                        begin
                            counter             <= '0;
                            pulse_counter       <= '0;
                            data_out            <= pulse_counter;

                        end
                end
        end



endmodule