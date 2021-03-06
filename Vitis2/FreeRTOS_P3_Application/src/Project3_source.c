/**
*
* @file Project3_source.c
*
* @authors Alex Beaulier (beaulier@pdx.edu) and Erik Fox
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
#include "PmodENC544.h"
#include "pmodHB3.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xintc.h"
#include "xtmrctr.h"
#include "xwdttb.h"
#include "xil_exception.h"

#include "GPIOfunctions.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"


/************************** Constant Definitions ****************************/

// Clock frequencies
#define CPU_CLOCK_FREQ_HZ		XPAR_CPU_CORE_CLOCK_FREQ_HZ
#define AXI_CLOCK_FREQ_HZ		XPAR_CPU_M_AXI_DP_FREQ_HZ

// AXI timer parameters
#define AXI_TIMER_DEVICE_ID		XPAR_AXI_TIMER_0_DEVICE_ID
#define AXI_TIMER_BASEADDR		XPAR_AXI_TIMER_0_BASEADDR
#define AXI_TIMER_HIGHADDR		XPAR_AXI_TIMER_0_HIGHADDR
#define TmrCtrNumber			0

// Definitions for peripheral NEXYS4IO - SSEG DISP
#define NX4IO_DEVICE_ID		XPAR_NEXYS4IO_0_DEVICE_ID
#define NX4IO_BASEADDR		XPAR_NEXYS4IO_0_S00_AXI_BASEADDR
#define NX4IO_HIGHADDR		XPAR_NEXYS4IO_0_S00_AXI_HIGHADDR

// PMOD PMODOLEDRGB
#define RGBDSPLY_DEVICE_ID		XPAR_PMODOLEDRGB_0_DEVICE_ID
#define RGBDSPLY_GPIO_BASEADDR	XPAR_PMODOLEDRGB_0_AXI_LITE_GPIO_BASEADDR
#define RGBDSPLY_GPIO_HIGHADDR	XPAR_PMODOLEDRGB_0_AXI_LITE_GPIO_HIGHADDR
#define RGBDSPLY_SPI_BASEADDR	XPAR_PMODOLEDRGB_0_AXI_LITE_SPI_BASEADDR
#define RGBDSPLY_SPI_HIGHADDR	XPAR_PMODOLEDRGB_0_AXI_LITE_SPI_HIGHADDR

// Green LEDs
#define GPIO_0_DEVICE_ID			XPAR_AXI_GPIO_0_DEVICE_ID
#define GPIO_0_Base_GLEDS			XPAR_AXI_GPIO_0_BASEADDR
#define GPIO_0_INPUT_0_CHANNEL		1
#define GPIO_0_OUTPUT_0_CHANNEL		2

// Pushbutton Switches
#define GPIO_1_PBSWITCH				XPAR_AXI_GPIO_1_BASEADDR
#define GPIO_1_PBSWITCH_HIGHADDR	XPAR_AXI_GPIO_1_HIGHADDR
#define GPIO_1_PBSWITCH_DEVICE_ID	XPAR_AXI_GPIO_1_DEVICE_ID
#define GPIO_1_PBSWITCH_INTR_PRESENT XPAR_AXI_GPIO_1_INTERRUPT_PRESENT
#define GPIO_1_PBSWITCH_DUAL		XPAR_AXI_GPIO_1_IS_DUAL
#define GPIO_1_Channel_1			1	//Pushbuttons
#define GPIO_1_Channel_2			2	//Switches

// PMODENC Definitions
#define PMODENC_DEVICE_ID		XPAR_PMODENC544_0_DEVICE_ID
#define PMODENC_BASEADDR		XPAR_PMODENC544_0_S00_AXI_BASEADDR
#define PMODENC_HIGHADDR		XPAR_PMODENC544_0_S00_AXI_HIGHADDR

// HB3 Definitions
#define PMODHB3_DEVICE_ID		XPAR_PMODHB3_0_DEVICE_ID
#define PMODHB3_BASEADDR		XPAR_PMODHB3_0_S00_AXI_BASEADDR
#define PMODHB3_HIGHADDR		XPAR_PMODHB3_0_S00_AXI_HIGHADDR

// Fixed Interval timer - 100 MHz input clock, 40KHz output clock
// FIT_COUNT_1MSEC = FIT_CLOCK_FREQ_HZ * .001
#define FIT_IN_CLOCK_FREQ_HZ	CPU_CLOCK_FREQ_HZ
#define FIT_CLOCK_FREQ_HZ		40000
#define FIT_COUNT				(FIT_IN_CLOCK_FREQ_HZ / FIT_CLOCK_FREQ_HZ)
#define FIT_COUNT_1MSEC			40

// Interrupt Controller parameters
//Interrupt 2 is switches and buttons
//Interrupt GPIO SW and Buttons
/*
#define XPAR_INTC_SINGLE_BASEADDR
#define XPAR_INTC_SINGLE_HIGHADDR
*/

//Interrupt 1 is caused by timer
/*
#define XPAR_INTC_0_DEVICE_ID
#define XPAR_INTC_0_BASEADDR
#define XPAR_INTC_0_HIGHADDR
*/

