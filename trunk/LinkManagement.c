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

uint8_t			ConnectedState;
static uint8_t  DialSteps;

void LinkManagement_ManageConnectionState(void)
{
	switch (ConnectedState)
	{
		case LINKMANAGEMENT_STATE_Idle:
			SystemTicks = 0;
			DialSteps = 0;
			PPP_InitPPP();
			ConnectedState = LINKMANAGEMENT_STATE_SendModemDialCommands;
		break;

		case LINKMANAGEMENT_STATE_SendModemDialCommands:
			if (LinkManagement_DialConnection(ModemDialCommands))
				ConnectedState = LINKMANAGEMENT_STATE_SendNetworkDialCommands;
		break;
		
		case LINKMANAGEMENT_STATE_SendNetworkDialCommands:
			if (LinkManagement_DialConnection(NetworkDialCommands))
			{
				Debug_Print("Starting PPP\r\n");
				PPP_StartLink();
				ConnectedState = LINKMANAGEMENT_STATE_WaitForLink;
			}	
		break;

		case LINKMANAGEMENT_STATE_WaitForLink:
			PPP_ManageLink();
		break;
		
		case LINKMANAGEMENT_STATE_InitializeTCPStack:
			PPP_ManageLink();
			TCPIP_InitializeTCPStack();
		break;
		
		case LINKMANAGEMENT_STATE_ConnectToRemoteHost:
			PPP_ManageLink();
			TCPIP_ConnectToRemoteHost();
		break;
		
		case LINKMANAGEMENT_STATE_ManageTCPConnection:
			PPP_ManageLink();
			TCPIP_TCPIPTask();
		break;
	}
}

// Read the dial commands and send them to the modem. Wait for the expected response then move on to the next command.
// Returns true if the last dial command has been processed.
static bool LinkManagement_DialConnection(const char** DialCommands)
{
	static char* ResponsePtr = NULL;
	char* CommandPtr = NULL;
	static uint8_t CharsMatched = 0;
	char c;

	if (USB_HostState != HOST_STATE_Configured)	
		return false;

	while (!RingBuffer_IsEmpty(&Modem_ReceiveBuffer))						    // Read back the response
	{	
		c = RingBuffer_Remove(&Modem_ReceiveBuffer);
		Debug_PrintChar(c);
		
		if (c == *(ResponsePtr + CharsMatched))									// Match the response character by character with the expected response						
			CharsMatched++;
		else
			CharsMatched = 0;
			
		if (CharsMatched != 0 && CharsMatched == strlen(ResponsePtr))			// Look for the expected response
		{
			DialSteps += 2;														// Move on to the next dial command
			CharsMatched = 0;
		}
	}	
		
	if (SystemTicks > 100)														// Space each command by 1 second
	{
		CommandPtr = (char*)DialCommands[DialSteps];
		ResponsePtr = (char*)DialCommands[DialSteps + 1];

		if (CommandPtr == NULL || ResponsePtr == NULL)							// No more dial commands
		{
			DialSteps = 0;
			return true;														// Finished dialling
		}

		SystemTicks = 0;

		Debug_Print("Send: "); Debug_Print(CommandPtr);
		Debug_Print("(Expect: "); Debug_Print(ResponsePtr); Debug_Print(")\r\n");
				
		while (*CommandPtr)														
			RingBuffer_Insert(&Modem_SendBuffer, *(CommandPtr++));			// Send the command	to the modem
	}

	return false;																// Haven't finished dialling
}
