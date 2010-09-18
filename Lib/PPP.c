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

#define  INCLUDE_FROM_PPP_C
#include "PPP.h"

static uint8_t  	OutgoingPacketID;								// Unique packet ID

PPP_Phases_t 		PPP_Phase = PPP_PHASE_Dead;						// PPP negotiation phases

PPP_States_t		LCP_State = PPP_STATE_Initial;					// Each phase has a number of states
PPP_States_t		PAP_State = PPP_STATE_Initial;					// Each phase has a number of states
PPP_States_t		IPCP_State = PPP_STATE_Initial;					// Each phase has a number of states

uint8_t 			PacketBuff[64];
PPP_Packet_t*  		OutgoingPacket = (PPP_Packet_t*)PacketBuff;		// The outgoing packet
PPP_Packet_t*  		IncomingPacket;									// The incoming packet
uint16_t     		CurrentProtocol = NONE;							// Type of the last received packet

uint8_t 			RestartCount = MAX_RESTARTS;
static uint16_t		LinkTimer = 0;
bool				TimerOn = false;

bool 				MarkForNAK, MarkForREJ;


void PPP_InitPPP(void)
{
	PPP_Phase = PPP_PHASE_Dead;
}

void PPP_LinkUp()
{
	PPP_Phase = PPP_PHASE_Establish;
	PPP_ManageState(PPP_EVENT_Up, &LCP_State);
}

void PPP_LinkOpen()
{
	PPP_ManageState(PPP_EVENT_Open, &LCP_State);
	PPP_ManageState(PPP_EVENT_Open, &PAP_State);
	PPP_ManageState(PPP_EVENT_Open, &IPCP_State);
}

// Called every 10ms. Send events to the state machine every 3 seconds if currently running
void PPP_LinkTimer(void)
{
	if (!TimerOn || LinkTimer++ < 300)
		return;

	if (RestartCount > 0)
	{
		Debug_Print("Timer+\r\n");

		switch(PPP_Phase)
		{
			case PPP_PHASE_Establish:
				PPP_ManageState(PPP_EVENT_TOPlus, &LCP_State);
			break;

			case PPP_PHASE_Authenticate:
				PPP_ManageState(PPP_EVENT_TOPlus, &PAP_State);
			break;

			case PPP_PHASE_Network:
				PPP_ManageState(PPP_EVENT_TOPlus, &IPCP_State);
			break;

			default:
			break;
		}
	}
	else
	{
		Debug_Print("Timer-\r\n");
		
		switch(PPP_Phase)
		{
			case PPP_PHASE_Establish:
				PPP_ManageState(PPP_EVENT_TOMinus, &LCP_State);
			break;

			case PPP_PHASE_Authenticate:
				PPP_ManageState(PPP_EVENT_TOMinus, &PAP_State);
			break;

			case PPP_PHASE_Network:
				PPP_ManageState(PPP_EVENT_TOMinus, &IPCP_State);
			break;

			default:
			break;
		}
	}

	LinkTimer = 0;																										// Reset Timer
}