//WD timer is interrupt 0
//#define FIT_INTERRUPT_ID		XPAR_MICROBLAZE_0_AXI_INTC_FIT_TIMER_0_INTERRUPT_INTR

//	Watchdog
#define WDTTB_DEVICE_ID		XPAR_WDTTB_0_DEVICE_ID
#define WDTTB_INTERRUPT_ID  XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMEBASE_WDT_0_WDT_INTERRUPT_INTR
#define INTC_DEVICE_ID		XPAR_INTC_0_DEVICE_ID
/*
#define WDTTB_IRPT_INTR		XPAR_INTC_0_WDTTB_0_WDT_INTERRUPT_VEC_ID
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID
#define WDTTB_IRPT_INTR		XPAR_FABRIC_WDTTB_0_VEC_ID
*/

// FreeRTOS
#define mainQUEUE_LENGTH					( 1 )

#define mainDONT_BLOCK						( portTickType ) 0

//Declare a Semaphore
xSemaphoreHandle binary_sem;


/**************************** Type Definitions ******************************/

/***************** Macros (Inline Functions) Definitions ********************/

/************************** Variable Definitions ****************************/
// Microblaze peripheral instances PMODS
uint64_t 	timestamp = 0L;
PmodOLEDrgb	pmodOLEDrgb_inst;
//PmodENC 	pmodENC_inst;

//GPIO
XGpio		GPIOInst0;					// GPIO instance	//Green LEDs
XGpio		GPIOButton;					// GPIO Buttons and Switches

//Interrupts and Timers
XIntc 		IntrptCtlrInst;				// Interrupt Controller instance
XTmrCtr		AXITimerInst;				// PWM timer instance
XWdtTb		XWdtTbInstance;				/* Instance of Time Base WatchDog Timer */

//Declare a Semaphore for flagging interrupt/PID-Visuals
xSemaphoreHandle binary_sem;

/* The queue used by the queue send and queue receive tasks. */
static xQueueHandle xQueue_PID_Update = NULL;
static xQueueHandle xQueue_Display_Update = NULL;
static xQueueHandle xQueue_Inputs_Update = NULL;

//Building Handlers for Xtaskcreate function, not sure what it's for
static TaskHandle_t	xMaster_TaskHandler = NULL;
static TaskHandle_t	xPID_TaskHandler = NULL;
static TaskHandle_t	xDisplay_TaskHandler = NULL;
static TaskHandle_t	xInputs_TaskHandler = NULL;

//===============================End of Instances===================

// The following variables are shared between non-interrupt processing and
// interrupt processing such that they must be global(and declared volatile)
volatile uint32_t	gpio_in;					// GPIO input port

//Hardware reading function inputs
//Seven Seg Display Vals

//Tricolor LED
volatile u32 LED1_Red			= 0;	//Default 0% of 255
volatile u32 LED1_Green  		= 0;	//Default 0%
volatile u32 LED1_Blue			= 0;	//Default 5%

volatile u32 switch_values		= 0;
volatile int notpressed_BTNU 	= 0;
volatile int notpressed_BTND 	= 0;
volatile int notpressed_BTNC 	= 0;

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
volatile uint32_t ticks = 0, lastticks = 1;


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
typedef struct{
	volatile bool direction;
	volatile u8 Kp;
	volatile u8 Ki;
	volatile u8 Kd;
	volatile u8  setpoint_current;
	volatile u8	 setpoint_target;
	volatile u32 RPM_Target;
	volatile u32 RPM_Current;
	volatile int RPM_Error;
	volatile u32 intg_Max;
	volatile u32 intg_min;
	volatile u32 EncoderVal;
	volatile double integral;
	volatile double integral_error;
	volatile double derivative;
	volatile double derivative_error;
	volatile double setpoint;
	volatile double error;
	volatile double prev_error;
}pid_vars;

volatile u8 wdt_crash_flag = 0;

/************************** Function Prototypes *****************************/
void PMDIO_itoa(int32_t value, char *string, int32_t radix);
void PMDIO_puthex(PmodOLEDrgb* InstancePtr, uint32_t num);
void PMDIO_putnum(PmodOLEDrgb* InstancePtr, int32_t num, int32_t radix);
int	 do_init(void);											// initialize system
void GPIO_PBSWITCH_Handler(void *p);										// fixed interval timer interrupt handler
int AXI_Timer_initialize(void);

void Master_thread(void *p);
void display_thread(void *p);
void PID_Controller_Thread();
void parameter_input_thread(void *p);

//Updaters for RTOS Conversion
void GreenLED_Update(pid_vars* pid_vars);
void GreenLED_Clear();
void ROT_ENC_Update(pid_vars* pid_vars);
bool ROT_ENC_State_Update();
void OLED_Initialize();
void OLED_Clear();
void PshBtn_Update(pid_vars* pid_vars);
void SSEG_Update(pid_vars* pid_vars);
void SSEG_Clear();
void Switch_Update();
void Watchdog_Hand(void *);
void Setpoint_RPM_Convert(pid_vars* pid_vars);
void SetpointFromRPM_Convert(pid_vars* pid_vars);
/*****************************************************************************/


