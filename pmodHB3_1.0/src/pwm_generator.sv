//* pwm_generator.sv 
//* Creates a pwm signal to an output pin based on a desired duty cycle 
//**************************************
module pwm_generator(
    input  logic 		    clock,
    input  logic 		    reset,
    input  logic    [31:0]  duty_cycle,	
    output logic	        pwm_out	
);
    parameter       MAX_COUNT = 255;

    logic   [31:0]  counter;
	

	always_ff@(posedge clock) 
        begin
            if ( !reset  ) 
                begin	 
			        counter <= '0;
                    pwm_out <= '0;
		        end
		    else 
                begin
                    if(duty_cycle == MAX_COUNT)
                        begin
                            pwm_out <= '1;
                        end
                    else if(duty_cycle == '0 || counter >= duty_cycle)
                        begin
                            pwm_out <= '0;
                        end
                    else
                        begin
                            pwm_out <= '1;
                        end
                    if( counter == MAX_COUNT)
                        begin
                            counter <= '0;
                        end
                    else
                        begin
                            counter <= counter + 1'b1;
                        end
                    
		        end
	    end
endmodule