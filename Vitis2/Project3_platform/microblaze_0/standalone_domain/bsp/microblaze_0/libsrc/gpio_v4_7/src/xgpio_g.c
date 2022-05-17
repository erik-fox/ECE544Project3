
/*******************************************************************
*
* CAUTION: This file is automatically generated by HSI.
* Version: 2020.2
* DO NOT EDIT.
*
* Copyright (C) 2010-2022 Xilinx, Inc. All Rights Reserved.
* SPDX-License-Identifier: MIT 

* 
* Description: Driver configuration
*
*******************************************************************/

#include "xparameters.h"
#include "xgpio.h"

/*
* The configuration table for devices
*/

XGpio_Config XGpio_ConfigTable[XPAR_XGPIO_NUM_INSTANCES] =
{
	{
		XPAR_AXI_GPIO_0_DEVICE_ID,
		XPAR_AXI_GPIO_0_BASEADDR,
		XPAR_AXI_GPIO_0_INTERRUPT_PRESENT,
		XPAR_AXI_GPIO_0_IS_DUAL
	},
	{
		XPAR_AXI_GPIO_BLUE_HIGH_DEVICE_ID,
		XPAR_AXI_GPIO_BLUE_HIGH_BASEADDR,
		XPAR_AXI_GPIO_BLUE_HIGH_INTERRUPT_PRESENT,
		XPAR_AXI_GPIO_BLUE_HIGH_IS_DUAL
	},
	{
		XPAR_AXI_GPIO_GREEN_HIGH_DEVICE_ID,
		XPAR_AXI_GPIO_GREEN_HIGH_BASEADDR,
		XPAR_AXI_GPIO_GREEN_HIGH_INTERRUPT_PRESENT,
		XPAR_AXI_GPIO_GREEN_HIGH_IS_DUAL
	},
	{
		XPAR_AXI_GPIO_RED_HIGH_DEVICE_ID,
		XPAR_AXI_GPIO_RED_HIGH_BASEADDR,
		XPAR_AXI_GPIO_RED_HIGH_INTERRUPT_PRESENT,
		XPAR_AXI_GPIO_RED_HIGH_IS_DUAL
	}
};

