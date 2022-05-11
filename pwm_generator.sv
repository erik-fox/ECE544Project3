//* pwm_generator.sv 
//* Creates a pwm signal to an output pin based on a desired angle
//* Also uses special servo commands for extra ability with servo control
//* Requires registers at higher level for modification 
//**************************************
module pwm_generator_top(
	input  wire [31:0]  angle_requested,	//Used wire instead of logic due to issues on VGA lab
    input  wire 		 reset,
    input  wire 		 clk,
    output logic	     PWMPIN_o	
);
    logic [31:0] counted_value;
    logic [31:0] Duty_Cycle;
	
    // Convert the angle value to PWM Duty Cycle. 
    //100MHZ clock, one clock cycle is 10ns
	//[90 degree ranges, 2 to 1.66ms, 1.66 to 1.33ms, 1.33 to 1ms] [Left, Front, Right]1
	//angle_requested(Duty_Cycle_) = 1ms + (1ms partition(20'd100000) / 180 degree) => Partitions/ 1 Degree => 5555 per degree(floor rounded down)
	//Simplifying: angle_requested = 20'd1000000 + (5555*degree)
	//Calculating in C rather than verilog due to floating point of angle
	//Duty Cycle Reg written in address register of servo yaw(LeftRight) and pitch(updown)
    always_comb begin
        Duty_Cycle = angle_requested;
    end
	
	
	//Duty Cycle drive output high if the counter is within the range, else drive low 
	always_comb begin
		if (counted_value < Duty_Cycle) begin
			PWMPIN_o = 1'b1;
		end
		else begin
			PWMPIN_o = 1'b0;
		end
	end
	
	//Updates position every 20 ms
	//Calculation for Servos
	//Time Period of Signal is [(CLK:100MHz)/(1/(Refresh:20ms * 10^-3) = count required]
	//20000000ns required for 20ms
	//Clock ticks(counted) will be 20,000,000 / 10ns period = 2,000,000 count ticks
	always_ff@(posedge clk) begin
      if (reset == 1'b1 || counted_value == 31'd2000000) begin	//Period 20ms, clock  
			counted_value <= 31'b0;
		end
		else begin
			counted_value <= counted_value + 1'b1;
		end
	end
endmodule