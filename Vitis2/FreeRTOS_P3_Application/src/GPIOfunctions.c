#define BUTTON_C 0x01
#define BUTTON_D 0x02
#define BUTTON_L 0x04
#define BUTTON_R 0x08
#define BUTTON_U 0x10
#define BUTTON_CHANNEL 1
#define SWITCH_CHANNEL 2
#define GPIO_LEDS_MASK 0x0000FFFF

#include "PmodENC544.h"
#include "xgpio.h"
#include "GPIOfunctions.h"

bool Button_isPressed(XGpio * InstancePtr,enum GPIO_btns btnslct)
{
	u8 btns, msk;

	btns = XGpio_DiscreteRead(InstancePtr, BUTTON_CHANNEL);
	switch (btnslct)
	{
		case BBTNR:
			msk = BUTTON_R;
			break;
		case BBTNL:
			msk = BUTTON_L;
			break;
		case BBTND:
			msk = BUTTON_D;
			break;
		case BBTNU:
			msk = BUTTON_U;
			break;
		case BBTNC:
			msk = BUTTON_C;
			break;
		default:		msk = 0x00;	break;
	}
	return ((btns & msk) != 0) ? true : false;
}

void GPIO_setLEDs(XGpio * InstancePtr,u32 ledvalue)
{
	u32 val;

	val = ledvalue & GPIO_LEDS_MASK;
	XGpio_DiscreteWrite(InstancePtr, 1, val);
}
