/**
*
* @file Project3_source.c
*
* @author Alex Beaulier (beaulier@pdx.edu) | Partner Erik Fox
* @copyright Portland State University, 2022
*
* This file implements the project 3 program.
* The program runs a PID loop control for a small hobby DC motor.
* Hardware handles control and interface to a PMOD HB3 from Digilent
* Software receives encoder data, calculates duty cycle for PWM generator
* which feeds into the motor.
*
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who  Date     Changes
* ----- ---- -------- -----------------------------------------------
* 1.00a	AJB	13-May-2022		First release of test program. Runs basic function tasks for conversion into RTOS next week
*
* </pre>
*
* @note
* The minimal hardware configuration for this test is a Microblaze-based system with at least 32KB of memory,
* an instance of Nexys4IO, an instance of the pmodOLEDrgb AXI slave peripheral, and instance of the pmodENC AXI
* slave peripheral, an instance of AXI GPIO, an instance of AXI timer and an instance of the AXI UARTLite
* (used for xil_printf() console output)
*
* @note
* The driver code and test application(s) for the pmodOLDrgb and pmodENC are
* based on code provided by Digilent, Inc.
*
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "platform.h"
#include "xparameters.h"
#include "xstatus.h"
#include "microblaze_sleep.h"
#include "nexys4IO.h"
#include "PmodOLEDrgb.h"
#include "PmodENC.h"
#include "xgpio.h"
#include "xintc.h"
#include "xtmrctr.h"

/************************** Constant Definitions ****************************/

// Clock frequencies
#define CPU_CLOCK_FREQ_HZ		XPAR_CPU_CORE_CLOCK_FREQ_HZ
#define AXI_CLOCK_FREQ_HZ		XPAR_CPU_M_AXI_DP_FREQ_HZ

// AXI timer parameters
#define AXI_TIMER_DEVICE_ID		XPAR_AXI_TIMER_0_DEVICE_ID
#define AXI_TIMER_BASEADDR		XPAR_AXI_TIMER_0_BASEADDR
#define AXI_TIMER_HIGHADDR		XPAR_AXI_TIMER_0_HIGHADDR
#define TmrCtrNumber			0


// Definitions for peripheral NEXYS4IO
#define NX4IO_DEVICE_ID		XPAR_NEXYS4IO_0_DEVICE_ID
#define NX4IO_BASEADDR		XPAR_NEXYS4IO_0_S00_AXI_BASEADDR
#define NX4IO_HIGHADDR		XPAR_NEXYS4IO_0_S00_AXI_HIGHADDR

// Definitions for peripheral PMODOLEDRGB
#define RGBDSPLY_DEVICE_ID		XPAR_PMODOLEDRGB_0_DEVICE_ID
#define RGBDSPLY_GPIO_BASEADDR	XPAR_PMODOLEDRGB_0_AXI_LITE_GPIO_BASEADDR
#define RGBDSPLY_GPIO_HIGHADDR	XPAR_PMODOLEDRGB_0_AXI_LITE_GPIO_HIGHADD
#define RGBDSPLY_SPI_BASEADDR	XPAR_PMODOLEDRGB_0_AXI_LITE_SPI_BASEADDR
#define RGBDSPLY_SPI_HIGHADDR	XPAR_PMODOLEDRGB_0_AXI_LITE_SPI_HIGHADDR

// Definition for GPIO_R items
//#define RGB_R_HIGH_ADDR 			XPAR_AXI_GPIO_RED_HIGH_BASEADDR
//#define RGB_R_LOW_ADDR 				XPAR_AXI_GPIO_RED_LOW_BASEADDR
#define GPIO_RH_DEVICE_ID			XPAR_AXI_GPIO_RED_HIGH_DEVICE_ID
#define GPIO_RH_INPUT_0_CHANNEL		1
#define GPIO_RH_OUTPUT_0_CHANNEL	2
#define GPIO_GH_DEVICE_ID			XPAR_AXI_GPIO_GREEN_HIGH_DEVICE_ID
#define GPIO_GH_INPUT_0_CHANNEL		1
#define GPIO_GH_OUTPUT_0_CHANNEL	2
#define GPIO_BH_DEVICE_ID			XPAR_AXI_GPIO_BLUE_HIGH_DEVICE_ID
#define GPIO_BH_INPUT_0_CHANNEL		1
#define GPIO_BH_OUTPUT_0_CHANNEL	2
// GPIO parameters
#define GPIO_0_DEVICE_ID			XPAR_AXI_GPIO_0_DEVICE_ID
#define GPIO_0_INPUT_0_CHANNEL		1
#define GPIO_0_OUTPUT_0_CHANNEL		2

