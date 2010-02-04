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
char DebugMode = 0;
char WatchdogTicks = 0;
unsigned char DialSteps = 0;
struct uip_conn* ThisConn;
uip_ipaddr_t LocalIPAddress, RemoteIPAddress;
struct timer periodic_timer;
unsigned int TIME;											// 10 millseconds counter

// Defines
#define UART_BAUD_RATE 19200

// Interrupt Handlers
ISR(TIMER1_COMPA_vect)										// Timer 1 interrupt handler
{
	TIME++;
}

ISR(WDT_vect)												// Watchdog Timer interrupt handler
{
	if (++WatchdogTicks >= 23)								// 23 * 8s = 3 minutes. If we've received no data in 3 minutes reboot.
	{
		WDTCSR = _BV(WDCE) | _BV(WDE);						// Set watchdog timer to reboot rather than interrupt next time it fires
		Debug("Watchdog reboot\r\n");
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

// Main program entry point. This routine configures the hardware required by the application, then runs the application tasks.
int main(void)
{
	// Disable clock division
	clock_prescale_set(clock_div_1);

	// Hardware Initialization
	uart1_init(UART_BAUD_SELECT(UART_BAUD_RATE, F_CPU)); 	// Initialise the UART
	modem_init();
	LEDs_Init();

	// Blink the lights for a bit
	LEDs_SetAllLEDs(LEDMASK_USB_READY);
	_delay_ms(500);
	LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
	_delay_ms(500);
	LEDs_SetAllLEDs(LEDMASK_USB_READY);
	
	// Initialize USB Subsystem
	USB_Init();
	
	//Startup message
	uart1_puts("\r\nUSB Modem - Press space bar to debug\r\n");			// Make sure the first 5 chars do not contain a space as terminal will echo this back
	
	for (int i = 0; i <= 5; i++)
	{
		uart1_putc('.');
		
		if (uart1_getc() == ' ')
		{
			DebugMode = 1;
			uart1_puts("\r\nDebugging\r\n");
			break;
		}
		
		_delay_ms(500);
	}

	uart1_puts("\r\n");

	// Timer 1
	TCCR1B = _BV(WGM12) | _BV(CS10) | _BV(CS12);			// CK/1024 prescale, CTC mode
	OCR1A = 78;												// 10ms timer. 8,000,000 / 1,024 * 0.01
	TIMSK1 = _BV(OCIE1A); 									// Enable interrupt on Timer compare
	
	LEDs_SetAllLEDs(LEDS_NO_LEDS);							// Turn the LEDs off

	wdt_reset();
	WDTCSR = _BV(WDCE) | _BV(WDE);						
	WDTCSR = _BV(WDIE) | _BV(WDP0) | _BV(WDP3);				// Set the Watchdog timer to interrupt (not reset) every 8 seconds

	TIME = 0;												// Reset the 10ms timer
	
	for(;;)
	{
		USB_USBTask();
		CDC_Host_Task();
		SendDataToAndFromModem();
		
		switch (ConnectedState)
		{
			case 0:
				Dial();
				break;
			case 1:
				DoPPP();
				break;
			case 2:
				Debug("Initialise TCP Stack\r\n");
			
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
				TIME_SET(2000);			// Make the first CONNECT happen straight away
				break;
			case 3:
				if (TIME > 1000)		//Try to connect every 1 second
				{
					TIME_SET(0);
					
					// Connect to the remote machine
					ThisConn = uip_connect(&RemoteIPAddress, HTONS(80));

					if (ThisConn != 0)
					{
						Debug("Connected to host\r\n");
						ConnectedState = 4;
						TIME_SET(3001);			// Make the first GET happen straight away
					}
					else
						Debug("Failed to Connect\r\n");

					Debug("Maximum Segment Size: 0x"); PrintHex(uip_mss() / 256);
					Debug("0x"); PrintHex(uip_mss() & 255); 
					Debug("\r\n");
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
	DebugChar('*');

	if (uip_newdata())
		Debug("NewData ");

	if (uip_acked())
		Debug("Acked ");
	
	if (uip_connected())
		Debug("Connected ");

	if (uip_closed())
	{
		Debug("Closed - Reconnecting...");
		_delay_ms(1000);
		ConnectedState = 3;
	}

	if (uip_aborted())
	{
		Debug("Aborted - Reconnecting... ");
		_delay_ms(1000);
		ConnectedState = 3;
	}

	if (uip_timedout())
	{
		Debug("Timeout - Reconnecting...");
		uip_abort();
		_delay_ms(1000);
		ConnectedState = 3;
	}

	if (uip_poll() && TIME > 3000)
	{
		TIME_SET(0);
		
		Debug("\r\nSending GET\r\n");
		SendGET();
	}
	
	if (uip_rexmit())
	{
		Debug("\r\nRetransmit GET\r\n");
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
		Debug("Got non-PPP packet\r\n");
		TIME_SET(0);
		ConnectedState = 0;
		return;
	}

	if (uip_len > 0)								// We have some data to process
	{
	
		/********************** Debug **********************/

		Debug("\r\nReceive:\r\n");
	
		for (i = 0; i < uip_len; i += 16)
		{	
			// Print the hex
			for (j = 0; j < 16; j++)
			{
				if ((i + j) >= uip_len)
					break;

				PrintHex(*(uip_buf + i + j));
			}
			
			Debug("\r\n");	
			
			// Print the ASCII
			for (j = 0; j < 16; j++)
			{
				if ((i + j) >= uip_len)
					break;

				if (*(uip_buf + i + j) >= 0x20 && *(uip_buf + i + j) <= 0x7e)
				{
					DebugChar(' ');
					DebugChar(*(uip_buf + i + j));
					DebugChar(' ');
				}
				else
					Debug(" . ");

			}
			Debug("\r\n");
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

	// Read any available data from the serial port.
	// If we see a '!' in the input stream, switch debug mode on. If we see a "@", switch debug mode off.
	int c = uart1_getc();
	
	while (c != UART_NO_DATA)
	{
		if (c == '!')
		{
			uart1_puts("\r\nDebug on\r\n");
			DebugMode = 1;
		}
		else if (c == '@')
		{
			uart1_puts("\r\nDebug off\r\n");
			DebugMode = 0;
		}

		c = uart1_getc();
	}	
}

void device_enqueue(char *x, int len)
{
	Debug("\r\nData:\r\n");

	for (int i = 0; i < len; i++)
	{
		WatchdogTicks = 0;							// Reset the watchdog count
		uart1_putc(*(x + i));
	}

	Debug("\r\n");
}

bool device_queue_full(void)						// TODO: Figure out how to return a proper value here
{
	return false;
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
	int c;

	if (USB_HostState == HOST_STATE_Configured)	
	{
		c = modem_getc();
		
		if (!(c & MODEM_NO_DATA))
		{
			do
			{
				DebugChar(c);
				c = modem_getc();
			}
			while (!(c & MODEM_NO_DATA));
		}
			
		if (TIME > 100)
		{
			TIME_SET(0);
			strcpy(Command, DialCommands[DialSteps++]);

			if (strcmp(Command, "PPP") == 0)
			{
				Debug("Starting PPP\r\n");
				DialSteps = 0;
				ConnectedState = 1;
				return;
			}

			Debug("Sending command: ");
			Debug(Command);
			
			modem_puts(Command);
		}
	}
}


// Event handler for the USB_DeviceAttached event. This indicates that a device has been attached to the host, and
// starts the library USB task to begin the enumeration and USB management process.
 
void EVENT_USB_Host_DeviceAttached(void)
{
	//puts_P(PSTR(ESC_FG_GREEN "Device Attached.\r\n" ESC_FG_WHITE));
	Debug("Device Attached\r\n");
	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

// Event handler for the USB_DeviceUnattached event. This indicates that a device has been removed from the host, and
//  stops the library USB task management process.
void EVENT_USB_Host_DeviceUnattached(void)
{
	//puts_P(PSTR(ESC_FG_GREEN "\r\nDevice Unattached.\r\n" ESC_FG_WHITE));
	Debug("Device Unattached\r\n");
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	ConnectedState = 0;
}

// Event handler for the USB_DeviceEnumerationComplete event. This indicates that a device has been successfully
//  enumerated by the host and is now ready to be used by the application.
void EVENT_USB_Host_DeviceEnumerationComplete(void)
{
	Debug("Enumeration complete\r\n");
	LEDs_SetAllLEDs(LEDMASK_USB_READY);
}

// Event handler for the USB_HostError event. This indicates that a hardware error occurred while in host mode. */
void EVENT_USB_Host_HostError(const uint8_t ErrorCode)
{
	USB_ShutDown();

	Debug("Host Mode Error\r\n");
	LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
	for(;;);
}

// Event handler for the USB_DeviceEnumerationFailed event. This indicates that a problem occurred while
// enumerating an attached USB device.
void EVENT_USB_Host_DeviceEnumerationFailed(const uint8_t ErrorCode, const uint8_t SubErrorCode)
{
	Debug("Enumeration failed\r\n");
	LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
}

// Task to set the configuration of the attached device after it has been enumerated, and to read in
// data received from the attached CDC device and print it to the serial port.
void CDC_Host_Task(void)
{
	uint8_t ErrorCode;

	switch (USB_HostState)
	{
		case HOST_STATE_WaitForDeviceRemoval:
			Debug("Waiting for device removal\r\n");

			// Wait until USB device disconnected
			while (USB_HostState == HOST_STATE_WaitForDeviceRemoval);
			break;
		case HOST_STATE_Addressed:
			Debug("Sending configuration command\r\n");

			// Standard request to set the device configuration to configuration 1
			// For the Huawei modem, this will cause the device to disconnect and change modes
			USB_ControlRequest = (USB_Request_Header_t)
				{
					.bmRequestType = (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_DEVICE),
					.bRequest      = REQ_SetConfiguration,
					.wValue        = 1,
					.wIndex        = 0,
					.wLength       = 0,
				};

			// Select the control pipe for the request transfer
			Pipe_SelectPipe(PIPE_CONTROLPIPE);

			// Send the request and display any error
			if ((ErrorCode = USB_Host_SendControlRequest(NULL)) != HOST_SENDCONTROL_Successful)
			{
				Debug("Control Error (Set Configuration).\r\n");
			}

			Debug("Looking for modem device...");
			
			// Get and process the configuration descriptor data
			// First time through we expect an error. Once the device has re-attached it should be OK
			if ((ErrorCode = ProcessConfigurationDescriptor()) != SuccessfulConfigRead)
			{
				if (ErrorCode == ControlError)
				  Debug("Control Error (Get Configuration).\r\n");
				else
				  Debug("Not a modem device\r\n");

				// Indicate error via status LEDs
				LEDs_SetAllLEDs(LEDMASK_USB_ERROR);

				// Wait until USB device disconnected
				USB_HostState = HOST_STATE_WaitForDeviceRemoval;
				break;
			}

			Debug("CDC Device Enumerated\r\n");

			USB_HostState = HOST_STATE_Configured;
			break;
	}
}

void SendDataToAndFromModem(void)
{
	uint8_t ErrorCode;
	uint8_t Buffer[MODEM_TX_BUFFER_SIZE];
	uint16_t BufferLength = 0;

	if (USB_HostState != HOST_STATE_Configured)
		return;

	////////////////////////////////
	// From Circular Buffer to Modem
	////////////////////////////////

	// Select the OUT data pipe for transmission
	Pipe_SelectPipe(CDC_DATAPIPE_OUT);
	Pipe_SetPipeToken(PIPE_TOKEN_OUT);
	Pipe_Unfreeze();

	if (!modem_TxBufferEmpty())
	{
		if (!(Pipe_IsReadWriteAllowed()))
		{
			Pipe_ClearOUT();

			if ((ErrorCode = Pipe_WaitUntilReady()) != PIPE_READYWAIT_NoError)
			{
				// Freeze pipe after use
				Pipe_Freeze();
		  		return;
			}
		}

		// Copy from the circular buffer to a temporary transmission buffer		
		BufferLength = modem_getTxBuffer(Buffer, (char)sizeof(Buffer));

		if ((ErrorCode = Pipe_Write_Stream_LE(Buffer, BufferLength)) != PIPE_RWSTREAM_NoError)
			Debug("Error writing Pipe\r\n");

		// Send the data in the OUT pipe to the attached device
		Pipe_ClearOUT();
	}

	// Freeze pipe after use
	Pipe_Freeze();

	
	////////////////////////////////
	// From Modem to Circular Buffer
	////////////////////////////////
	
	// Select the data IN pipe
	Pipe_SelectPipe(CDC_DATAPIPE_IN);
	Pipe_SetPipeToken(PIPE_TOKEN_IN);/////
	Pipe_Unfreeze();

	// Check if data is in the pipe
	if (Pipe_IsINReceived())
	{
			// Re-freeze IN pipe after the packet has been received
			Pipe_Freeze();

			// Check if data is in the pipe
			if (Pipe_IsReadWriteAllowed())
			{
				// Get the length of the pipe data, and create a new temporary buffer to hold it
				BufferLength = Pipe_BytesInPipe();

				if (BufferLength >= MODEM_RX_BUFFER_SIZE)
					BufferLength = MODEM_RX_BUFFER_SIZE - 1;

				uint8_t Buffer[BufferLength];
		
				// Read in the pipe data to the temporary buffer
				if ((ErrorCode = Pipe_Read_Stream_LE(Buffer, BufferLength)) != PIPE_RWSTREAM_NoError)
					Debug("Error reading Pipe\r\n");
		
				// Clear the pipe after it is read, ready for the next packet
				Pipe_ClearIN();

				// Copy the temporary buffer contents to the circular buffer
				modem_putRxBuffer(Buffer, BufferLength);
			}
	}
	
	// Re-freeze IN pipe after use
	Pipe_Freeze();		

	// Select and unfreeze the notification pipe
	Pipe_SelectPipe(CDC_NOTIFICATIONPIPE);
	Pipe_Unfreeze();
	
	// Check if data is in the pipe
	if (Pipe_IsINReceived())
	{
		// Discard the event notification
		Pipe_ClearIN();
	}
	
	// Freeze notification IN pipe after use
	Pipe_Freeze();
}

void DebugChar(char DebugText)
{
	if (DebugMode == 1)
		uart1_putc(DebugText);
}

void Debug(char *DebugText)
{
	if (DebugMode == 1)
		uart1_puts(DebugText);
}

void PrintHex(unsigned char c)
{
	if ((c >> 4) > 9)
		DebugChar((c >> 4) + 'a' - 10);
	else
		DebugChar((c >> 4) + '0');

	if ((c & 0x0f) > 9)
		DebugChar((c & 0x0f) + 'a' - 10);
	else
		DebugChar((c & 0x0f) + '0');

	DebugChar(' ');
}
