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
 *  Header file for USBModem.c.
 */

#ifndef _USB_MODEM_H_
#define _USB_MODEM_H_

	/* Includes: */
		#include <avr/io.h>
		#include <avr/wdt.h>
		#include <avr/pgmspace.h>
		#include <avr/power.h>
		#include <stdio.h>

		#include <LUFA/Version.h>
		#include <LUFA/Common/Common.h>
		#include <LUFA/Drivers/USB/USB.h>
		#include <LUFA/Drivers/Peripheral/SerialStream.h>
		#include <LUFA/Drivers/Board/LEDs.h>
		
		#include <uIP-Contiki/network.h>
		#include <uIP-Contiki/timer.h>
		#include <uIP-Contiki/uip.h>

		#include "Lib/RingBuff.h"
		#include "Lib/Debug.h"
		#include "Lib/PPP.h"

		#include "ConfigDescriptor.h"
		#include "USBManagement.h"
		#include "LinkManagement.h"

	/* Macros: */
		/** Serial baud rate for debugging */
		#define UART_BAUD_RATE            19200

		/** LED mask for the library LED driver, to indicate that the USB interface is not ready. */
		#define LEDMASK_USB_NOTREADY      LEDS_LED1

		/** LED mask for the library LED driver, to indicate that the USB interface is enumerating. */
		#define LEDMASK_USB_ENUMERATING  (LEDS_LED2 | LEDS_LED3)

		/** LED mask for the library LED driver, to indicate that the USB interface is ready. */
		#define LEDMASK_USB_READY        (LEDS_LED2 | LEDS_LED4)

		/** LED mask for the library LED driver, to indicate that an error has occurred in the USB interface. */
		#define LEDMASK_USB_ERROR        (LEDS_LED1 | LEDS_LED3)
	
	/* External Variables: */
		extern uint8_t WatchdogTicks;
		extern uint8_t ConnectedState;
		
	/* Function Prototypes: */
		void SetupHardware(void);
		void WDT_Init(void) ATTR_NAKED ATTR_INIT_SECTION(3);

#endif
