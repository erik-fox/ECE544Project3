module tachometer(
    input   logic           system_reset,
    input   logic           timer_reset,
    input   logic           timer_on,
    input   logic           encoder_in,
    output  logic           tachometer_ready,
    output  logic   [31:0]  data_out
);

    logic   [31:0]  pulse_counter;

    always_ff @( posedge encoder_in)
        begin
            if(!system_reset || timer_reset ) 
                begin
                    pulse_counter       <= '0;
                    data_out            <= '0;
                    tachometer_ready    <= '1;
                   
                end
            else
                begin
                    if(timer_on)
                        begin
                            pulse_counter       <= pulse_counter + 1'b1;
                        end
                    else
                        begin
                            tachometer_ready    <= '0;
                            data_out            <= pulse_counter;
                        end

                end
        end

endmodule