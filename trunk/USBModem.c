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
char ConnectedState = 0;
char IPAddr1, IPAddr2, IPAddr3, IPAddr4;
char WatchdogTicks = 0;
unsigned char DialSteps = 0;
struct uip_conn* ThisConn;
uip_ipaddr_t LocalIPAddress, RemoteIPAddress;
struct timer periodic_timer;
unsigned int TIME;											// 10 millseconds counter

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
void wdt_init(void)
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
	
	//Startup message
	puts("\r\nUSB Modem - Press space bar to debug\r\n");			// Make sure the first 5 chars do not contain a space as terminal will echo this back
	
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
		USB_USBTask();
		USBManagement_ManageUSBStateMachine();
		USBManagement_SendReceivePipes();
		
		switch (ConnectedState)
		{
			case 0:
				Dial();
				break;
			case 1:
				DoPPP();
				break;
			case 2:
				Debug_Print("Initialise TCP Stack\r\n");
			
				network_init();
	
				clock_init();

				timer_set(&periodic_timer, CLOCK_SECOND / 2);
			
				uip_init();

				// Set this machine's IP address
				uip_ipaddr(&LocalIPAddress, IPAddr1, IPAddr2, IPAddr3, IPAddr4);
				uip_sethostaddr(&LocalIPAddress);

				// Set remote IP address
				uip_ipaddr(&RemoteIPAddress, 192,0,32,10);	// www.example.com

				ConnectedState = 3;
				TIME = 2000;			// Make the first CONNECT happen straight away
				break;
			case 3:
				if (TIME > 1000)		//Try to connect every 1 second
				{
					TIME = 0;
					
					// Connect to the remote machine
					ThisConn = uip_connect(&RemoteIPAddress, HTONS(80));

					if (ThisConn != 0)
					{
						Debug_Print("Connected to host\r\n");
						ConnectedState = 4;
						TIME = 3001;			// Make the first GET happen straight away
					}
					else
						Debug_Print("Failed to Connect\r\n");

					Debug_Print("Maximum Segment Size: 0x"); Debug_PrintHex(uip_mss() / 256);
					Debug_Print("0x"); Debug_PrintHex(uip_mss() & 255); 
					Debug_Print("\r\n");
				}
				break;
			case 4:
				TCPIPTask();
				break;
		}
	}
}

extern void TCPCallback(void)
{
	Debug_PrintChar('*');

	if (uip_newdata())
		Debug_Print("NewData ");

	if (uip_acked())
		Debug_Print("Acked ");
	
	if (uip_connected())
		Debug_Print("Connected ");

	if (uip_closed())
	{
		Debug_Print("Closed - Reconnecting...");
		_delay_ms(1000);
		ConnectedState = 3;
	}

	if (uip_aborted())
	{
		Debug_Print("Aborted - Reconnecting... ");
		_delay_ms(1000);
		ConnectedState = 3;
	}

	if (uip_timedout())
	{
		Debug_Print("Timeout - Reconnecting...");
		uip_abort();
		_delay_ms(1000);
		ConnectedState = 3;
	}

	if (uip_poll() && TIME > 3000)
	{
		TIME = 0;
		
		Debug_Print("\r\nSending GET\r\n");
		SendGET();
	}
	
	if (uip_rexmit())
	{
		Debug_Print("\r\nRetransmit GET\r\n");
		SendGET();
	}

	if (uip_newdata())
	{
		device_enqueue(uip_appdata, uip_datalen());
		
		if (device_queue_full())
		{
			uip_stop();
		}
	}

	if (uip_poll() && uip_stopped(ThisConn))
	{
		if (!device_queue_full())
		{
			uip_restart();
		}
	}
}

void SendGET(void)
{
	uip_send("GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: Keep-Alive\r\n\r\n", 65);
}