// Definitions for peripheral PMODENC
#define PMODENC_DEVICE_ID		XPAR_PMODENC_0_DEVICE_ID
#define PMODENC_BASEADDR		XPAR_PMODENC_0_AXI_LITE_GPIO_BASEADDR
#define PMODENC_HIGHADDR		XPAR_PMODENC_0_AXI_LITE_GPIO_HIGHADDR


// Fixed Interval timer - 100 MHz input clock, 40KHz output clock
// FIT_COUNT_1MSEC = FIT_CLOCK_FREQ_HZ * .001
#define FIT_IN_CLOCK_FREQ_HZ	CPU_CLOCK_FREQ_HZ
#define FIT_CLOCK_FREQ_HZ		40000
#define FIT_COUNT				(FIT_IN_CLOCK_FREQ_HZ / FIT_CLOCK_FREQ_HZ)
#define FIT_COUNT_1MSEC			40

// Interrupt Controller parameters
#define INTC_DEVICE_ID			XPAR_INTC_0_DEVICE_ID
#define FIT_INTERRUPT_ID		XPAR_MICROBLAZE_0_AXI_INTC_FIT_TIMER_0_INTERRUPT_INTR

/**************************** Type Definitions ******************************/

/***************** Macros (Inline Functions) Definitions ********************/

/************************** Variable Definitions ****************************/
// Microblaze peripheral instances
uint64_t 	timestamp = 0L;
PmodOLEDrgb	pmodOLEDrgb_inst;
PmodENC 	pmodENC_inst;
XGpio		GPIOInst0;					// GPIO instance
XGpio 		GPIOInstRH;
XGpio 		GPIOInstGH;
XGpio 		GPIOInstBH;
XIntc 		IntrptCtlrInst;				// Interrupt Controller instance
XTmrCtr		AXITimerInst;				// PWM timer instance


// The following variables are shared between non-interrupt processing and
// interrupt processing such that they must be global(and declared volatile)
volatile uint32_t	gpio_in;					// GPIO input port
volatile uint32_t	TargetHeading;				//Compass target heading from encoder set
volatile uint32_t	CurrentHeading;				//Compass current heading as read on CMPS(0-359 + wrap)
volatile uint32_t 	UpdateTargetHeading_Flag;	//Flag on update interrupt
volatile uint32_t   HeadingDiff;				//Abs diff of current heading and target explored in blue
volatile int        HeadingChange;				//Check if change is warmer or less
volatile uint32_t 	HeadingDiffPrev;			//Stores previous Heading Diff value

//Hardware reading function inputs
//Seven Seg Display Vals
volatile u32 LED1_Red			= 0;	//Default 0% of 255
volatile u32 LED1_Green  		= 0;	//Default 0%
volatile u32 LED1_Blue			= 0;	//Default 5%

volatile u32 switch_values		= 0;

//OLED function inputs
volatile uint32_t u32_ss_disp_val = 0;
volatile uint16_t RGB_Combo  	= 0;
volatile uint8_t  startcol		= 0;
volatile uint8_t  endcol		= 60;
volatile uint8_t  startrow		= 0;
volatile uint8_t  endrow		= 5;
volatile uint16_t linecolor 	= 63489;
volatile uint8_t  bFill 		= 1; //0 or 1 // Should be filled
volatile uint16_t fillColor 	= 63489; // 255,255,255
volatile uint8_t  OLED_updatelock	= 0;

//ENCODER SETUP
volatile uint32_t state = 0, laststate = 0; //comparing current and previous state to detect edges on GPIO pins.
volatile int ticks = 0, lastticks = 1;

