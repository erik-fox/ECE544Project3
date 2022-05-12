`timescale 1ns / 1ps //100MHz clock
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 11/19/2021 02:45:56 PM
// Design Name: 
// Module Name: Testbench_pwm_generator
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////

module Testbench_pwm_generator();

reg [31:0]      angle_requested = 20'd00100000;
reg 		    reset;
reg 		    clk;
wire	        PWMPIN_o;

//Note to change sim run time in the settings file, overrides stop
//Also note delays here are double of real time due to clk changing every other cycle
initial begin
    clk <= 0;
    reset <= 1;
    #5 reset <= 0;
    #20000000 angle_requested <= angle_requested + 20'd00010000;//1.1ms
    #20000000 angle_requested <= angle_requested + 20'd00010000;//1.2ms
    #20000000 angle_requested <= angle_requested + 20'd00010000;//1.3ms
    #20000000 angle_requested <= angle_requested + 20'd00010000;//1.4ms
    #20000000 angle_requested <= angle_requested + 20'd00010000;//1.5ms
    #20000000 angle_requested <= angle_requested + 20'd00010000;//1.6ms
    #20000000 angle_requested <= angle_requested + 20'd00010000;//1.7ms
    #20000000 angle_requested <= angle_requested + 20'd00010000;//1.8ms
    #20000000 angle_requested <= angle_requested + 20'd00010000;//1.9ms
    #20000000 angle_requested <= angle_requested + 20'd00010000;//2.0ms
    #100000000 $stop;     //100ms 

end

//Generates 100 MHZ clock
always begin
#5 clk =! clk;
end

pwm_generator_top DUT_pwm(
.angle_requested(angle_requested),
.reset(reset),
.clk(clk),
.PWMPIN_o(PWMPIN_o)
);

endmodule