void PPP_ManageLink(void)
{	
	if (PPP_Phase == PPP_PHASE_Dead)
		return;
	
	CurrentProtocol = network_read();

	if (CurrentProtocol == 0)
		return;
	
	Debug_Print("Got ");
	IncomingPacket = (PPP_Packet_t*)uip_buf;																			// Map the incoming data to a packet
	
	switch (CurrentProtocol)
	{
		case LCP:
			Debug_Print("LCP ");

			switch(IncomingPacket->Code)
			{
				case REQ:
					Debug_Print("REQ\r\n");

					MarkForNAK = MarkForREJ = false;

					// List of options that we can support. If any other options come in, we have to REJ them
					uint8_t SupportedOptions[] = {LCP_OPTION_Async_Control_Character_Map,
									  		 	  LCP_OPTION_Authentication_Protocol,
									  		 	  LCP_OPTION_Protocol_Field_Compression,
									  		 	  LCP_OPTION_Address_and_Control_Field_Compression};

					if ((MarkForREJ = PPP_TestForREJ(SupportedOptions, sizeof(SupportedOptions))))						// Check that we can support all the options the other end wants to use
					{
						PPP_ManageState(PPP_EVENT_RCRMinus, &LCP_State);
						break;
					}
					
					static PPP_Option_t Option3 = {.Type  = 0x03, .Length = 4, .Data = {0xc0, 0x23}};				

					if ((MarkForNAK = PPP_TestForNAK(&Option3)))														// Check that the authentication protocol = PAP (0xc023)
					{
						PPP_ManageState(PPP_EVENT_RCRMinus, &LCP_State);
						break;
					}

					PPP_ManageState(PPP_EVENT_RCRPlus, &LCP_State);
				break;

				case ACK:
					Debug_Print("ACK\r\n");
					PPP_ManageState(PPP_EVENT_RCA, &LCP_State);
				break;

				case NAK:
					Debug_Print("NAK\r\n");
					ProcessNAK();
					PPP_ManageState(PPP_EVENT_RCN, &LCP_State);
				break;

				case REJ:
					Debug_Print("REJ\r\n");
					ProcessREJ();
					PPP_ManageState(PPP_EVENT_RCN, &LCP_State);
				break;

				case DISC:
				case ECHOREQ:
				case ECHOREPLY:
					Debug_Print("DISC\r\n");
					PPP_ManageState(PPP_EVENT_RXR, &LCP_State);
				break;

				case TERMREQ:
					Debug_Print("TERM\r\n");
					PPP_ManageState(PPP_EVENT_RTR, &LCP_State);
				break;

				case CODEREJ:
				case PROTREJ:
					Debug_Print("CODE/PROTREJ\r\n");
					PPP_ManageState(PPP_EVENT_RXJMinus, &LCP_State);
				break;
				
				default:
					Debug_Print("unknown\r\n");
					PPP_ManageState(PPP_EVENT_RUC, &LCP_State);
				break;
			}
		break;

		case PAP:
			Debug_Print("PAP ");
			
			switch(IncomingPacket->Code)
			{
				case REQ:
					Debug_Print("REQ\r\n");
					PPP_ManageState(PPP_EVENT_RCRPlus, &PAP_State);
				break;

				case ACK:
					Debug_Print("ACK\r\n");
					PPP_ManageState(PPP_EVENT_RCRPlus, &PAP_State);
					PPP_ManageState(PPP_EVENT_RCA, &PAP_State);
				break;
			}
		break;

		case IPCP:
			Debug_Print("IPCP ");

			switch(IncomingPacket->Code)
			{
				case REQ:
					Debug_Print("REQ\r\n");
	
					MarkForNAK = MarkForREJ = false;
					
					// List of options that we can support. If any other options come in, we have to REJ them
					uint8_t SupportedOptions[] = {IPCP_OPTION_IP_address, IPCP_OPTION_Primary_DNS, IPCP_OPTION_Secondary_DNS};
					
					if ((MarkForREJ = PPP_TestForREJ(SupportedOptions, sizeof(SupportedOptions))))
					{
						PPP_ManageState(PPP_EVENT_RCRMinus, &IPCP_State);
						break;
					}

					PPP_ManageState(PPP_EVENT_RCRPlus, &IPCP_State);
				break;

				case ACK:
					Debug_Print("ACK\r\n");
					IPAddr1 = IncomingPacket->Options[0].Data[0]; 						// Store address for use in IP packets.
					IPAddr2 = IncomingPacket->Options[0].Data[1];    				 	// Assumption is that Option 3 is the first option, which it
					IPAddr3 = IncomingPacket->Options[0].Data[2];						// should be as the PPP spec states that implementations should
					IPAddr4 = IncomingPacket->Options[0].Data[3];						// not reorder packets, and we sent out a REQ with option 3 first

					PPP_ManageState(PPP_EVENT_RCA, &IPCP_State);
				break;

				case NAK:
					Debug_Print("NAK\r\n");
					ProcessNAK();
					PPP_ManageState(PPP_EVENT_RCN, &IPCP_State);
				break;

				case REJ:
					Debug_Print("REJ\r\n");
					ProcessREJ();
					PPP_ManageState(PPP_EVENT_RCN, &IPCP_State);
				break;
			}
		break;

		case IP:
			TCPIP_GotNewPacket();
		break;

		default:
			Debug_Print("unknown protocol: 0x");
			Debug_PrintHex(CurrentProtocol / 256); Debug_PrintHex(CurrentProtocol & 255);
			Debug_Print("\r\n");
		break;
	}
}