//State Machines Setup
typedef enum {
    KP,
	KI,
    KD,
    Neutral
} Kpid;
volatile Kpid Kpid_current_state = Neutral;

typedef enum {
    One,
	Five,
    Ten,
    Default
} Incr_Status;

volatile Incr_Status Incr_Status_KPID = Default;		//SW 5:4
volatile Incr_Status Incr_Status_ROT_ENC = Default;	//SW 3:2


//PID Variables
volatile u32 Kp			 = 0;
volatile u32 Ki  		 = 0;
volatile u32 Kd			 = 0;
volatile u32 RPM_Target	 = 0;
volatile u32 RPM_Current = 0;
volatile u32 EncoderVal  = 0;

/************************** Function Prototypes *****************************/
void PMDIO_itoa(int32_t value, char *string, int32_t radix);
void PMDIO_puthex(PmodOLEDrgb* InstancePtr, uint32_t num);
void PMDIO_putnum(PmodOLEDrgb* InstancePtr, int32_t num, int32_t radix);
int	 do_init(void);											// initialize system
void FIT_Handler(void);										// fixed interval timer interrupt handler
int AXI_Timer_initialize(void);

//Updaters for RTOS Conversion
void GreenLED_Update();
void GreenLED_Clear();
void ENC_Update();
void ENC_State_Update();
void MotorENC_Update();
void OLED_Initialize();
void OLED_Update();
void OLED_Clear();
void PIDController_Update();
void PshBtn_Update();
void SSEG_Update();
void SSEG_Clear();
void Switch_Update();
void TriColorLED1_Update();
void TriColorLED1_Clear();
/*****************************************************************************/


/************************** MAIN PROGRAM ************************************/
int main(void)
{
    init_platform();

	uint32_t sts;

	sts = do_init();
	if (XST_SUCCESS != sts)
	{
		exit(1);
	}

	microblaze_enable_interrupts();

	xil_printf("ECE 544 Project 3 Test Program \n\r");
	xil_printf("By Alex Beaulier. 13-May-2022\n\n\r");


	// blank the display digits and turn off the decimal points
	SSEG_Clear();
	//Startup for OLED, prepwork before writing begins to eliminate writing these every time.
	OLED_Initialize();
	TriColorLED1_Clear();

	// loop the test until the user presses the encoder button
	laststate = ENC_getState(&pmodENC_inst);

	while (1)//Main Loop Start
	{
		//Update PMODENC state
		ENC_Update();
		if (ENC_buttonPressed(state) && !ENC_buttonPressed(laststate))//only check on button posedge
		{
			break;
		}
		//*Haven't figured out a good method for implementing the break statement in func,
		//maybe RTOS works better^^^?

		//Update Switches
		Switch_Update();

		//Update Push Button
		PshBtn_Update();


		/*HARDWARE IMPLEMENTATION Precursor
		//Read Encoder Stack
		//Calculate RPMs eventually
			//MotorENC_Update();
			//PIDController_Update();
		 */

		//Update PmodOLEDrgb
		OLED_Update();

		//Update Green LED
		GreenLED_Update();

		//Update last state elements
		ENC_State_Update();

		//Update 7-seg display
		// 5678 target RPM and 1234 digits for current RPM
		SSEG_Update();
	}//EO Main Loop

	//Clearing off all 7seg display digits & decimals, OLED
	OLEDrgb_Clear(&pmodOLEDrgb_inst);
	SSEG_Clear();
	TriColorLED1_Clear();
	GreenLED_Clear;

}



/**************************** HELPER FUNCTIONS ******************************/

/****************************************************************************/
/**
* initialize the system
*
* This function is executed once at start-up and after resets.  It initializes
* the peripherals and registers the interrupt handler(s)
*****************************************************************************/

