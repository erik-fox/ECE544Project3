module top();
    logic           clock;
    logic           reset_n;
    logic [31:0]    duty_cycle;
    wire            pwm_out; 

    pwm_generator pwm0(.clock(clock),.reset(reset_n),.duty_cycle(duty_cycle),.pwm_out(pwm_out));
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
    //signal generation
    initial
        begin
            @(posedge reset_n);
            duty_cycle = 0;
            repeat(1000) @(posedge clock);
            duty_cycle = 255;
            repeat(1000) @(posedge clock);
            for(int i = 0; i<100; i++)
                begin
                    duty_cycle = $urandom()%255;
                    repeat($urandom()%500) @(posedge clock);
                end
            $stop;
        end
endmodule