//#ifndef GPIOfunctions
//#define GPIOfunctions

enum GPIO_btns {BBTNR, BBTNL, BBTND, BBTNU, BBTNC};

// API function prototypes
bool Button_isPressed(XGpio * InstancePtr,enum GPIO_btns btnslct);
void GPIO_setLEDs(XGpio * InstancePtr,u32 ledvalue);

//#endif // PMODENC544_H