int	 do_init(void)
{
	uint32_t status;				// status from Xilinx Lib calls

	// initialize the Nexys4 driver and (some of)the devices
	status = (uint32_t) NX4IO_initialize(NX4IO_BASEADDR);
	if (status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	// set all of the display digits to blanks and turn off
	// the decimal points using the "raw" set functions.
	// These registers are formatted according to the spec
	// and should remain unchanged when written to Nexys4IO...
	// something else to check w/ the debugger when we bring the
	// drivers up for the first time
	NX4IO_SSEG_setSSEG_DATA(SSEGHI, 0x0058E30E);
	NX4IO_SSEG_setSSEG_DATA(SSEGLO, 0x00144116);

	// initialize the GPIO instances
	status = XGpio_Initialize(&GPIOInst0, GPIO_0_DEVICE_ID);
	if (status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}
	// GPIO0 channel 1 is an 8-bit input port.
	// GPIO0 channel 2 is an 8-bit output port.
	XGpio_SetDataDirection(&GPIOInst0, GPIO_0_INPUT_0_CHANNEL, 0xFF);
	XGpio_SetDataDirection(&GPIOInst0, GPIO_0_OUTPUT_0_CHANNEL, 0x00);

	// initialize the GPIO instances
	status = XGpio_Initialize(&GPIOInstRH, GPIO_RH_DEVICE_ID);
	if (status != XST_SUCCESS)
	{
	   return XST_FAILURE;
	}
	status = XGpio_Initialize(&GPIOInstGH, GPIO_GH_DEVICE_ID);
	if (status != XST_SUCCESS)
	{
	   return XST_FAILURE;
	}
	status = XGpio_Initialize(&GPIOInstBH, GPIO_BH_DEVICE_ID);
	if (status != XST_SUCCESS)
	{
	   return XST_FAILURE;
	}

	// GPIO channel 1 is an 32-bit input port.
	// GPIO channel 2 is an 32-bit input port.
	XGpio_SetDataDirection(&GPIOInstRH, GPIO_RH_INPUT_0_CHANNEL, 0xFFFFFFFF);
	XGpio_SetDataDirection(&GPIOInstRH, GPIO_RH_OUTPUT_0_CHANNEL, 0xFFFFFFFF);
	XGpio_SetDataDirection(&GPIOInstGH, GPIO_GH_INPUT_0_CHANNEL, 0xFFFFFFFF);
	XGpio_SetDataDirection(&GPIOInstGH, GPIO_GH_OUTPUT_0_CHANNEL, 0xFFFFFFFF);
	XGpio_SetDataDirection(&GPIOInstBH, GPIO_BH_INPUT_0_CHANNEL, 0xFFFFFFFF);
	XGpio_SetDataDirection(&GPIOInstBH, GPIO_BH_OUTPUT_0_CHANNEL, 0xFFFFFFFF);


	OLEDrgb_begin(&pmodOLEDrgb_inst, RGBDSPLY_GPIO_BASEADDR, RGBDSPLY_SPI_BASEADDR);

	// initialize the pmodENC and hardware
	ENC_begin(&pmodENC_inst, PMODENC_BASEADDR);

	status = AXI_Timer_initialize();
	if (status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	// initialize the interrupt controller
	status = XIntc_Initialize(&IntrptCtlrInst, INTC_DEVICE_ID);
	if (status != XST_SUCCESS)
	{
	   return XST_FAILURE;
	}

	// connect the fixed interval timer (FIT) handler to the interrupt
	status = XIntc_Connect(&IntrptCtlrInst, FIT_INTERRUPT_ID,
						   (XInterruptHandler)FIT_Handler,
						   (void *)0);
	if (status != XST_SUCCESS)
	{
		return XST_FAILURE;

	}

	// start the interrupt controller such that interrupts are enabled for
	// all devices that cause interrupts.
	status = XIntc_Start(&IntrptCtlrInst, XIN_REAL_MODE);
	if (status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	// enable the FIT interrupt
	XIntc_Enable(&IntrptCtlrInst, FIT_INTERRUPT_ID);
	return XST_SUCCESS;
}
/*
 * AXI timer initializes it to generate out a 4Khz signal, Which is given to the Nexys4IO module as clock input.
 * DO NOT MODIFY
 */
int AXI_Timer_initialize(void){

	uint32_t status;				// status from Xilinx Lib calls
	uint32_t		ctlsts;		// control/status register or mask

	status = XTmrCtr_Initialize(&AXITimerInst,AXI_TIMER_DEVICE_ID);
		if (status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	status = XTmrCtr_SelfTest(&AXITimerInst, TmrCtrNumber);
		if (status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	ctlsts = XTC_CSR_AUTO_RELOAD_MASK | XTC_CSR_EXT_GENERATE_MASK | XTC_CSR_LOAD_MASK |XTC_CSR_DOWN_COUNT_MASK ;
	XTmrCtr_SetControlStatusReg(AXI_TIMER_BASEADDR, TmrCtrNumber,ctlsts);

	//Set the value that is loaded into the timer counter and cause it to be loaded into the timer counter
	XTmrCtr_SetLoadReg(AXI_TIMER_BASEADDR, TmrCtrNumber, 24998);
	XTmrCtr_LoadTimerCounterReg(AXI_TIMER_BASEADDR, TmrCtrNumber);
	ctlsts = XTmrCtr_GetControlStatusReg(AXI_TIMER_BASEADDR, TmrCtrNumber);
	ctlsts &= (~XTC_CSR_LOAD_MASK);
	XTmrCtr_SetControlStatusReg(AXI_TIMER_BASEADDR, TmrCtrNumber, ctlsts);

	ctlsts = XTmrCtr_GetControlStatusReg(AXI_TIMER_BASEADDR, TmrCtrNumber);
	ctlsts |= XTC_CSR_ENABLE_TMR_MASK;
	XTmrCtr_SetControlStatusReg(AXI_TIMER_BASEADDR, TmrCtrNumber, ctlsts);

	XTmrCtr_Enable(AXI_TIMER_BASEADDR, TmrCtrNumber);
	return XST_SUCCESS;

}

/*********************** DISPLAY-RELATED FUNCTIONS ***********************************/

/****************************************************************************/
/**
* Converts an integer to ASCII characters
*
* algorithm borrowed from ReactOS system libraries
*
* Converts an integer to ASCII in the specified base.  Assumes string[] is
* long enough to hold the result plus the terminating null
*
* @param 	value is the integer to convert
* @param 	*string is a pointer to a buffer large enough to hold the converted number plus
*  			the terminating null
* @param	radix is the base to use in conversion,
*
* @return  *NONE*
*
* @note
* No size check is done on the return string size.  Make sure you leave room
* for the full string plus the terminating null in string
*****************************************************************************/
void PMDIO_itoa(int32_t value, char *string, int32_t radix)
{
	char tmp[33];
	char *tp = tmp;
	int32_t i;
	uint32_t v;
	int32_t  sign;
	char *sp;

	if (radix > 36 || radix <= 1)
	{
		return;
	}

	sign = ((10 == radix) && (value < 0));
	if (sign)
	{
		v = -value;
	}
	else
	{
		v = (uint32_t) value;
	}

  	while (v || tp == tmp)
  	{
		i = v % radix;
		v = v / radix;
		if (i < 10)
		{
			*tp++ = i+'0';
		}
		else
		{
			*tp++ = i + 'a' - 10;
		}
	}
	sp = string;

	if (sign)
		*sp++ = '-';

	while (tp > tmp)
		*sp++ = *--tp;
	*sp = 0;

  	return;
}


/****************************************************************************/
/**
* Write a 32-bit unsigned hex number to PmodOLEDrgb in Hex
*
* Writes  32-bit unsigned number to the pmodOLEDrgb display starting at the current
* cursor position.
*
* @param num is the number to display as a hex value
*
* @return  *NONE*
*
* @note
* No size checking is done to make sure the string will fit into a single line,
* or the entire display, for that matter.  Watch your string sizes.
*****************************************************************************/
void PMDIO_puthex(PmodOLEDrgb* InstancePtr, uint32_t num)
{
  char  buf[9];
  int32_t   cnt;
  char  *ptr;
  int32_t  digit;

  ptr = buf;
  for (cnt = 7; cnt >= 0; cnt--) {
    digit = (num >> (cnt * 4)) & 0xF;

    if (digit <= 9)
	{
      *ptr++ = (char) ('0' + digit);
	}
    else
	{
      *ptr++ = (char) ('a' - 10 + digit);
	}
  }

  *ptr = (char) 0;
  OLEDrgb_PutString(InstancePtr,buf);

  return;
}


/****************************************************************************/
/**
* Write a 32-bit number in Radix "radix" to LCD display
*
* Writes a 32-bit number to the LCD display starting at the current
* cursor position. "radix" is the base to output the number in.
*
* @param num is the number to display
*
* @param radix is the radix to display number in
*
* @return *NONE*
*
* @note
* No size checking is done to make sure the string will fit into a single line,
* or the entire display, for that matter.  Watch your string sizes.
*****************************************************************************/
void PMDIO_putnum(PmodOLEDrgb* InstancePtr, int32_t num, int32_t radix)
{
  char  buf[16];

  PMDIO_itoa(num, buf, radix);
  OLEDrgb_PutString(InstancePtr,buf);

  return;
}


/**************************** INTERRUPT HANDLERS ******************************/
/****************************************************************************/
/**
* Fixed interval timer interrupt handler
*
* Reads the GPIO port which reads back the hardware generated PWM wave for the RGB Leds
*
 *****************************************************************************/

void FIT_Handler(void)
{
	// Read the GPIO port to read back the generated PWM signal for RGB led's
	gpio_in = XGpio_DiscreteRead(&GPIOInst0, GPIO_0_INPUT_0_CHANNEL);

}


/**************************** Task Functions ******************************/
/****************************************************************************/
/**
* Various precursors to RTOS Development
*
* @note
* ECE
 *****************************************************************************/
void OLED_Initialize(){
	RGB_Combo  = OLEDrgb_BuildHSV(LED1_Red,LED1_Green,LED1_Blue);
	OLEDrgb_Clear(&pmodOLEDrgb_inst);
	OLEDrgb_SetFontColor(&pmodOLEDrgb_inst,63489);
	OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 0, 1);
	OLEDrgb_PutString(&pmodOLEDrgb_inst,"RpmCur");
	OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 0, 2);
	OLEDrgb_PutString(&pmodOLEDrgb_inst,"RpmTar");
	OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 0, 3);
	OLEDrgb_PutString(&pmodOLEDrgb_inst,"Kp");
	OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 0, 4);
	OLEDrgb_PutString(&pmodOLEDrgb_inst,"Ki");
	OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 0, 5);
	OLEDrgb_PutString(&pmodOLEDrgb_inst,"Kd");
	OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 0, 6);
	OLEDrgb_PutString(&pmodOLEDrgb_inst,"Select:");
	OLEDrgb_DrawRectangle(&pmodOLEDrgb_inst, startcol, startrow, endcol, endrow, linecolor, fillColor, true);
}

