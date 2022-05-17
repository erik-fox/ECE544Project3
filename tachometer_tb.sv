module top();
    logic clock;
    logic reset_n;
    logic encoder_data;
    wire  [31:0] data_out;

    tachometer t0(.clock(clock),.system_reset(reset_n),.encoder_data(encoder_data),.data_out(data_out));

    //clock generator
    initial
        begin
            $dumpfile("dump.vcd"); $dumpvars;
            clock = 0;
            forever #10 clock = ~clock;
        end   
    // 10 clock reset
    initial
        begin
            reset_n = 0;
            repeat (10) @ (posedge clock)
            reset_n = 1;
        end
    initial
        begin
            encoder_data = '0;
            @(posedge reset_n);
            for(int i = 0; i< 1000; i++)
                begin
                    repeat(20)@(posedge clock);
                    encoder_data = ~encoder_data;
                end
            $stop;
        end
endmodule