// Either create a new OutgoingPacket, or if we've just received a NAK or REJ we will have already changed the OutgoingPacket so just send that
static void Send_Configure_Request()
{
	Debug_Print("Send Configure Request\r\n");
	
	switch(PPP_Phase)
	{
		case PPP_PHASE_Establish:
			if (CurrentProtocol == NONE)																					// Create a new packet
			{
				// When we send a REQ, we want to make sure that the other end supports these options
				static PPP_Option_t Option1 = {.Type  = LCP_OPTION_Maximum_Receive_Unit, .Length = 4, .Data = {0x5, 0xa0}};
				static PPP_Option_t Option2 = {.Type  = LCP_OPTION_Async_Control_Character_Map, .Length = 6, .Data = {0x0, 0xa, 0x0, 0x0}};
				static PPP_Option_t Option7 = {.Type  = LCP_OPTION_Protocol_Field_Compression, .Length = 2};
				static PPP_Option_t Option8 = {.Type  = LCP_OPTION_Address_and_Control_Field_Compression, .Length = 2};
				
				CurrentProtocol = LCP;
				OutgoingPacket->Code = REQ;
				OutgoingPacket->Length = htons(4);

				AddOption(OutgoingPacket, &Option1);
				AddOption(OutgoingPacket, &Option2);
				AddOption(OutgoingPacket, &Option7);
				AddOption(OutgoingPacket, &Option8);
			}
		break;

		case PPP_PHASE_Authenticate:
			if (CurrentProtocol == NONE)																					// Create a new packet
			{
				CurrentProtocol = PAP;
				OutgoingPacket->Code = REQ;
				OutgoingPacket->Options[0].Type = 0x00;																		// No User Name
				OutgoingPacket->Options[0].Length = 0x00;																	// No Password
				OutgoingPacket->Length = htons(6);
			}
		break;

		case PPP_PHASE_Network:
			if (CurrentProtocol == NONE)																					// Create a new packet
			{
				// When we send a REQ, we want to make sure that the other end supports these options
				static PPP_Option_t Option3 = {.Type  = IPCP_OPTION_IP_address, .Length = 6, .Data = {0, 0, 0, 0}};			// Make sure this is first
				static PPP_Option_t Option81 = {.Type  = IPCP_OPTION_Primary_DNS, .Length = 6, .Data = {0, 0, 0, 0}};
				static PPP_Option_t Option83 = {.Type  = IPCP_OPTION_Secondary_DNS, .Length = 6, .Data = {0, 0, 0, 0}};
				
				CurrentProtocol = IPCP;
				OutgoingPacket->Code = REQ;
				OutgoingPacket->Length = htons(4);

				AddOption(OutgoingPacket, &Option3);
				AddOption(OutgoingPacket, &Option81);
				AddOption(OutgoingPacket, &Option83);
			}
		break;

		default:
		break;
	}

	OutgoingPacket->PacketID = OutgoingPacketID++;																// Every new REQ Packet going out gets a new ID
	RestartCount--;
	memcpy(uip_buf, OutgoingPacket, ntohs(OutgoingPacket->Length));												// Copy the outgoing packet to the buffer for sending
	uip_len = ntohs(OutgoingPacket->Length);
	network_send(CurrentProtocol);																				// Send either the new packet or the modified packet
}


// We change the incoming packet code to send an ACK to the remote end, and re-use all the data from the incoming packet
static void Send_Configure_Ack()
{
	Debug_Print("Send Configure ACK\r\n");

	IncomingPacket->Code = ACK;
	network_send(CurrentProtocol);
}

// We change the incoming packet code to send a NAK or REJ to the remote end. The incoming packet has already been altered to show which options to NAK/REJ
static void Send_Configure_Nak_Rej()
{
	if (MarkForNAK)
	{
		Debug_Print("Send Configure NAK\r\n");
		IncomingPacket->Code = NAK;
	}
	else if (MarkForREJ)
	{
		Debug_Print("Send Configure REJ\r\n");
		IncomingPacket->Code = REJ;
	}
	else return;

	uip_len = ntohs(IncomingPacket->Length);
	network_send(CurrentProtocol);
}

// Send a TERM to the remote end.
static void Send_Terminate_Request()
{
	Debug_Print("Send Terminate Request\r\n");
	
	CurrentProtocol = LCP;
	OutgoingPacket->Code = TERMREQ;
	OutgoingPacket->Length = htons(4);
	OutgoingPacket->PacketID = OutgoingPacketID++;																// Every new REQ Packet going out gets a new ID

	RestartCount--;

	memcpy(uip_buf, OutgoingPacket, ntohs(OutgoingPacket->Length));												// Copy the outgoing packet to the buffer for sending

	uip_len = ntohs(OutgoingPacket->Length);
	network_send(CurrentProtocol);																				// Send the packet
}

// Send a TERM ACK to the remote end.
static void Send_Terminate_Ack()
{
	Debug_Print("Send Terminate ACK\r\n");

	IncomingPacket->Code = TERMREPLY;
	network_send(CurrentProtocol);
}

// Send a REJ to the remote end.
static void Send_Code_Reject()
{
	Debug_Print("Send Code Reject\r\n");

	IncomingPacket->Code = CODEREJ;
	network_send(CurrentProtocol);
}

// Send an ECHO to the remote end.
static void Send_Echo_Reply()
{
	Debug_Print("Send Echo Reply\r\n");

	IncomingPacket->Code = ECHOREPLY;
	network_send(CurrentProtocol);
}

