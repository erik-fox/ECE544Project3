
#ifndef PMODHB3_H
#define PMODHB3_H


/****************** Include Files ********************/
#include "xil_types.h"
#include "xil_io.h"
#include "xstatus.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "microblaze_sleep.h"
#include "stdbool.h"



#define PMODHB3_S00_AXI_SLV_REG0_OFFSET 0
#define PMODHB3_S00_AXI_SLV_REG1_OFFSET 4
#define PMODHB3_S00_AXI_SLV_REG2_OFFSET 8
#define PMODHB3_S00_AXI_SLV_REG3_OFFSET 12
#define DIR_BIT_MASK 0x80000000
#define PWM_BIT_MASK 0x7FFFFFFF
#define FORWARD 1
#define BACKWARD 0


/**************************** Type Definitions *****************************/
/**
 *
 * Write a value to a PMODHB3 register. A 32 bit write is performed.
 * If the component is implemented in a smaller width, only the least
 * significant data is written.
 *
 * @param   BaseAddress is the base address of the PMODHB3device.
 * @param   RegOffset is the register offset from the base to write to.
 * @param   Data is the data written to the register.
 *
 * @return  None.
 *
 * @note
 * C-style signature:
 * 	void PMODHB3_mWriteReg(u32 BaseAddress, unsigned RegOffset, u32 Data)
 *
 */
#define PMODHB3_mWriteReg(BaseAddress, RegOffset, Data) \
  	Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))

/**
 *
 * Read a value from a PMODHB3 register. A 32 bit read is performed.
 * If the component is implemented in a smaller width, only the least
 * significant data is read from the register. The most significant data
 * will be read as 0.
 *
 * @param   BaseAddress is the base address of the PMODHB3 device.
 * @param   RegOffset is the register offset from the base to write to.
 *
 * @return  Data is the data from the register.
 *
 * @note
 * C-style signature:
 * 	u32 PMODHB3_mReadReg(u32 BaseAddress, unsigned RegOffset)
 *
 */
#define PMODHB3_mReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))

/************************** Function Prototypes ****************************/
/**
 *
 * Run a self-test on the driver/device. Note this may be a destructive test if
 * resets of the device are performed.
 *
 * If the hardware system is not built correctly, this function may never
 * return to the caller.
 *
 * @param   baseaddr_p is the base address of the PMODHB3 instance to be worked on.
 *
 * @return
 *
 *    - XST_SUCCESS   if all self-test code passed
 *    - XST_FAILURE   if any self-test code failed
 *
 * @note    Caching must be turned off for this function to work.
 * @note    Self test may fail if data memory and device are not on the same bus.
 *
 */
XStatus PMODHB3_Reg_SelfTest(u32 baseaddr_p);

int PMODHB3_initialize(u32 BaseAddr);
u32 PMODHB3_getTachometer(void);
u32 PMODHB3_TachometerRPM(void);
u32 PMODHB3_getPWM(void);
void PMODHB3_setPWM(u32 pwmvalue);
void PMODHB3_setDIR(bool direction);

#endif // PMODHB3_H
