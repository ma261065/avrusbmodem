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

#define INCLUDE_FROM_USBMANAGEMENT_C
#include "USBManagement.h"

RingBuff_t Modem_SendBuffer;
RingBuff_t Modem_ReceiveBuffer;


// Event handler for the USB_DeviceAttached event. This indicates that a device has been attached to the host, and
// starts the library USB task to begin the enumeration and USB management process.
void EVENT_USB_Host_DeviceAttached(void)
{
	Debug_Print("Device attached\r\n");
	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

// Event handler for the USB_DeviceUnattached event. This indicates that a device has been removed from the host, and
//  stops the library USB task management process.
void EVENT_USB_Host_DeviceUnattached(void)
{
	Debug_Print("Device unattached\r\n");
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	ConnectedState = 0;
}

// Event handler for the USB_DeviceEnumerationComplete event. This indicates that a device has been successfully
//  enumerated by the host and is now ready to be used by the application.
void EVENT_USB_Host_DeviceEnumerationComplete(void)
{
	Debug_Print("Enumeration complete\r\n");
	LEDs_SetAllLEDs(LEDMASK_USB_READY);
}

// Event handler for the USB_HostError event. This indicates that a hardware error occurred while in host mode.
void EVENT_USB_Host_HostError(const uint8_t ErrorCode)
{
	USB_ShutDown();

	Debug_Print("Host Mode error\r\n");
    Debug_PrintHex(ErrorCode);
	LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
	for(;;);
}

// Event handler for the USB_DeviceEnumerationFailed event. This indicates that a problem occurred while
// enumerating an attached USB device.
void EVENT_USB_Host_DeviceEnumerationFailed(const uint8_t ErrorCode,
                                            const uint8_t SubErrorCode)
{
	Debug_Print("Enumeration failed\r\n");
	Debug_PrintHex(ErrorCode);
    Debug_PrintHex(SubErrorCode);
	LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
}

// Task to set the configuration of the attached device after it has been enumerated, and to read in
// data received from the attached CDC device and print it to the serial port.
void USBManagement_ManageUSBState(void)
{
	uint8_t ErrorCode;

	switch (USB_HostState)
	{
		case HOST_STATE_WaitForDeviceRemoval:
			Debug_Print("Wait for device removal\r\n");

			// Wait until USB device disconnected
			while (USB_HostState == HOST_STATE_WaitForDeviceRemoval);
		break;
		
		case HOST_STATE_Addressed:
			Debug_Print("Send configuration command\r\n");
			
			SwitchModemMode();
					
			Debug_Print("Looking for modem device...");
			
			// Get and process the configuration descriptor data
			// First time through we expect an error. Once the device has re-attached it should be OK
			if ((ErrorCode = ProcessConfigurationDescriptor()) != SuccessfulConfigRead)
			{
				if (ErrorCode == ControlError)
				{
					Debug_Print("Control error (Get Configuration).\r\n");
					Debug_PrintHex(ErrorCode);
				}
				else
				{
					Debug_Print("Not a modem device - switching modes\r\n");
					SwitchModemMode();
				}

				// Indicate error via status LEDs
				LEDs_SetAllLEDs(LEDMASK_USB_ERROR);

				// Wait until USB device disconnected
				USB_HostState = HOST_STATE_WaitForDeviceRemoval;
				break;
			}

			Debug_Print("Modem device enumerated\r\n");

			USB_HostState = HOST_STATE_Configured;
		break;
		
		case HOST_STATE_Configured:
			USBManagement_SendReceivePipes();
		break;
	}
}

void USBManagement_SendReceivePipes(void)
{
	uint8_t ErrorCode;

	if (USB_HostState != HOST_STATE_Configured)
		return;

	////////////////////////////////
	// From Circular Buffer to Modem
	////////////////////////////////

	// Select the OUT data pipe for transmission
	Pipe_SelectPipe(CDC_DATAPIPE_OUT);
	Pipe_Unfreeze();

	while (Modem_SendBuffer.Elements) 
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

		Pipe_Write_Byte(Buffer_GetElement(&Modem_SendBuffer)); 
	} 
	  
	// Send remaining data in pipe bank 
	if (Pipe_BytesInPipe()) 
		Pipe_ClearOUT(); 
 
	// Freeze pipe after use
	Pipe_Freeze();

	
	////////////////////////////////
	// From Modem to Circular Buffer
	////////////////////////////////
	
	// Select the data IN pipe
	Pipe_SelectPipe(CDC_DATAPIPE_IN);
	Pipe_Unfreeze();

	// Check if data is in the pipe and space is available in the receive buffer
	if (Pipe_IsINReceived())
	{
		if ((Modem_ReceiveBuffer.Elements < (BUFF_STATICSIZE - 64)))
		{
			// Check if data is in the pipe
			if (Pipe_IsReadWriteAllowed())
			{
				while (Pipe_BytesInPipe())
				  Buffer_StoreElement(&Modem_ReceiveBuffer, Pipe_Read_Byte());
			}

			// Clear the pipe after it is read, ready for the next packet
			Pipe_ClearIN();
		}
		else
		{
			Debug_Print("Overflow");
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
