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

#define  INCLUDE_FROM_USBMODEM_C
#include "USBModem.h"

uint8_t  WatchdogTicks = 0;
uint16_t SystemTicks   = 0;									// 10 millseconds counter

ISR(TIMER1_COMPA_vect)										// Timer 1 interrupt handler
{
	SystemTicks++;
	PPP_LinkTimer();
}

ISR(TIMER3_COMPA_vect)										// Timeout Timer interrupt handler
{
	if (++WatchdogTicks >= 23)								// 23 * 8s = 3 minutes. If we've received no data in 3 minutes reboot.
	{
		Debug_Print("Timeout. Rebooting...\r\n");
		Reboot();
	}
}

// Once the watchdog is enabled, then it stays enabled, even after a reset! 
// A function needs to be added to the .init3 section (i.e. during the startup code, before main()) to disable the watchdog early enough so it does not continually reset the AVR.
void WDT_Init(void)
{
    MCUSR = 0;												// Clear the source of the reset
    wdt_disable();

	wdt_reset();
	WDTCSR = ((1 << WDCE) | (1 << WDE));		
	WDTCSR = ((1 << WDE) | (1 << WDP3));					// Watchdog Initialization (4 Second Period)
}

void Reboot(void)
{
	WDTCSR = ((1 << WDCE) | (1 << WDE));
	WDTCSR = ((1 << WDE));									// Set watchdog timer to 16ms
	_delay_ms(30);											// Wait longer than 16ms
}

// Main program entry point. This routine configures the hardware required by the application, then runs the application tasks.
int main(void)
{	
	SetupHardware();
	ConnectedState = LINKMANAGEMENT_STATE_Idle;
	
	while(true)
	{
		wdt_reset();

		LinkManagement_ManageConnectionState();
		USBManagement_ManageUSBState();
		USB_USBTask();

		switch (getchar())
		{
			case '!':
				puts("\r\nDebug ON\r\n");
				DebugModeEnabled = true;
				break;
			case '@':
				puts("\r\nDebug OFF\r\n");
				DebugModeEnabled = false;
				break;
		}
	}
}

void SetupHardware(void)
{
	// Disable clock division
	clock_prescale_set(clock_div_1);

	// Enable interrups
	sei();
	
	SerialStream_Init(UART_BAUD_RATE, false);
	puts("\r\nUSB Modem - Copyright (C) 2011 Mike Alexander and Dean Camera"
	     "\r\n   *** Press '!' to enable debugging, '@' to disable ***\r\n");
	
	USB_Host_SuspendBus();									// If the WDT has reset the AVR, need to give the modem time to reinitialise
	_delay_ms(HOST_DEVICE_SETTLE_DELAY_MS);
	
	// Hardware Initialization
	LEDs_Init();
	USB_Init();
	
	// Timer 1 is used as a general 10ms timekeeping timer (Timer 0 is used by TCP/IP stack)
	TCCR1B = ((1 << WGM12) | (1 << CS10) | (1 << CS12));	// CK/1024 prescale, CTC mode
	OCR1A  = ((F_CPU / 1024) / 100);						// 10ms timer period
	TIMSK1 = (1 << OCIE1A);									// Enable interrupt on Timer compare

	// Timer 3 is used as a timeout timer
	TCCR3B = ((1 << WGM32) | (1 << CS30) | (1 << CS32));	// CK/1024 prescale, CTC mode
	OCR3A  = ((F_CPU / 1024) * 8);							// 8s timer period
	TIMSK3 = (1 << OCIE3A);									// Enable interrupt on Timer compare
	
	// Modem Packet Ring Buffer Initialization
	Buffer_Initialize(&Modem_SendBuffer);
	Buffer_Initialize(&Modem_ReceiveBuffer);
}
