/*
    LUFA Powered Wireless 3G Modem Host
	
    Copyright (C) Mike Alexander, 2010.
     Copyright (C) Dean Camera, 2010.
*/

/*
  Copyright 2010  Mike Alexander (mike [at] mikealex [dot] com)
  Copyright 2010  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this 
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in 
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting 
  documentation, and that the name of the author not be used in 
  advertising or publicity pertaining to distribution of the 
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Header file for USBManagement.c.
 */

#ifndef _USB_MANAGEMENT_H_
#define _USB_MANAGEMENT_H_

	/* Includes: */
		#include <avr/io.h>
		#include <LUFA/Drivers/USB/USB.h>
		#include "Lib/RingBuff.h"
		#include "ConfigDescriptor.h"

	/* Macros: */
		/** Pipe number for the CDC data IN pipe */
		#define CDC_DATA_IN_PIPE           (PIPE_DIR_IN | 1)

		/** Pipe number for the CDC data OUT pipe */
		#define CDC_DATA_OUT_PIPE          (PIPE_DIR_OUT | 2)

		/** Pipe number for the CDC notification pipe */
		#define CDC_NOTIFICATION_PIPE      (PIPE_DIR_IN | 3) 
	
	/* External Variables: */
		extern uint8_t ConnectedState;
		
	/* Function Prototypes: */
		void USBManagement_ManageUSBState(void);
		void USBManagement_SendReceivePipes(void);

#endif