void TCPIPTask(void)
{
	int i, j;

	uip_len = network_read();

	if (uip_len == -1)								// Got a non-SLIP packet. Probably a LCP-TERM Re-establish link.
	{
		Debug_Print("Got non-PPP packet\r\n");
		TIME = 0;
		ConnectedState = 0;
		return;
	}

	if (uip_len > 0)								// We have some data to process
	{
	
		/********************** Debug **********************/

		Debug_Print("\r\nReceive:\r\n");
	
		for (i = 0; i < uip_len; i += 16)
		{	
			// Print the hex
			for (j = 0; j < 16; j++)
			{
				if ((i + j) >= uip_len)
					break;

				Debug_PrintHex(*(uip_buf + i + j));
			}
			
			Debug_Print("\r\n");	
			
			// Print the ASCII
			for (j = 0; j < 16; j++)
			{
				if ((i + j) >= uip_len)
					break;

				if (*(uip_buf + i + j) >= 0x20 && *(uip_buf + i + j) <= 0x7e)
				{
					Debug_PrintChar(' ');
					Debug_PrintChar(*(uip_buf + i + j));
					Debug_PrintChar(' ');
				}
				else
					Debug_Print(" . ");

			}
			Debug_Print("\r\n");
		}

		/********************** Debug **********************/

		uip_input();
 
	 	// If the above function invocation resulted in data that should be sent out on the network, the global variable uip_len is set to a value > 0.
	 	if (uip_len > 0)
		{
	 		network_send();
	 	}
	}
	else if (timer_expired(&periodic_timer))
	{
		timer_reset(&periodic_timer);

		for (int i = 0; i < UIP_CONNS; i++)
		{
	 		uip_periodic(i);
 
	 		// If the above function invocation resulted in data that should be sent out on the network, the global variable uip_len is set to a value > 0.
	 		if (uip_len > 0)
	 		{
	 			network_send();
	 		}
		}
	}

	char c;

	// Read any available data from the serial port.
	// If we see a '!' in the input stream, switch debug mode on. If we see a "@", switch debug mode off.
	while ((c = getchar()) != EOF)
	{
		if (c == '!')
		{
			puts("\r\nDebug on\r\n");
			DebugModeEnabled = true;
		}
		else if (c == '@')
		{
			puts("\r\nDebug off\r\n");
			DebugModeEnabled = false;
		}

		c = getchar();
	}	
}

const char *DialCommands[] = 
{	
	"AT\r\n",
	"AT&F\r\n",
	"AT+CPMS=\"SM\",\"SM\",\"\"\r\n",
	"ATQ0 V1 E1 S0=0 &C1 &D2 +FCLASS=0\r\n",
	"AT+CGDCONT=1,\"IP\",\"3services\",,0,0\r\n",
	"ATDT*99#\r\n",
	"PPP"											// PPP is a special case to transition to next state
};

void Dial(void)
{
	char Command[64];

	if (USB_HostState == HOST_STATE_Configured)	
	{
		while (Modem_ReceiveBuffer.Elements)
		  Debug_PrintChar(Buffer_GetElement(&Modem_ReceiveBuffer));
			
		if (TIME > 100)
		{
			TIME = 0;
			strcpy(Command, DialCommands[DialSteps++]);

			if (strcmp(Command, "PPP") == 0)
			{
				Debug_Print("Starting PPP\r\n");
				DialSteps = 0;
				ConnectedState = 1;
				return;
			}

			Debug_Print("Sending command: ");
			Debug_Print(Command);
			
			char* CommandPtr = Command;
			while (*CommandPtr != 0x00)
			  Buffer_StoreElement(&Modem_SendBuffer, *(CommandPtr++));
		}
	}
}

void device_enqueue(char *x, int len)
{
	Debug_Print("\r\nData:\r\n");

	for (int i = 0; i < len; i++)
	{
		WatchdogTicks = 0;							// Reset the watchdog count
		putchar(*(x + i));
	}

	Debug_Print("\r\n");
}

bool device_queue_full(void)						// TODO: Figure out how to return a proper value here
{
	return false;
}