// Called by the state machine when the current layer comes up. Use this to start the next layer.
static void This_Layer_Up()
{
	switch(PPP_Phase)
	{
		case PPP_PHASE_Establish:
			Debug_Print("**LCP Up**\r\n");
			PPP_Phase = PPP_PHASE_Authenticate;
			CurrentProtocol = NONE;
			PPP_ManageState(PPP_EVENT_Up, &PAP_State);
		break;

		case PPP_PHASE_Authenticate:
			Debug_Print("**PAP Up**\r\n");
			PPP_Phase = PPP_PHASE_Network;
			CurrentProtocol = NONE;
			PPP_ManageState(PPP_EVENT_Up, &IPCP_State);
		break;

		case PPP_PHASE_Network:
			Debug_Print("**LINK CONNECTED**\r\n");
			CurrentProtocol = NONE;
			ConnectedState = LINKMANAGEMENT_STATE_InitializeTCPStack;
		break;

		default:
		break;
	}
}

// Called by the state machine when the current layer goes down. Reset everything
static void This_Layer_Down()
{
	Debug_Print("**Layer Down**\r\n");

	PPP_ManageState(PPP_EVENT_Down, &LCP_State);
	PPP_ManageState(PPP_EVENT_Down, &PAP_State);
	PPP_ManageState(PPP_EVENT_Down, &IPCP_State);
	PPP_Phase = PPP_PHASE_Dead;
	ConnectedState = LINKMANAGEMENT_STATE_Idle;
}

// Called by the state machine when the current is in negotiation
static void This_Layer_Started()
{
	Debug_Print("**Layer Started**\r\n");
}

// Called by the state machine when the current layer is closing
static void This_Layer_Finished()
{
	Debug_Print("**Layer Finished**\r\n");
}


// We get a NAK if our outbound Configure Request contains valid options, but the values are wrong. So we adjust our values for when the next Configure Request is sent
void ProcessNAK()
{
	PPP_Option_t* CurrentOption = NULL;

	while ((CurrentOption = GetNextOption(IncomingPacket, CurrentOption)) != NULL)		// Scan options in NAK packet
	{
		ChangeOption(OutgoingPacket, CurrentOption);
	}
}

// We get a REJ if our outbound Configure Request contains any options not acceptable to the remote end. So we remove those options.
void ProcessREJ()
{
	PPP_Option_t* CurrentOption = NULL;

	while ((CurrentOption = GetNextOption(IncomingPacket, CurrentOption)) != NULL)		// Scan options in REJ packet
	{
		RemoveOption(OutgoingPacket, CurrentOption->Type);
	}
}

// Test to see if the incoming packet contains any options we can't support If so, take out all good options and leave the bad ones to be sent out in the REJ
bool PPP_TestForREJ(uint8_t Options[], uint8_t NumOptions)
{
	PPP_Option_t* CurrentOption = NULL;
	bool FoundBadOption = false;
	bool ThisOptionOK;

	while ((CurrentOption = GetNextOption(IncomingPacket, CurrentOption)) != NULL)		// Scan incoming options
	{
		ThisOptionOK = false;

		for (int i = 0; i < NumOptions; i++)
		{
			if (CurrentOption->Type == Options[i])
			{
				ThisOptionOK = true;
				break;
			}
		}

		if (!ThisOptionOK)
			FoundBadOption = true;
	}
	
	if (!FoundBadOption)																// No bad options. Return, leaving the packet untouched
		return false;

	// We found some bad options, so now we need to go through the packet and remove all others, leaving the bad options to be sent out in the REJ
	while ((CurrentOption = GetNextOption(IncomingPacket, CurrentOption)) != NULL)
	{
		for (int i = 0; i < NumOptions; i++)
		{
			if (CurrentOption->Type == Options[i])
			{
				RemoveOption(IncomingPacket, CurrentOption->Type);
				CurrentOption = NULL;													// Start again. Easier than moving back (as next option has now moved into place of current option)
				break;
			}
		}
	}

	return true;
}

// Test to see if the incoming packet contains an option with values that we can't accept
bool PPP_TestForNAK(PPP_Option_t* Option)
{
	PPP_Option_t* CurrentOption = NULL;
	bool FoundBadOption = false;
	
	while ((CurrentOption = GetNextOption(IncomingPacket, CurrentOption)) != NULL)		// Scan options in receiver buffer
	{
		if (CurrentOption->Type == Option->Type)
		{	
			for (uint8_t i = 0; i < Option->Length - 2; i++)
			{
				if (CurrentOption->Data[i] != Option->Data[i])
					FoundBadOption = true;
			}
		}
	}
	
	if (!FoundBadOption)																// No bad option. Return, leaving the packet untouched
		return false;

	// We found a bad option, so now we need to go through the packet and remove all others, leaving the bad option to be sent out in the NAK
	// and change the bad option to have a value that we can support
	while ((CurrentOption = GetNextOption(IncomingPacket, CurrentOption)) != NULL)
	{
		if (CurrentOption->Type == Option->Type)
			ChangeOption(IncomingPacket, Option);
		else
		{
			RemoveOption(IncomingPacket, CurrentOption->Type);
			CurrentOption = NULL;														// Start again. Easier than moving back (as next option has now moved into place of current option)
		}
	}

	return true;
}


