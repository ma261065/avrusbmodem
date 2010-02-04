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
		#include <stdlib.h>
		#include <ctype.h>
		#include <string.h>

		#include <LUFA/Version.h>
		#include <LUFA/Drivers/USB/USB.h>
		#include <LUFA/Drivers/Peripheral/SerialStream.h>
		#include <LUFA/Drivers/Board/LEDs.h>
		
		#include <uIP-Contiki/network.h>
		#include <uIP-Contiki/timer.h>
		#include <uIP-Contiki/uip.h>

		#include "ConfigDescriptor.h"
		#include "Lib/Modem.h"
		#include "Lib/Debug.h"
		#include "Lib/PPP.h"

	/* Macros: */
		/** Serial baud rate for debugging */
		#define UART_BAUD_RATE            19200
	
		/** Pipe number for the CDC data IN pipe */
		#define CDC_DATAPIPE_IN           1

		/** Pipe number for the CDC data OUT pipe */
		#define CDC_DATAPIPE_OUT          2

		/** Pipe number for the CDC notification pipe */
		#define CDC_NOTIFICATIONPIPE      3

		/** LED mask for the library LED driver, to indicate that the USB interface is not ready. */
		#define LEDMASK_USB_NOTREADY      LEDS_LED1

		/** LED mask for the library LED driver, to indicate that the USB interface is enumerating. */
		#define LEDMASK_USB_ENUMERATING  (LEDS_LED2 | LEDS_LED3)

		/** LED mask for the library LED driver, to indicate that the USB interface is ready. */
		#define LEDMASK_USB_READY        (LEDS_LED2 | LEDS_LED4)

		/** LED mask for the library LED driver, to indicate that an error has occurred in the USB interface. */
		#define LEDMASK_USB_ERROR        (LEDS_LED1 | LEDS_LED3)
	
	/* External Variables: */
		extern char WatchdogTicks;
		
	/* Function Prototypes: */
		void SetupHardware(void);
		void CDC_Host_Task(void);
	
		void EVENT_USB_Host_HostError(const uint8_t ErrorCode);
		void EVENT_USB_Host_DeviceAttached(void);
		void EVENT_USB_Host_DeviceUnattached(void);
		void EVENT_USB_Host_DeviceEnumerationFailed(const uint8_t ErrorCode, const uint8_t SubErrorCode);
		void EVENT_USB_Host_DeviceEnumerationComplete(void);

		void SendDataToAndFromModem(void);
		void UpdateStatus(uint8_t CurrentStatus);
		unsigned int modem_getc(void);
		void modem_putc(unsigned char data);
		void modem_puts(const char *s);
		void Dial(void);
		void TCPIPTask(void);
		extern void TCPCallback(void);
		void SendGET(void);
		void SendPOST(void);

		void device_enqueue(char *x, int len);
		bool device_queue_full(void);

		void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));
#endif