/************************** MAIN PROGRAM ************************************/
int main()
{
	portBASE_TYPE xStatus;

	//init_platform
    init_platform();

	uint32_t sts;

	//Init peripherals
	sts = do_init();
	if (XST_SUCCESS != sts)
	{
		exit(1);
	}

	//Handle WDT expired event, reset will cycle back here
	if (XWdtTb_IsWdtExpired(&XWdtTbInstance))
	{
		//Handle it
		//Restart
		XWdtTb_RestartWdt(&XWdtTbInstance);
		//xil_printf("\n WDT Reinitialized\n\n");
	}

	microblaze_enable_interrupts();

	//xil_printf("ECE 544 Project 3 Test Program \n\r");
	//xil_printf("By Alex Beaulier. 13-May-2022\n\n\r");

	//START THE MASTER THREAD
	xStatus = xTaskCreate( Master_thread,
			 ( const char * ) "MasterThread",	//PC Name
			 1024,	//usStackDepth
			 NULL,
			 1,		//Priority
			 &xMaster_TaskHandler ); //Unsure what this does
	if(xStatus == pdFAIL){
		xil_printf("Failed Master Thread Generation\r\n");
	}

	//START RTOS
	vTaskStartScheduler();

	//CLEAN RTOS, SHOULD NEVER REACH IN THIS PROJECT
	cleanup_platform();

	return -1;	//Should never reach this line
}