/////////////////////////////
// Packet Helper functions //
/////////////////////////////

// Try and find the option in the packet, and if it exists, change its value to that in the passed-in option
void ChangeOption(PPP_Packet_t* ThisPacket, PPP_Option_t* Option)
{
	PPP_Option_t* CurrentOption = NULL;

	while ((CurrentOption = GetNextOption(ThisPacket, CurrentOption)) != NULL)						// Scan options in the packet
	{
		if (CurrentOption->Type == Option->Type)
		{
			memcpy(CurrentOption->Data, Option->Data, Option->Length - 2);
		}
	}
}

// Add the given option to the end of the packet and adjust the size of the packet
void AddOption(PPP_Packet_t* ThisPacket, PPP_Option_t* Option)
{
	memcpy((void*)ThisPacket + ntohs(ThisPacket->Length), Option, Option->Length);
	ThisPacket->Length = htons(ntohs(ThisPacket->Length) + Option->Length);
}

// Try and find the option in the packet, and if it exists remove it and adjust the size of the packet
void RemoveOption(PPP_Packet_t* ThisPacket, uint8_t Type)
{
	PPP_Option_t* CurrentOption = NULL;
	PPP_Option_t* NextOption = NULL;

	while ((CurrentOption = GetNextOption(ThisPacket, CurrentOption)) != NULL)						// Scan the options in the packet
	{
		if (CurrentOption->Type == Type)															// Is it the packet we want to remove?
		{
			NextOption = GetNextOption(ThisPacket, CurrentOption);									// Find the next option in the packet
			uint8_t OptionLength = CurrentOption->Length;											// Save the Option Length as the memcpy will change CurrentOption->Length

			if (NextOption != NULL)																	// If it's not the last option in the packet ...
				memcpy(CurrentOption, NextOption, ntohs(ThisPacket->Length) - OptionLength - 4);	// ... move all further options forward

			ThisPacket->Length = htons(ntohs(ThisPacket->Length) - OptionLength);					// Adjust the length
		}
	}
}

// Get the next option in the packet from the option that is passed in. Return NULL if last packet
PPP_Option_t* GetNextOption(PPP_Packet_t* ThisPacket, PPP_Option_t* CurrentOption)
{
	PPP_Option_t* NextOption;
	
	if (CurrentOption == NULL)
		NextOption = (PPP_Option_t*)ThisPacket->Options;
	else
		NextOption = (PPP_Option_t*)((uint8_t*)CurrentOption + CurrentOption->Length);

	// Check that we haven't overrun the end of the packet
	if ((void*)NextOption - (void*)ThisPacket->Options < ntohs(ThisPacket->Length) - 4)
		return NextOption;
	else
		return NULL;
}



/////////////////////////////////////////////////////////////////////////////////////////
// The main PPP state machine - following RFC1661 - http://tools.ietf.org/html/rfc1661 //
/////////////////////////////////////////////////////////////////////////////////////////