void PIDController_Update(){
	//TODO implement the PID loop here
}

void GreenLED_Update(){
	//TODO Modify the switch values with the watchdog property
	//switch_values = watchdoginterruptval << 15

	//2 = Kp
	if(Kp != 0){
		switch_values |= 1 << 2;
	}
	//1 = Ki
	if(Ki != 0){
		switch_values |= 1 << 1;
	}
	//0 = Kd
	if(Kd != 0){
		switch_values |= 1 << 0;
	}

	//Update all LEDs together
	NX4IO_setLEDs(switch_values);
}

void GreenLED_Clear(){
	NX4IO_setLEDs(0);
}

void SSEG_Update(){
	u32_ss_disp_val = (RPM_Current  * 100000) + RPM_Target; //simple answer...
	NX4IO_SSEG_putU32Dec(u32_ss_disp_val,0);
}

void SSEG_Clear(){
	NX410_SSEG_setAllDigits(SSEGLO, CC_BLANK, CC_BLANK, CC_BLANK, CC_BLANK, DP_NONE);
	NX410_SSEG_setAllDigits(SSEGHI, CC_BLANK, CC_BLANK, CC_BLANK, CC_BLANK, DP_NONE);
}

void PshBtn_Update(){
	//Determine Button
	if (NX4IO_isPressed(BTNU))
	{
		OLED_updatelock = 1;
		switch(Kpid_current_state){
			case KP:
				if(Incr_Status_KPID == One){
					Kp += 1;
				}
				if(Incr_Status_KPID == Five){
					Kp += 5;
				}
				if(Incr_Status_KPID == Ten){
					Kp += 10;
				}
			break;
			case KI:
				if(Incr_Status_KPID == One){
					Ki += 1;
				}
				if(Incr_Status_KPID == Five){
					Ki += 5;
				}
				if(Incr_Status_KPID == Ten){
					Ki += 10;
				}
			break;
			case KD:
				if(Incr_Status_KPID == One){
					Kd += 1;
				}
				if(Incr_Status_KPID == Five){
					Kd += 5;
				}
				if(Incr_Status_KPID == Ten){
					Kd += 10;
				}
			break;
		}
	}
	if (NX4IO_isPressed(BTND))
	{
		OLED_updatelock = 1;
		switch(Kpid_current_state){
			case KP:
				if(Incr_Status_KPID == One){
					Kp = (Kp >= 1) ? Kp - 1 :  Kp;
				}
				if(Incr_Status_KPID == Five){
					Kp = (Kp >= 5) ? Kp - 5 :  Kp;
				}
				if(Incr_Status_KPID == Ten){
					Kp = (Kp >= 10) ? Kp - 10 : Kp;
				}
			break;
			case KI:
				if(Incr_Status_KPID == One){
					Ki = (Ki >= 1) ? Ki - 1 :  Ki;
				}
				if(Incr_Status_KPID == Five){
					Ki = (Ki >= 5) ? Ki - 5 :  Ki;
				}
				if(Incr_Status_KPID == Ten){
					Ki = (Ki >= 10) ? Ki - 10 : Ki;
				}
			break;
			case KD:
				if(Incr_Status_KPID == One){
					Kd = (Kd >= 1) ? Kd - 1 : Kd;
				}
				if(Incr_Status_KPID == Five){
					Kd = (Kd >= 5) ? Kd - 5 : Kd;
				}
				if(Incr_Status_KPID == Ten){
					Kd = (Kd >= 10) ? Kd - 10 : Kd;
				}
			break;
		}
	}

	if (NX4IO_isPressed(BTNL)){

	}
	if (NX4IO_isPressed(BTNR)){

	}
	if (NX4IO_isPressed(BTNC)){
		OLED_updatelock = 4;
		//Motor speed to 0
		//TODO Turn off PWM sig to motor
		//KPID constants to non zero val to guarantee effect
		RPM_Target = 0;
		Kp = 1;
		Ki = 1;
		Kd = 1;
		OLED_updatelock = 3;
	}
}