/**************************** RTOS FUNCTIONS ******************************/
void Master_thread(void *p){
	portBASE_TYPE xStatus;

	//Create and initialize semaphores
	vSemaphoreCreateBinary(binary_sem);

	//Create and initialize message queue=================
	/* Sanity checks that the queues are created. */
	//Sending pid vars between the tasks for updating
	xQueue_PID_Update = xQueueCreate(1, sizeof(pid_vars));
	configASSERT(xQueue_PID_Update);

	xQueue_Display_Update = xQueueCreate(1, sizeof(pid_vars));
	configASSERT(xQueue_Display_Update);

	xQueue_Inputs_Update = xQueueCreate(1, sizeof(pid_vars));
	configASSERT(xQueue_Inputs_Update);
	//END Create and initialize message queue=================


	//TASKS/THREADS SETUP==========================================
	/**
	* Notes
	* The 3rd variable in xtaskcreate is the priority 1-10-> 10 is higher and will take priority
	* WDT and Interrupts need semaphore to stop them from continuously running and need sleep time
	* so the tasks can run
	* Each task should run forever with while loop.
	* Tasks should pass the PID parameters -> Structified for passability
	*
	*****************************************************************************/
	//Create Task_PID
	xStatus = xTaskCreate( PID_Controller_Thread,
					 ( const char * ) "RX PID Update",	//PC Name
					 1024,	//usStackDepth
					 NULL,
					 2,		//Priority
					 &xPID_TaskHandler ); //Unsure what this does
	 if( xStatus == pdPASS ){
		 //xil_printf("Passed PID Generation \r\n");
	 }
	//Create Task_Display
	xStatus = xTaskCreate( display_thread,
					 ( const char * ) "RX OLED Update",	//PC Name
					 1024,	//usStackDepth
					 NULL,
					 3,		//Priority
					 &xDisplay_TaskHandler ); //Unsure what this does
	if( xStatus == pdPASS ){
		 //xil_printf("Passed Displpay Generation\r\n");
	 }
	//Create Task_Inputs
	xStatus = xTaskCreate( parameter_input_thread,
					 ( const char * ) "TX Inputs",	//PC Name
					 1024,	//usStackDepth
					 NULL,
					 1,		//Priority
					 &xInputs_TaskHandler );	//Unsure what this does
	if( xStatus == pdPASS ){
		 //xil_printf("Passed Input Generation\r\n");
	 }
	//END TASKS/THREADS SETUP==========================================

	//Begin scheduling, possibly have to swap to main?
	//xil_printf("Starting the scheduler\r\n");

	//Register interrupt handlers
	//Enable WDT interrupt and start WDT
	XWdtTb_Start(&XWdtTbInstance);

	//Begin forever loop
	while(1){
	}
	return -1;	//Should never reach this line
}
/**********************************************************/


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
	const unsigned char ucSetToInput = 0xFFU;

	// initialize the Nexys4 driver and (some of)the devices
	status = (uint32_t) NX4IO_initialize(NX4IO_BASEADDR);
	if (status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}
	PMODHB3_initialize(PMODHB3_BASEADDR);
	// set all of the display digits to blanks and turn off
	// the decimal points using the "raw" set functions.
	// These registers are formatted according to the spec
	// and should remain unchanged when written to Nexys4IO...
	// something else to check w/ the debugger when we bring the
	// drivers up for the first time
	NX4IO_SSEG_setSSEG_DATA(SSEGLO, 0x1);

	// initialize the GPIO instances
	status = XGpio_Initialize(&GPIOInst0, GPIO_0_DEVICE_ID);
	if (status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}
	NX4IO_SSEG_setSSEG_DATA(SSEGLO, 0x2);

	//Green LEDs
	// GPIO0 channel 1 is an 8-bit input port.
	// GPIO0 channel 2 is an 8-bit output port.
	XGpio_SetDataDirection(&GPIOInst0, GPIO_0_INPUT_0_CHANNEL, 0xFF);
	//XGpio_SetDataDirection(&GPIOInst0, GPIO_0_OUTPUT_0_CHANNEL, 0xFF);
	NX4IO_SSEG_setSSEG_DATA(SSEGLO, 0x3);


	// GPIO Switches and Pushbutton
	status = XGpio_Initialize(&GPIOButton, GPIO_1_PBSWITCH_DEVICE_ID);
	if (status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}
	if( status == XST_SUCCESS )
	{
		/* Install the handler defined in this task for the button input.
		*NOTE* The FreeRTOS defined xPortInstallInterruptHandler() API function
		must be used for this purpose. */
		status = xPortInstallInterruptHandler(XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_1_IP2INTC_IRPT_INTR,GPIO_PBSWITCH_Handler, NULL );

		if( status == pdPASS )
		{
			xil_printf("Buttons and Switches interrupt handler installed\r\n");

			/* Set switches and buttons to input. */
			XGpio_SetDataDirection( &GPIOButton, 1, ucSetToInput ); //Pb
			XGpio_SetDataDirection( &GPIOButton, 2, ucSetToInput ); //Sw

			/* Enable the button input interrupts in the interrupt controller.
			*NOTE* The vPortEnableInterrupt() API function must be used for this
			purpose. */

			vPortEnableInterrupt( XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_1_IP2INTC_IRPT_INTR );

			/* Enable GPIO channel interrupts. */
			XGpio_InterruptEnable( &GPIOButton, XPAR_AXI_GPIO_1_IP2INTC_IRPT_MASK);
			XGpio_InterruptGlobalEnable( &GPIOButton );
		}
	}
	configASSERT( ( status == pdPASS ) );
	NX4IO_SSEG_setSSEG_DATA(SSEGLO, 0x4);
	//Initialize OLED
	OLEDrgb_begin(&pmodOLEDrgb_inst, RGBDSPLY_GPIO_BASEADDR, RGBDSPLY_SPI_BASEADDR);

	NX4IO_SSEG_setSSEG_DATA(SSEGLO, 0x5);
	//Initialize the pmodENC and hardware
	PMODENC544_initialize(PMODENC_BASEADDR);

	NX4IO_SSEG_setSSEG_DATA(SSEGLO, 0x6);
	status = AXI_Timer_initialize();
	if (status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	NX4IO_SSEG_setSSEG_DATA(SSEGLO, 0x7);

	// initialize the interrupt controller
	status = XIntc_Initialize(&IntrptCtlrInst, INTC_DEVICE_ID);
	if (status != XST_SUCCESS)
	{
	   return XST_FAILURE;
	}

	// start the interrupt controller such that interrupts are enabled for
	// all devices that cause interrupts.
	status = XIntc_Start(&IntrptCtlrInst, XIN_REAL_MODE);
	if (status != XST_SUCCESS)
	{
		xil_printf("Interrupt Failed Generation\r\n");
		return XST_FAILURE;
	}

	//Initialize watch dog
	//Can use this initialize instead of config -> Roy comments class
	status = XWdtTb_Initialize(&XWdtTbInstance,XPAR_AXI_TIMEBASE_WDT_0_DEVICE_ID);
	if(status != XST_SUCCESS)
	{
	  xil_printf("WDT Failed Generation\r\n");
	  return XST_FAILURE;
	}
	//xil_printf("WDT Initialized\r\n");
	status = xPortInstallInterruptHandler(WDTTB_INTERRUPT_ID, Watchdog_Hand, NULL);
	if(status != pdPASS)
	{
		return XST_FAILURE;
	}
	//xil_printf("WDT Handler Initialized\r\n");
	vPortEnableInterrupt(WDTTB_INTERRUPT_ID);

	//blank the display digits and turn off the decimal points
	SSEG_Clear();
	//Startup for OLED, prepwork before writing begins to eliminate writing these every time.
	OLED_Initialize();
	//Grab state for the loop
	laststate = PMODENC544_getBtnSwReg();

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
}


/**
* Masks the LEDs appropriately based on inputs
*
* @note
* ECE
 *****************************************************************************/
void GreenLED_Update(pid_vars* pid_vars){
	int switchvalues2 = 0;

	//Watchdog light
	u_int32_t mask1 = 1 << 15;
	if((switch_values & mask1) == mask1){
		switchvalues2 |= 1 << 15;
	}

	//2 = Kp
	if(pid_vars->Kp != 0){
		switchvalues2 |= 1 << 2;
	}
	//1 = Ki
	if(pid_vars->Ki != 0){
		switchvalues2 |= 1 << 1;
	}
	//0 = Kd
	if(pid_vars->Kd != 0){
		switchvalues2 |= 1 << 0;
	}
	XGpio_DiscreteWrite(&GPIOInst0, GPIO_0_INPUT_0_CHANNEL, switchvalues2);
}


/**
* Potential function without execution
*
* @note
* ECE
 *****************************************************************************/
void GreenLED_Clear(){
	//REPLACE NX4IO_setLEDs(0);
}


/**
* Update SSEG Display with the target PWM and the calculated target RPM
*
* @note
* ECE
 *****************************************************************************/