void PPP_ManageState(PPP_Events_t Event, PPP_States_t* State)
{
	switch (*State)
	{
		case PPP_STATE_Initial:

			switch (Event)
			{
				case PPP_EVENT_Up:
					*State = PPP_STATE_Closed;
				break;

				case PPP_EVENT_Open:
					*State = PPP_STATE_Starting;
					This_Layer_Started();
				break;

				case PPP_EVENT_Close:
					*State = PPP_STATE_Initial;
				break;

				case PPP_EVENT_TOPlus:
				case PPP_EVENT_TOMinus:
				break;

				default:
					Debug_Print("Illegal Event\r\n");
				break;
			}
		break;

		case PPP_STATE_Starting:

			switch (Event)
			{
				case PPP_EVENT_Up:
					*State = PPP_STATE_Req_Sent;
					RestartCount = MAX_RESTARTS;
					LinkTimer = 0;
					TimerOn = true;
					Send_Configure_Request();
				break;

				case PPP_EVENT_Open:
					*State = PPP_STATE_Starting;
				break;

				case PPP_EVENT_Close:
					*State = PPP_STATE_Initial;
					This_Layer_Finished();
				break;

				case PPP_EVENT_TOPlus:
				case PPP_EVENT_TOMinus:
				break;

				default:
					Debug_Print("Illegal Event\r\n");
				break;
			}
		break;

		case PPP_STATE_Closed:

			switch (Event)
			{
				case PPP_EVENT_Down:
					*State = PPP_STATE_Initial;
				break;

				case PPP_EVENT_Open:
					*State = PPP_STATE_Req_Sent;
					RestartCount = MAX_RESTARTS;
					LinkTimer = 0;
					TimerOn = true;
					Send_Configure_Request();
				break;

				case PPP_EVENT_Close:
				case PPP_EVENT_RTA:
				case PPP_EVENT_RXJPlus:
				case PPP_EVENT_RXR:
					*State = PPP_STATE_Closed;
				break;

				case PPP_EVENT_RCRPlus:
				case PPP_EVENT_RCRMinus:
				case PPP_EVENT_RCA:
				case PPP_EVENT_RCN:
					*State = PPP_STATE_Closed;
					Send_Terminate_Ack();
				break;

				case PPP_EVENT_TOPlus:
				case PPP_EVENT_TOMinus:
				break;

				default:
					Debug_Print("Illegal Event\r\n");
				break;
			}
		break;

		case PPP_STATE_Stopped:
			
			switch (Event)
			{
				case PPP_EVENT_Down:
					*State = PPP_STATE_Starting;
					This_Layer_Started();
				break;

				case PPP_EVENT_Open:
					*State = PPP_STATE_Stopped;
				break;

				case PPP_EVENT_Close:
					*State = PPP_STATE_Closed;
				break;

				case PPP_EVENT_RCRPlus:
					*State = PPP_STATE_Ack_Sent;
					RestartCount = MAX_RESTARTS;
					LinkTimer = 0;
					TimerOn = true;
					Send_Configure_Request();
					Send_Configure_Ack();
				break;

				case PPP_EVENT_RCRMinus:
					*State = PPP_STATE_Req_Sent;
					RestartCount = MAX_RESTARTS;
					LinkTimer = 0;
					TimerOn = true;
					Send_Configure_Request();
					Send_Configure_Nak_Rej();
				break;

				case PPP_EVENT_RCA:
				case PPP_EVENT_RCN:
				case PPP_EVENT_RTR:
					*State = PPP_STATE_Stopped;
					Send_Terminate_Ack();
				break;

				case PPP_EVENT_RTA:
				case PPP_EVENT_RXJPlus:
				case PPP_EVENT_RXR:
					*State = PPP_STATE_Stopped;
				break;

				case PPP_EVENT_RUC:
					*State = PPP_STATE_Stopped;
					Send_Code_Reject();
				break;
				
				case PPP_EVENT_RXJMinus:
					*State = PPP_STATE_Stopped;
					This_Layer_Finished();
				break;

				case PPP_EVENT_TOPlus:
				case PPP_EVENT_TOMinus:
				break;

				default:
					Debug_Print("Illegal Event\r\n");
				break;
			}
		break;

		case PPP_STATE_Closing:

			switch (Event)
			{
				case PPP_EVENT_Down:
					TimerOn = false;
					*State = PPP_STATE_Initial;
				break;

				case PPP_EVENT_Open:
					*State = PPP_STATE_Stopping;
				break;

				case PPP_EVENT_Close:
					*State = PPP_STATE_Closing;
				break;

				case PPP_EVENT_TOPlus:
					Send_Terminate_Request();
					*State = PPP_STATE_Closing;
				break;
				
				case PPP_EVENT_TOMinus:
					*State = PPP_STATE_Closed;
					TimerOn = false;
					This_Layer_Finished();
				break;

				case PPP_EVENT_RCRPlus:
				case PPP_EVENT_RCRMinus:
				case PPP_EVENT_RCA:
				case PPP_EVENT_RCN:
				case PPP_EVENT_RXJPlus:
				case PPP_EVENT_RXR:
					*State = PPP_STATE_Closing;
				break;

				case PPP_EVENT_RTR:
					*State = PPP_STATE_Closing;
					Send_Terminate_Ack();
				break;

				case PPP_EVENT_RTA:
				case PPP_EVENT_RXJMinus:
					*State = PPP_STATE_Closed;
					TimerOn = false;
					This_Layer_Finished();
				break;

				case PPP_EVENT_RUC:
					*State = PPP_STATE_Closing;
					Send_Code_Reject();
				break;

				default:
					Debug_Print("Illegal Event\r\n");
				break;
			}
		break;

		case PPP_STATE_Stopping:
			
			switch (Event)
			{
				case PPP_EVENT_Down:
					*State = PPP_STATE_Starting;
					TimerOn = false;
				break;

				case PPP_EVENT_Open:
					*State = PPP_STATE_Stopping;
				break;

				case PPP_EVENT_Close:
					*State = PPP_STATE_Closing;
				break;

				case PPP_EVENT_TOPlus:
					Send_Terminate_Request();
					*State = PPP_STATE_Stopping;
				break;
				
				case PPP_EVENT_TOMinus:
					This_Layer_Finished();
					TimerOn = false;
					*State = PPP_STATE_Stopped;
				break;

				case PPP_EVENT_RCRPlus:
				case PPP_EVENT_RCRMinus:
				case PPP_EVENT_RCA:
				case PPP_EVENT_RCN:
				case PPP_EVENT_RXJPlus:
				case PPP_EVENT_RXR:
					*State = PPP_STATE_Stopping;
				break;

				case PPP_EVENT_RTR:
					Send_Terminate_Ack();
					*State = PPP_STATE_Stopping;
				break;

				case PPP_EVENT_RTA:
				case PPP_EVENT_RXJMinus:
					This_Layer_Finished();
					TimerOn = false;
					*State = PPP_STATE_Stopped;
				break;

				case PPP_EVENT_RUC:
					Send_Code_Reject();
					*State = PPP_STATE_Stopping;
				break;

				default:
					Debug_Print("Illegal Event\r\n");
				break;
			}
		break;

		case PPP_STATE_Req_Sent:
			
			switch (Event)
			{
				case PPP_EVENT_Down:
					*State = PPP_STATE_Starting;
					TimerOn = false;
				break;

				case PPP_EVENT_Open:
				case PPP_EVENT_RTA:
				case PPP_EVENT_RXJPlus:
				case PPP_EVENT_RXR:
					*State = PPP_STATE_Req_Sent;
				break;

				case PPP_EVENT_Close:
					RestartCount = MAX_RESTARTS;
					LinkTimer = 0;
					Send_Terminate_Request();
					*State = PPP_STATE_Closing;
				break;

				case PPP_EVENT_TOPlus:
					Send_Configure_Request();
					*State = PPP_STATE_Req_Sent;
				break;

				case PPP_EVENT_TOMinus:
					This_Layer_Finished();
					TimerOn = false;
					*State = PPP_STATE_Stopped;
				break;

				case PPP_EVENT_RCRPlus:
					Send_Configure_Ack();
					*State = PPP_STATE_Ack_Sent;
				break;

				case PPP_EVENT_RCRMinus:
					Send_Configure_Nak_Rej();
					*State = PPP_STATE_Req_Sent;
				break;

				case PPP_EVENT_RCA:
					RestartCount = MAX_RESTARTS;
					LinkTimer = 0;
					*State = PPP_STATE_Ack_Rcvd;
				break;

				case PPP_EVENT_RCN:
					RestartCount = MAX_RESTARTS;
					LinkTimer = 0;
					Send_Configure_Request();
					*State = PPP_STATE_Req_Sent;
				break;

				case PPP_EVENT_RTR:
					Send_Terminate_Ack();
					*State = PPP_STATE_Req_Sent;
				break;

				case PPP_EVENT_RUC:
					Send_Code_Reject();
					*State = PPP_STATE_Req_Sent;
				break;

				case PPP_EVENT_RXJMinus:
					This_Layer_Finished();
					TimerOn = false;
					*State = PPP_STATE_Stopped;
				break;
				
				default:
					Debug_Print("Illegal Event\r\n");
				break;
			}
		break;

		case PPP_STATE_Ack_Rcvd:

			switch (Event)
			{
				case PPP_EVENT_Down:
					*State = PPP_STATE_Starting;
					TimerOn = false;
				break;

				case PPP_EVENT_Open:
				case PPP_EVENT_RXR:
					*State = PPP_STATE_Ack_Rcvd;
				break;

				case PPP_EVENT_Close:
					RestartCount = MAX_RESTARTS;
					LinkTimer = 0;
					Send_Terminate_Request();
					*State = PPP_STATE_Closing;
				break;

				case PPP_EVENT_TOPlus:
				case PPP_EVENT_RCA:
				case PPP_EVENT_RCN:
					Send_Configure_Request();
					*State = PPP_STATE_Req_Sent;
				break;

				case PPP_EVENT_TOMinus:
					This_Layer_Finished();
					TimerOn = false;
					*State = PPP_STATE_Stopped;
				break;

				case PPP_EVENT_RCRPlus:
					Send_Configure_Ack();
					This_Layer_Up();
					TimerOn = false;
					*State = PPP_STATE_Opened;
				break;

				case PPP_EVENT_RCRMinus:
					Send_Configure_Nak_Rej();
					*State = PPP_STATE_Ack_Rcvd;
				break;

				case PPP_EVENT_RTR:
					Send_Terminate_Ack();
					*State = PPP_STATE_Req_Sent;
				break;

				case PPP_EVENT_RTA:
				case PPP_EVENT_RXJPlus:
					Send_Terminate_Ack();
					*State = PPP_STATE_Req_Sent;
				break;

				case PPP_EVENT_RUC:
					Send_Code_Reject();
					*State = PPP_STATE_Ack_Rcvd;
				break;

				case PPP_EVENT_RXJMinus:
					This_Layer_Finished();
					TimerOn = false;
					*State = PPP_STATE_Stopped;
				break;
				
				default:
					Debug_Print("Illegal Event\r\n");
				break;
			}
		break;

		case PPP_STATE_Ack_Sent:
			
			switch (Event)
			{
				case PPP_EVENT_Down:
					*State = PPP_STATE_Starting;
					TimerOn = false;
				break;

				case PPP_EVENT_Open:
				case PPP_EVENT_RTA:
				case PPP_EVENT_RXJPlus:
				case PPP_EVENT_RXR:
					*State = PPP_STATE_Ack_Sent;
				break;

				case PPP_EVENT_Close:
					RestartCount = MAX_RESTARTS;
					LinkTimer = 0;
					Send_Terminate_Request();
					*State = PPP_STATE_Closing;
				break;

				case PPP_EVENT_TOPlus:
					Send_Configure_Request();
					*State = PPP_STATE_Ack_Sent;
				break;

				case PPP_EVENT_TOMinus:
					This_Layer_Finished();
					TimerOn = false;
					*State = PPP_STATE_Stopped;
				break;

				case PPP_EVENT_RCRPlus:
					Send_Configure_Ack();
					*State = PPP_STATE_Ack_Sent;
				break;

				case PPP_EVENT_RCRMinus:
					Send_Configure_Nak_Rej();
					*State = PPP_STATE_Req_Sent;
				break;

				case PPP_EVENT_RCA:
					RestartCount = MAX_RESTARTS;
					LinkTimer = 0;
					TimerOn = false;
					This_Layer_Up();
					*State = PPP_STATE_Opened;
				break;

				case PPP_EVENT_RCN:
					RestartCount = MAX_RESTARTS;
					LinkTimer = 0;
					Send_Configure_Request();
					*State = PPP_STATE_Ack_Sent;
				break;

				case PPP_EVENT_RTR:
					Send_Terminate_Ack();
					*State = PPP_STATE_Req_Sent;
				break;

				case PPP_EVENT_RUC:
					Send_Code_Reject();
					*State = PPP_STATE_Ack_Sent;
				break;

				case PPP_EVENT_RXJMinus:
					This_Layer_Finished();
					TimerOn = false;
					*State = PPP_STATE_Stopped;
				break;
				
				default:
					Debug_Print("Illegal Event\r\n");
				break;
			}
		break;

		case PPP_STATE_Opened:

			switch (Event)
			{
				case PPP_EVENT_Down:
					This_Layer_Down();
					*State = PPP_STATE_Starting;
				break;

				case PPP_EVENT_Open:
				case PPP_EVENT_RXJPlus:
					*State = PPP_STATE_Opened;
				break;

				case PPP_EVENT_Close:
					This_Layer_Down();
					RestartCount = MAX_RESTARTS;
					LinkTimer = 0;
					TimerOn = true;
					Send_Terminate_Request();
					*State = PPP_STATE_Closing;
				break;

				case PPP_EVENT_RCRPlus:
					This_Layer_Down();
					TimerOn = true;
					Send_Configure_Request();
					Send_Configure_Ack();
					*State = PPP_STATE_Ack_Sent;
				break;

				case PPP_EVENT_RCRMinus:
					This_Layer_Down();
					TimerOn = true;
					Send_Configure_Request();
					Send_Configure_Nak_Rej();
					*State = PPP_STATE_Req_Sent;
				break;

				case PPP_EVENT_RCA:
				case PPP_EVENT_RCN:
				case PPP_EVENT_RTA:
					This_Layer_Down();
					TimerOn = true;
					Send_Configure_Request();
					*State = PPP_STATE_Req_Sent;
				break;

				case PPP_EVENT_RTR:
					This_Layer_Down();
					RestartCount = 0;
					LinkTimer = 0;
					TimerOn = true;
					Send_Terminate_Ack();
					*State = PPP_STATE_Stopping;
				break;

				case PPP_EVENT_RUC:
					Send_Code_Reject();
					*State = PPP_STATE_Opened;
				break;

				case PPP_EVENT_RXJMinus:
					This_Layer_Down();
					RestartCount = MAX_RESTARTS;
					LinkTimer = 0;
					TimerOn = true;
					Send_Terminate_Request();
					*State = PPP_STATE_Stopping;
				break;
				
				case PPP_EVENT_RXR:
					Send_Echo_Reply();
					*State = PPP_STATE_Opened;
				break;
				
				default:
					Debug_Print("Illegal Event\r\n");
				break;
			}
		break;

		default:
		break;
	}

}