void ENC_Update(){
	state = ENC_getState(&pmodENC_inst);
	//Update the Encoder value, wrap if necessary
	ticks += (ENC_getRotation(state, laststate));
	if(ticks > lastticks){//CW Turn, Increment
		OLED_updatelock = 2;
		switch(Incr_Status_ROT_ENC){
			case One:
				RPM_Target += 1;
			break;
			case Five:
				RPM_Target += 5;
			break;
			case Ten:
				RPM_Target += 10;
			break;
			case Default:
				RPM_Target += 0;
			break;
		}
	}
	if(ticks < lastticks){//CCW Turn, decrement
		OLED_updatelock = 2;
		switch(Incr_Status_ROT_ENC){
			case One:
				RPM_Target = (RPM_Target > 0) ? RPM_Target - 1 : RPM_Target - 0;
			break;
			case Five:
				RPM_Target = (RPM_Target > 5) ? RPM_Target - 5 : RPM_Target - 0;
			break;
			case Ten:
				RPM_Target = (RPM_Target > 10) ? RPM_Target - 10 : RPM_Target - 0;
			break;
			case Default:
				RPM_Target -= 0;
			break;
		}
	}
}

void ENC_State_Update(){
	laststate = state;
	lastticks = ticks;
}

void OLED_Update(){
	if (OLED_updatelock == 2 | OLED_updatelock == 4) {
		RGB_Combo  = OLEDrgb_BuildRGB(LED1_Red,LED1_Green,LED1_Blue);
		fillColor = RGB_Combo;
		linecolor = RGB_Combo;
		OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 7, 1);
		OLEDrgb_PutString(&pmodOLEDrgb_inst,"    ");
		OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 7, 1);
		PMDIO_putnum(&pmodOLEDrgb_inst,RPM_Current,10);
		OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 7, 2);
		OLEDrgb_PutString(&pmodOLEDrgb_inst,"    ");
		OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 7, 2);
		PMDIO_putnum(&pmodOLEDrgb_inst,RPM_Target,10);
	}
	if(OLED_updatelock == 1 | OLED_updatelock == 4){
		if (Kpid_current_state == KP){
			OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 4, 3);
			OLEDrgb_PutString(&pmodOLEDrgb_inst,"    ");
			OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 4, 3);
			PMDIO_putnum(&pmodOLEDrgb_inst,Kp,5);
		}else if(Kpid_current_state == KI){
			OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 4, 4);
			OLEDrgb_PutString(&pmodOLEDrgb_inst,"    ");
			OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 4, 4);
			PMDIO_putnum(&pmodOLEDrgb_inst,Ki,5);
		}else if(Kpid_current_state == KD){
			OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 4, 5);
			OLEDrgb_PutString(&pmodOLEDrgb_inst,"    ");
			OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 4, 5);
			PMDIO_putnum(&pmodOLEDrgb_inst,Kd,5);
		}
	}
	if(OLED_updatelock == 5){
		if (Kpid_current_state == KP){
		OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 7, 6);
		OLEDrgb_PutString(&pmodOLEDrgb_inst,"Kp");
		}else if(Kpid_current_state == KI){
			OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 7, 6);
			OLEDrgb_PutString(&pmodOLEDrgb_inst,"Ki");
		}else if(Kpid_current_state == KD){
			OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 7, 6);
			OLEDrgb_PutString(&pmodOLEDrgb_inst,"Kd");
		}
	}
	OLED_updatelock = 0;
}

