

/***************************** Include Files *******************************/
#include "pmodHB3.h"
u32 PMODHB3_BaseAddress;
/************************** Function Definitions ***************************/

int PMODHB3_initialize(u32 BaseAddr)
{
	PMODHB3_BaseAddress = BaseAddr;
	return PMODHB3_Reg_SelfTest(PMODHB3_BaseAddress);
}

u32 PMODHB3_getTachometer(void)
{
	u32 val;

	val =  PMODHB3_mReadReg(PMODHB3_BaseAddress, PMODHB3_S00_AXI_SLV_REG1_OFFSET);
	return val;
}

u32 PMODHB3_TachometerRPM(void)
{
	u32 val;

	val =  (PMODHB3_getTachometer()/12)* 1000 * 60;
	return val;
}

u32 PMODHB3_getPWM(void)
{
	u32 val;

	val =  PMODHB3_mReadReg(PMODHB3_BaseAddress, PMODHB3_S00_AXI_SLV_REG0_OFFSET);
	return val;
}
void PMODHB3_setPWM(u32 pwmvalue)
{
	u32 current_state, next_state;
    current_state = PMODHB3_getPWM();
    next_state = current_state & DIR_BIT_MASK; // if the msb in current state is 1 next_state will be 0x80000000, if 0 next_state will be 0x00000000,
	next_state = pwmvalue + next_state;//add old direction bit to new pwm value to keep direction the same
	NEXYS4IO_mWriteReg(PMODHB3_BaseAddress, PMODHB3_S00_AXI_SLV_REG0_OFFSET, next_state);
}
void PMODHB3_setDIR(bool direction)
{
	u32 current_state, next_state, old_direction, old_pwm;
    current_state = PMODHB3_getPWM() ;
    old_direction = current_state & DIR_BIT_MASK;
    old_pwm = current_state & PWM_BIT_MASK;
    NEXYS4IO_mWriteReg(PMODHB3_BaseAddress, PMODHB3_S00_AXI_SLV_REG0_OFFSET, old_direction );
    usleep(1000);
    if(direction == FORWARD)
    {
        next_state = DIR_BIT_MASK | old_pwm;
    }
    else
    {
        next_state = old_pwm; 
    }
	
	NEXYS4IO_mWriteReg(PMODHB3_BaseAddress, PMODHB3_S00_AXI_SLV_REG0_OFFSET, next_state);
}