void SSEG_Update( pid_vars* pid_vars){
	u32_ss_disp_val = (pid_vars->setpoint_target  * 10000) + (pid_vars->RPM_Target); //simple answer...
	NX4IO_SSEG_putU32Dec(u32_ss_disp_val,0);
}

/**
* Clears all SSEG
*
* @note
* ECE
 *****************************************************************************/
void SSEG_Clear(){
	NX410_SSEG_setAllDigits(SSEGLO, CC_BLANK, CC_BLANK, CC_BLANK, CC_BLANK, DP_NONE);
	NX410_SSEG_setAllDigits(SSEGHI, CC_BLANK, CC_BLANK, CC_BLANK, CC_BLANK, DP_NONE);
}


/**
* Updates all Pushbuttons based on input, sets OLED locks for updating concisely
* Can Reset entire system with BTNC
* @note
* ECE
 *****************************************************************************/
void PshBtn_Update(pid_vars* pid_vars){
	if(Button_isPressed(&GPIOButton,BBTNU))
	{
		if(notpressed_BTNU == 0){
		notpressed_BTNU = 1;
		OLED_updatelock = 1;
		switch(Kpid_current_state){
			case KP:
				if(Incr_Status_KPID == One){
					pid_vars->Kp += 1;
				}
				if(Incr_Status_KPID == Five){
					pid_vars->Kp += 5;
				}
				if(Incr_Status_KPID == Ten){
					pid_vars->Kp += 10;
				}
			break;
			case KI:
				if(Incr_Status_KPID == One){
					pid_vars->Ki += 1;
				}
				if(Incr_Status_KPID == Five){
					pid_vars->Ki += 5;
				}
				if(Incr_Status_KPID == Ten){
					pid_vars->Ki += 10;
				}
			break;
			case KD:
				if(Incr_Status_KPID == One){
					pid_vars->Kd += 1;
				}
				if(Incr_Status_KPID == Five){
					pid_vars->Kd += 5;
				}
				if(Incr_Status_KPID == Ten){
					pid_vars->Kd += 10;
				}
			break;
			case Neutral:
			break;
		}
		}
	}else{
		notpressed_BTNU = 0;
	}
	if(Button_isPressed(&GPIOButton,BBTND))
	{
		if(notpressed_BTND == 0){
		notpressed_BTND = 1;
		OLED_updatelock = 1;
		switch(Kpid_current_state){
			case KP:
				if(Incr_Status_KPID == One){
					pid_vars->Kp = (pid_vars->Kp >= 1) ? pid_vars->Kp - 1 :  pid_vars->Kp;
				}
				if(Incr_Status_KPID == Five){
					pid_vars->Kp = (pid_vars->Kp >= 5) ? pid_vars->Kp - 5 :  pid_vars->Kp;
				}
				if(Incr_Status_KPID == Ten){
					pid_vars->Kp = (pid_vars->Kp >= 10) ? pid_vars->Kp - 10 : pid_vars->Kp;
				}
			break;
			case KI:
				if(Incr_Status_KPID == One){
					pid_vars->Ki = (pid_vars->Ki >= 1) ? pid_vars->Ki - 1 :  pid_vars->Ki;
				}
				if(Incr_Status_KPID == Five){
					pid_vars->Ki = (pid_vars->Ki >= 5) ? pid_vars->Ki - 5 :  pid_vars->Ki;
				}
				if(Incr_Status_KPID == Ten){
					pid_vars->Ki = (pid_vars->Ki >= 10) ? pid_vars->Ki - 10 : pid_vars->Ki;
				}
			break;
			case KD:
				if(Incr_Status_KPID == One){
					pid_vars->Kd = (pid_vars->Kd >= 1) ? pid_vars->Kd - 1 : pid_vars->Kd;
				}
				if(Incr_Status_KPID == Five){
					pid_vars->Kd = (pid_vars->Kd >= 5) ? pid_vars->Kd - 5 : pid_vars->Kd;
				}
				if(Incr_Status_KPID == Ten){
					pid_vars->Kd = (pid_vars->Kd >= 10) ? pid_vars->Kd - 10 : pid_vars->Kd;
				}
			break;
			case Neutral:
			break;
		}
		}
	}else{
		notpressed_BTND = 0;
	}

	if (Button_isPressed(&GPIOButton,BBTNL)){

	}
	if (Button_isPressed(&GPIOButton,BBTNR)){

	}
	if(Button_isPressed(&GPIOButton,BBTNC))
	{
		if(notpressed_BTNC == 0){
			notpressed_BTNC = 1;
			OLED_updatelock = 4;
			pid_vars->setpoint_target = 0; //Motor speed to 0 - Turn off PWM sig to motor

			//KPID constants to non zero val to guarantee effect
			pid_vars->Kp = 1;
			pid_vars->Ki = 1;
			pid_vars->Kd = 1;
		}
	}else{
		notpressed_BTNC = 0;
	}
}




/**
* Moves the setpoint based on ROTENC value
*
* @note
* ECE
 *****************************************************************************/
