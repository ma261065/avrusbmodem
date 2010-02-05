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

#include "USBModem.h"

// Global Variables
uint8_t  WatchdogTicks = 0;
uint16_t TIME;												// 10 millseconds counter


// Interrupt Handlers
ISR(TIMER1_COMPA_vect)										// Timer 1 interrupt handler
{
	TIME++;
}

ISR(WDT_vect)												// Watchdog Timer interrupt handler
{
	if (++WatchdogTicks >= 23)								// 23 * 8s = 3 minutes. If we've received no data in 3 minutes reboot.
	{
		WDTCSR = ((1 << WDCE) | (1 << WDE));				// Set watchdog timer to reboot rather than interrupt next time it fires
		Debug_Print("Watchdog reboot\r\n");
	}
}

// Once the watchdog is enabled, then it stays enabled, even after a reset! 
// A function needs to be added to the .init3 section (i.e. during the startup code, before main()) to disable the watchdog early enough so it does not continually reset the AVR.
void WDT_Init(void)
{
    MCUSR = 0;
    wdt_disable();

    return;
}

void SetupHardware(void)
{
	// Disable clock division
	clock_prescale_set(clock_div_1);

	// Hardware Initialization
	LEDs_Init();
	SerialStream_Init(UART_BAUD_RATE, false);
	
	// Initialize USB Subsystem
	USB_Init();

	// Timer 1
	TCCR1B = ((1 << WGM12) | (1 << CS10) | (1 << CS12));	// CK/1024 prescale, CTC mode
	OCR1A  = ((F_CPU / 1024) / 100);						// 10ms timer period
	TIMSK1 = (1 << OCIE1A);									// Enable interrupt on Timer compare
	
	// Set up ring buffers
	Buffer_Initialize(&Modem_SendBuffer);
	Buffer_Initialize(&Modem_ReceiveBuffer);
	
	// Set the Watchdog timer to interrupt (not reset) every 8 seconds
	wdt_reset();
	WDTCSR = ((1 << WDCE) | (1 << WDE));		
	WDTCSR = ((1 << WDIE) | (1 << WDP0) | (1 << WDP3));

	// Reset the 10ms timer	
	TIME = 0;
}

// Main program entry point. This routine configures the hardware required by the application, then runs the application tasks.
int main(void)
{	
	SetupHardware();
	
	// Startup message - make sure the first 5 chars do not contain a space as terminal will echo this back
	puts("\r\nUSB Modem - Press space bar to debug\r\n");
	for (int i = 0; i <= 5; i++)
	{
		putchar('.');
		
		if (getchar() == ' ')
		{
			DebugModeEnabled = true;
			puts("\r\nDebugging\r\n");
			break;
		}
		
		_delay_ms(500);
	}
	puts("\r\n");

	// Blink the lights for a bit
	LEDs_SetAllLEDs(LEDMASK_USB_READY);
	_delay_ms(500);
	LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
	_delay_ms(500);
	LEDs_SetAllLEDs(LEDMASK_USB_READY);
	
	for(;;)
	{
		LinkManagement_ManageConnectionState();
		USBManagement_ManageUSBStateMachine();
		USB_USBTask();

		switch (getchar())
		{
			case '!':
				puts("\r\nDebug on\r\n");
				DebugModeEnabled = true;
				break;
			case '@':
				puts("\r\nDebug off\r\n");
				DebugModeEnabled = false;
				break;
		}
	}
}