void Switch_Update(){
	u_int32_t mask1, mask2;

	switch_values = NX4IO_getSwitches();

	//SW 15 Watchdog
	mask1 = 1 << (16 - 1);
	if((switch_values & mask1) == mask1){
		//Crash the system here...TODO
	}

	//SW 5:4 KPID INC Vals
	//00 +1, 01 +5, 1x +10
	mask1 = 1 << (6 - 1);
	mask2 = 1 << (5 - 1);
	if((switch_values & mask1) == mask1){
		//Inc 10
		Incr_Status_KPID = Ten;
	}else if((switch_values & mask2) == mask2){
		//Inc 5
		Incr_Status_KPID = Five;
	}else{
		//Inc 1
		Incr_Status_KPID = One;
	}

	//SW 3:2 KPID Chooser
	//00 Kp, 01 Ki, 1x Kd
	mask1 = 1 << (4 - 1);
	mask2 = 1 << (3 - 1);
	if((switch_values & mask1) == mask1){
		//Inc Kd
		if(Kpid_current_state != KD){
			OLED_updatelock = 5;
			Kpid_current_state = KD;
		}
	}else if((switch_values & mask2) == mask2){
		//Inc Ki
		if(Kpid_current_state != KI){
			OLED_updatelock = 5;
			Kpid_current_state = KI;
		}
	}else{
		//Inc Kp
		if(Kpid_current_state != KP){
			OLED_updatelock = 5;
			Kpid_current_state = KP;
		}
	}

	//SW 1:0 ROT ENC  INC Vals
	//00 +1, 01 +5, 1x +10
	mask1 = 1 << (2 - 1);
	mask2 = 1 << (1 - 1);
	if((switch_values & mask1) == mask1){
		//Inc 10
		Incr_Status_ROT_ENC = Ten;
	}else if((switch_values & mask2) == mask2){
		//Inc 5
		Incr_Status_ROT_ENC = Five;
	}else{
		//Inc 1
		Incr_Status_ROT_ENC = One;
	}
}

void OLED_Clear(){

}

void MotorENC_Update(){

}

void TriColorLED1_Update(){
	NX4IO_RGBLED_setChnlEn(RGB1, true, true, true);
	NX4IO_RGBLED_setDutyCycle(RGB1, Kp, Ki, Kd);
}

void TriColorLED1_Clear(){
	NX4IO_RGBLED_setChnlEn(RGB1, true, true, true);
	NX4IO_RGBLED_setDutyCycle(RGB1, 0, 0, 0);
}