void ROT_ENC_Update(pid_vars* pid_vars){
	state = PMODENC544_getBtnSwReg();
	//Update the Encoder value, wrap if necessary
	ticks = PMODENC544_getRotaryCount();

	//Turn based update
	if(ticks < lastticks){//CW Turn, Increment
		OLED_updatelock = 2;
		switch(Incr_Status_ROT_ENC){
			case One:
			pid_vars->setpoint_target = (pid_vars->setpoint_target < 255) ? pid_vars->setpoint_target + 1 : pid_vars->setpoint_target;
			break;
			case Five:
				pid_vars->setpoint_target = (pid_vars->setpoint_target <= 250) ? pid_vars->setpoint_target + 5 : pid_vars->setpoint_target;
			break;
			case Ten:
				pid_vars->setpoint_target = (pid_vars->setpoint_target <= 245) ? pid_vars->setpoint_target + 10 : pid_vars->setpoint_target;
			break;
			case Default:
				pid_vars->setpoint_target == pid_vars->setpoint_target;
			break;
		}
	}
	if(ticks > lastticks){//CCW Turn, decrement
		OLED_updatelock = 2;
		switch(Incr_Status_ROT_ENC){
			case One:
				pid_vars->setpoint_target = (pid_vars->setpoint_target > 0) ? pid_vars->setpoint_target - 1 : pid_vars->setpoint_target - 0;
			break;
			case Five:
				pid_vars->setpoint_target = (pid_vars->setpoint_target >= 5) ? pid_vars->setpoint_target - 5 : pid_vars->setpoint_target - 0;
			break;
			case Ten:
				pid_vars->setpoint_target = (pid_vars->setpoint_target >= 10) ? pid_vars->setpoint_target - 10 : pid_vars->setpoint_target - 0;
			break;
			case Default:
				pid_vars->setpoint_target == pid_vars->setpoint_target;
			break;
		}
	}
	Setpoint_RPM_Convert(pid_vars);
	laststate = state;
	lastticks = ticks;

}

/**
* Switch status for direction control
*
* @note
* ECE
 *****************************************************************************/
bool ROT_ENC_State_Update(){
	u_int32_t BtnStatus;
	int mask1 = 1 << (2 - 1);
	BtnStatus = PMODENC544_getBtnSwReg();
	if((BtnStatus & mask1) == mask1){
		BtnStatus = 1;
	}else{
		BtnStatus = 0;
	}
	return (bool)BtnStatus;
}


/**
* Receive new messages from parameter_input_thread()
* Receive new messages from PID_thread()
* Update LEDs Update Display
* Updates SSEG
* Updates Green LED
* @note
* ECE
 *****************************************************************************/
void display_thread(void *p){
	pid_vars pid_vars_OLED, pid_var_prev;
	while(1){
		xQueueReceive(xQueue_Display_Update,&pid_vars_OLED,50);	//Update the parameters every loop
		if (pid_var_prev.RPM_Current != pid_vars_OLED.RPM_Current) {//ENC or center button
			//Write if RPM target == 0 and RPM current == 0 or RPM Target isn't 0 and RPM curr isnt 0-> Filter bad
			if((pid_vars_OLED.RPM_Current != 0 && pid_vars_OLED.RPM_Target != 0) ||
					(pid_vars_OLED.RPM_Current >= 0 && pid_vars_OLED.RPM_Target == 0)){
			OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 7, 1);
			OLEDrgb_PutString(&pmodOLEDrgb_inst,"    ");
			OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 7, 1);
			PMDIO_putnum(&pmodOLEDrgb_inst,pid_vars_OLED.RPM_Current,10);
			pid_var_prev.RPM_Current = pid_vars_OLED.RPM_Current;
			vTaskDelay(10);
			//usleep(100000);	//Can't do a sleep here, causes unresponsiveness
			}
		}
		if (pid_var_prev.RPM_Target != pid_vars_OLED.RPM_Target) {//ENC or center button
			OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 7, 2);
			OLEDrgb_PutString(&pmodOLEDrgb_inst,"    ");
			OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 7, 2);
			PMDIO_putnum(&pmodOLEDrgb_inst,pid_vars_OLED.RPM_Target,10);
			pid_var_prev.RPM_Target = pid_vars_OLED.RPM_Target;
		}
		if(OLED_updatelock == 1){//Pshbtns pressed
			if (Kpid_current_state == KP){
				OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 4, 3);
				OLEDrgb_PutString(&pmodOLEDrgb_inst,"    ");
				OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 4, 3);
				PMDIO_putnum(&pmodOLEDrgb_inst,pid_vars_OLED.Kp,10);
				OLED_updatelock = 0;
			}else if(Kpid_current_state == KI){
				OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 4, 4);
				OLEDrgb_PutString(&pmodOLEDrgb_inst,"    ");
				OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 4, 4);
				PMDIO_putnum(&pmodOLEDrgb_inst,pid_vars_OLED.Ki,10);
				OLED_updatelock = 0;
			}else if(Kpid_current_state == KD){
				OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 4, 5);
				OLEDrgb_PutString(&pmodOLEDrgb_inst,"    ");
				OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 4, 5);
				PMDIO_putnum(&pmodOLEDrgb_inst,pid_vars_OLED.Kd,10);
				OLED_updatelock = 0;
			}
		}
		if(OLED_updatelock == 4){ //Center button pressed
			OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 4, 3);
			OLEDrgb_PutString(&pmodOLEDrgb_inst,"    ");
			OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 4, 3);
			PMDIO_putnum(&pmodOLEDrgb_inst,pid_vars_OLED.Kp,10);
			OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 4, 4);
			OLEDrgb_PutString(&pmodOLEDrgb_inst,"    ");
			OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 4, 4);
			PMDIO_putnum(&pmodOLEDrgb_inst,pid_vars_OLED.Ki,10);
			OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 4, 5);
			OLEDrgb_PutString(&pmodOLEDrgb_inst,"    ");
			OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 4, 5);
			PMDIO_putnum(&pmodOLEDrgb_inst,pid_vars_OLED.Kd,10);
			OLED_updatelock = 0;
		}
		if(OLED_updatelock == 5){//Switches activated
			if (Kpid_current_state == KP){
				OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 7, 6);
				OLEDrgb_PutString(&pmodOLEDrgb_inst,"Kp");
				OLED_updatelock = 0;
			}else if(Kpid_current_state == KI){
				OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 7, 6);
				OLEDrgb_PutString(&pmodOLEDrgb_inst,"Ki");
				OLED_updatelock = 0;
			}else if(Kpid_current_state == KD){
				OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 7, 6);
				OLEDrgb_PutString(&pmodOLEDrgb_inst,"Kd");
				OLED_updatelock = 0;
			}
		}
		//xil_printf("Reached here\r\n");
		SSEG_Update(&pid_vars_OLED);
		GreenLED_Update(&pid_vars_OLED);
	}//EO While1

	return -2; //Should never reach here
}


