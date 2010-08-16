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

#define  INCLUDE_FROM_LINKMANAGEMENT_C
#include "LinkManagement.h"

uint8_t      IPAddr1, IPAddr2, IPAddr3, IPAddr4;
uint8_t      ConnectedState = LINKMANAGEMENT_STATE_DialConnectionStage1;
uint8_t      DialSteps = 0;
struct timer Periodic_Timer;

extern const char* ModemDialCommands[];
extern const char* NetworkDialCommands[];

void LinkManagement_ManageConnectionState(void)
{
	switch (ConnectedState)
	{
		case LINKMANAGEMENT_STATE_DialConnectionStage1:
			LinkManagement_DialConnectionStage1();
		break;
		
		case LINKMANAGEMENT_STATE_DialConnectionStage2:
			LinkManagement_DialConnectionStage2();
		break;

		case LINKMANAGEMENT_STATE_DoPPPNegotiation:
			PPP_ManagePPPNegotiation();
		break;
		
		case LINKMANAGEMENT_STATE_InitializeTCPStack:
			LinkManagement_InitializeTCPStack();
		break;
		
		case LINKMANAGEMENT_STATE_ConnectToRemoteHost:
			LinkManagement_ConnectToRemoteHost();
		break;
		
		case LINKMANAGEMENT_STATE_ManageTCPConnection:
			LinkManagement_TCPIPTask();
		break;
	}
}

static void LinkManagement_DialConnectionStage1(void)
{
	if (USB_HostState != HOST_STATE_Configured)	
		return;

	while (Modem_ReceiveBuffer.Elements)
		Debug_PrintChar(Buffer_GetElement(&Modem_ReceiveBuffer));
		
	if (TIME > 100)
	{
		TIME = 0;

		char* CommandPtr = (char*)ModemDialCommands[DialSteps++];

		if (CommandPtr == NULL)
		{
			DialSteps = 0;
			ConnectedState = LINKMANAGEMENT_STATE_DialConnectionStage2;
			return;
		}

		Debug_Print("Sending command: ");
		Debug_Print(CommandPtr);
		
		while (*CommandPtr)
			Buffer_StoreElement(&Modem_SendBuffer, *(CommandPtr++));
	}
}

static void LinkManagement_DialConnectionStage2(void)
{
	if (USB_HostState != HOST_STATE_Configured)	
		return;

	while (Modem_ReceiveBuffer.Elements)
		Debug_PrintChar(Buffer_GetElement(&Modem_ReceiveBuffer));
		
	if (TIME > 100)
	{
		TIME = 0;

		char* CommandPtr = (char*)NetworkDialCommands[DialSteps++];

		if (CommandPtr == NULL)
		{
			Debug_Print("Starting PPP\r\n");
			DialSteps = 0;
			ConnectedState = LINKMANAGEMENT_STATE_DoPPPNegotiation;
			return;
		}

		Debug_Print("Sending command: ");
		Debug_Print(CommandPtr);
		
		while (*CommandPtr)
			Buffer_StoreElement(&Modem_SendBuffer, *(CommandPtr++));
	}
}

static void LinkManagement_InitializeTCPStack(void)
{
	Debug_Print("Initialise TCP Stack\r\n");

	// uIP Initialization
	network_init();
	clock_init();
	uip_init();

	// Periodic Connection Timer Initialization
	timer_set(&Periodic_Timer, CLOCK_SECOND / 2);

	// Set this machine's IP address
	uip_ipaddr_t LocalIPAddress;
	uip_ipaddr(&LocalIPAddress, IPAddr1, IPAddr2, IPAddr3, IPAddr4);
	uip_sethostaddr(&LocalIPAddress);

	ConnectedState = LINKMANAGEMENT_STATE_ConnectToRemoteHost;
	TIME = 2000;			// Make the first CONNECT happen straight away
}

static void LinkManagement_ConnectToRemoteHost(void)
{
	if (TIME > 1000)		// Try to connect every 1 second
	{
		TIME = 0;
		
		if (HTTPClient_Connect())
		{
			TIME = 3001;	// Make the first GET happen straight away
			ConnectedState = LINKMANAGEMENT_STATE_ManageTCPConnection;
		}
	}
}

static void LinkManagement_TCPIPTask(void)
{
	uip_len = network_read();

	if (uip_len == -1)								// Got a non-SLIP packet. Probably a LCP-TERM Re-establish link.
	{
		Debug_Print("Got non-PPP packet\r\n");
		TIME = 0;
		ConnectedState = LINKMANAGEMENT_STATE_DialConnectionStage1;
		return;
	}

	if (uip_len > 0)								// We have some data to process
	{
		Debug_Print("\r\nReceive:\r\n");

		for (uint16_t i = 0; i < uip_len; i += 16)
		{	
			// Print the hex
			for (uint8_t j = 0; j < 16; j++)
			{
				if ((i + j) >= uip_len)
				  break;

				Debug_PrintHex(*(uip_buf + i + j));
			}
			
			Debug_Print("\r\n");	
			
			// Print the ASCII
			for (uint8_t j = 0; j < 16; j++)
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
				{
					Debug_Print(" . ");
				}
			}

			Debug_Print("\r\n");
		}

		uip_input();
 
	 	// If the above function invocation resulted in data that should be sent out on the network, the global variable uip_len is set to a value > 0.
	 	if (uip_len > 0)
	 	  network_send();
	}
	else if (timer_expired(&Periodic_Timer))
	{
		timer_reset(&Periodic_Timer);

		for (uint8_t i = 0; i < UIP_CONNS; i++)
		{
	 		uip_periodic(i);
 
	 		// If the above function invocation resulted in data that should be sent out on the network, the global variable uip_len is set to a value > 0.
	 		if (uip_len > 0)
				network_send();
		}
	}
}