/**
* Takes the input from Encoder and sets the direction signal.
* Calls pushbutton and switch update to send info out to display and PID threads
* @note
* ECE
 *****************************************************************************/
void parameter_input_thread(void *p){
	pid_vars pid_vars_OLED = {0};	//Initialize all to 0, otherwise randomness occurs
	int status;
	while(1){
		//Update PMODENC state
		//*NOTE PMOD enc not linked to interrupt semaphore, always read
		ROT_ENC_Update(&pid_vars_OLED);
		pid_vars_OLED.direction = ROT_ENC_State_Update();

		//TODO Turn this back on once interrupts on
		//if(xSemaphoreTake(binary_sem,10)){
			//Update Push Button
			PshBtn_Update(&pid_vars_OLED);
			//Update Switches
			Switch_Update();
		//}
		//Send message to update_display thread
		xQueueSend( xQueue_Display_Update,&pid_vars_OLED, mainDONT_BLOCK );
		//Send message to control params with setpoint
		xQueueSend( xQueue_PID_Update,&pid_vars_OLED, mainDONT_BLOCK );

	}
	return -3; //Should never reach here
}


/****************************************************************************/
/**
* PID_Controller_Thread() Function
*
* Calculates the appropriate compensation to an input system
* Based on reference from Kravitz 544 lecture notes
* (Output Control Methods) - May 4th
* Reads current RPM
* Passes info to display thread on the current RPM
* Calculates interpreted RPM and compensation
* Sends our the converted PID Compensation RPM as PWM to hardware 0-255
*
* @return *NONE*
*
* @note
*
*****************************************************************************/
void PID_Controller_Thread(){
	pid_vars pid_vars_PIDLocal,pid_vars_PIDPrev;
	int i;
	//xil_printf("Looped\r\n");

	while(1){

		//Receive new control parameters and setpoint
		xQueueReceive(xQueue_PID_Update,&pid_vars_PIDLocal,50);

		//Set the direction bit right away
		PMODHB3_setDIR(pid_vars_PIDLocal.direction);

		//motor speed from tachometer logic
		pid_vars_PIDLocal.RPM_Current = PMODHB3_getTachometer();	//Only updates every second

		//TODO Finish
		//Update the PID control algorithm
		//Calculate Proportional
		pid_vars_PIDLocal.RPM_Error = ((int)pid_vars_PIDLocal.RPM_Target - (int)pid_vars_PIDLocal.RPM_Current);

		//Calc Integral
		//Limit high and low
		if(pid_vars_PIDLocal.RPM_Error < (pid_vars_PIDLocal.RPM_Target/100)){
			pid_vars_PIDLocal.integral = (pid_vars_PIDLocal.integral + (double)pid_vars_PIDLocal.RPM_Error);
		}
		/*
		if(pid_vars_PIDLocal.integral > 1000)	//Max from sweep
			pid_vars_PIDLocal.integral = 1000;	//Max from sweep
		if(pid_vars_PIDLocal.integral < 0)
			pid_vars_PIDLocal.integral = 0;
		 */

		//Calc Deriv errors (d error, prev)
		pid_vars_PIDLocal.derivative = ((double)pid_vars_PIDLocal.RPM_Error - pid_vars_PIDPrev.prev_error); //assuming dt == 1 from delay sample time

		//Calculate new PID output
		pid_vars_PIDLocal.setpoint = ((double)pid_vars_PIDLocal.RPM_Error * (double)pid_vars_PIDLocal.Kp) +
				((pid_vars_PIDLocal.integral) * (double)(pid_vars_PIDLocal.Ki)) +
				(pid_vars_PIDLocal.derivative * (double)pid_vars_PIDLocal.Kd);

		//Convert the PID RPM Setpoint to a scaled pwm value
		//* Using sweep data @ 5.7V .9A draw
		//0 - 1000 -> 0 - 255

		//Convert RPM setpoint to a pwm setpoint
		SetpointFromRPM_Convert(&pid_vars_PIDLocal);

		//xil_printf("PWM Output %d\r\n",(int)pid_vars_PIDLocal.setpoint*1000/255);

		//Limit the PWM value between 0 to 255
		if(pid_vars_PIDLocal.setpoint > 255)
			pid_vars_PIDLocal.setpoint = 255;
		if (pid_vars_PIDLocal.setpoint < 0)
			pid_vars_PIDLocal.setpoint = 0;

		//Debug sweep python read from serial
		/*xil_printf("RPM_C: %.4d,RPM_T:%.4d,Kp:%.5d,Ki:%.4d,Kd:%.5d\r\n",
				pid_vars_PIDLocal.RPM_Current,pid_vars_PIDLocal.RPM_Target,
				(int)pid_vars_PIDLocal.RPM_Error, (int)pid_vars_PIDLocal.integral, (int)pid_vars_PIDLocal.derivative);
		*/
		xil_printf("%d,%d\r\n", pid_vars_PIDLocal.RPM_Current,pid_vars_PIDLocal.RPM_Target);

		//Put the setpoint PWM target into the motor
		//xil_printf("PWM Output %d\r\n",(int)pid_vars_PIDLocal.setpoint);
		PMODHB3_setPWM(pid_vars_PIDLocal.setpoint);


		pid_vars_PIDPrev.prev_error = pid_vars_PIDLocal.RPM_Error;

		xQueueSend( xQueue_Display_Update,&pid_vars_PIDLocal, mainDONT_BLOCK );
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}


void Switch_Update(){
	u_int32_t mask1, mask2;

	switch_values = XGpio_DiscreteRead(&GPIOButton,GPIO_1_Channel_2);

	//SW 15 Watchdog
	mask1 = 1 << (16 - 1);
	if((switch_values & mask1) == mask1){
		//Crash the system here, use a flag
		wdt_crash_flag = 1;
		//xil_printf("\n WDT Flag Set\r\n");
	}else{
		wdt_crash_flag = 0;
	}

	/*
	//SW 14 Test Direction
	mask1 = 1 << (15 - 1);
	if((switch_values & mask1) == mask1){
		PMODHB3_setDIR(1);
	}else{
		PMODHB3_setDIR(0);
	}

	//Tryout sleep test here Direction verification/Step fix

	//SW 13 Test PWM
	mask1 = 1 << (14 - 1);
	if((switch_values & mask1) == mask1){
		PMODHB3_setPWM(0xFFFF);		//HALF ON 16 bits
	}else{
		PMODHB3_setPWM(0);
	}
	*/

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

/**************************** INTERRUPT HANDLERS ******************************/
/****************************************************************************/
/**
* GPIO BTNSW interrupt handler
* Enables a semaphore for the gpio input function thread
* Clears the interrupt instance
 *****************************************************************************/
void GPIO_PBSWITCH_Handler(void *p){
	//xil_printf("I AM HERE@@@@\r\n");
	xSemaphoreGiveFromISR(binary_sem,NULL);
	XGpio_InterruptClear( &GPIOButton, 1);
}

void Watchdog_Hand(void *p)
{
	//xil_printf("In WDT\r\n");
	if(!wdt_crash_flag)
	{
		//xil_printf("WDT normal, resetting\r\n");
		XWdtTb_RestartWdt(&XWdtTbInstance);
	}
	else
	{
		//xil_printf("WDT Forcing Crash\r\n");
	}
}

/**
* Calculates the target RPM from the PWM setpoint
* @note
* ECE
 *****************************************************************************/
void Setpoint_RPM_Convert(pid_vars* pid_vars){

	pid_vars->RPM_Target = ((pid_vars->setpoint_target *1000)/255); //Scale the target linearly from 0 to max range of pwm
	/*
	switch(pid_vars->setpoint_target){
	case 0:
		pid_vars->RPM_Target = 0;
	break;

	case 1   ...  50:
	pid_vars->RPM_Target = (u32)((double)pid_vars->setpoint_target / 0.068493151);
	break;

	case 51  ... 100:
	pid_vars->RPM_Target = (u32)((double)pid_vars->setpoint_target / 0.114285714);
	break;

	case 101 ... 150:
	pid_vars->RPM_Target = (u32)((double)pid_vars->setpoint_target / 0.166852058);
	break;

	case 151 ... 200:
	pid_vars->RPM_Target = (u32)((double)pid_vars->setpoint_target / 0.223311547);
	break;

	case 201 ... 255:
	pid_vars->RPM_Target = (u32)((double)pid_vars->setpoint_target / 0.255178268);
	break;
	}*/
}

/**
* Calculates the output from controller as PWM from the RPM Target compensation setpoint from PID
* @note
* ECE
 *****************************************************************************/
void SetpointFromRPM_Convert(pid_vars* pid_vars){
	pid_vars->setpoint_target = ((pid_vars->setpoint_target *1000)/255); //Scale the target linearly from 0 to max range of pwm
}



