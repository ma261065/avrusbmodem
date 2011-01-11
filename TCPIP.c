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

#define  INCLUDE_FROM_TCPIP_C
#include "TCPIP.h"

uint8_t						IPAddr1, IPAddr2, IPAddr3, IPAddr4;
static uip_ipaddr_t			RemoteIPAddress;
static struct uip_conn*		TCPConnection;
static struct timer			Periodic_Timer;

bool TCPIP_Connect(void)
{
	// Connect to the remote machine (www.example.com)
	uip_ipaddr(&RemoteIPAddress, 192, 0, 32, 10);	
	TCPConnection = uip_connect(&RemoteIPAddress, HTONS(80));

	if (TCPConnection != NULL)
	{
		Debug_Print("Connecting to host\r\n");
		return true;
	}
	else
	{
		Debug_Print("Failed to Connect\r\n");
		return false;
	}
}

void TCPIP_TCPCallback(void)
{
	if (uip_acked())
		Debug_Print("[ACK] ");

	if (uip_newdata())
	{
		Debug_Print("New Data:\r\n");
		TCPIP_QueueData(uip_appdata, uip_datalen());
		
		if (TCPIP_IsDataQueueFull())
		  uip_stop();
	}

	if (uip_connected())
	{
		Debug_Print("Connected - Maximum Segment Size: 0x"); Debug_PrintHex(uip_mss() / 256); Debug_PrintHex(uip_mss() & 255); 
		Debug_Print("\r\n");
	}

	if (uip_closed())
	{
		Debug_Print("Closed - Reconnecting...");
		_delay_ms(1000);
		ConnectedState = LINKMANAGEMENT_STATE_ConnectToRemoteHost;
	}

	if (uip_aborted())
	{
		Debug_Print("Aborted - Reconnecting... ");
		_delay_ms(1000);
		ConnectedState = LINKMANAGEMENT_STATE_ConnectToRemoteHost;
	}

	if (uip_timedout())
	{
		Debug_Print("Timeout - Reconnecting...");
		uip_abort();
		_delay_ms(1000);
		ConnectedState = LINKMANAGEMENT_STATE_ConnectToRemoteHost;
	}

	if (uip_poll() && (SystemTicks > 3000))
	{
		SystemTicks = 0;
		
		Debug_Print("\r\nSending GET\r\n");
		TCPIP_SendGET();
	}
	
	if (uip_rexmit())
	{
		Debug_Print("\r\nRetransmit GET\r\n");
		TCPIP_SendGET();
	}

	if (uip_poll() && uip_stopped(TCPConnection))
	{
		if (!(TCPIP_IsDataQueueFull()))
		  uip_restart();
	}
}

static void TCPIP_SendGET(void)
{
	const char GETRequest[] = "GET / HTTP/1.1\r\n"
	                          "Host: www.example.com\r\n"
	                          "Connection: Keep-Alive\r\n\r\n";

	uip_send(GETRequest, strlen(GETRequest));
}

static void TCPIP_QueueData(const char* Data,
                            const uint16_t Length)
{
	if (Length > 0)
		WatchdogTicks = 0;							// Reset the timeout counter
	
	for (uint16_t i = 0; i < Length; i++)
		putchar(Data[i]);
	
	Debug_Print("\r\n");
}

static bool TCPIP_IsDataQueueFull(void)
{
	return false;
}

void TCPIP_InitializeTCPStack(void)
{
	Debug_Print("Init TCP Stack\r\n");

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
	SystemTicks = 2000;			// Make the first CONNECT happen straight away
}

void TCPIP_ConnectToRemoteHost(void)
{
	if (SystemTicks > 1000)		// Try to connect every 1 second
	{
		SystemTicks = 0;
		
		if (TCPIP_Connect())
		{
			SystemTicks = 3001;	// Make the first GET happen straight away
			uip_len = 0;
			ConnectedState = LINKMANAGEMENT_STATE_ManageTCPConnection;
		}
	}
}

void TCPIP_GotNewPacket(void)
{
	uip_input();																// Call the TCP/IP stack with the new packet
																					
	if (uip_len > 0)															// If the above function invocation resulted in data that should be sent out
	  network_send(IP);														// on the network, the global variable uip_len is set to a value > 0.
}

void TCPIP_TCPIPTask(void)
{
	if (timer_expired(&Periodic_Timer))
	{
		timer_reset(&Periodic_Timer);
		
		Debug_PrintChar('*');
		
		for (uint8_t i = 0; i < UIP_CONNS; i++)
		{
	 		uip_periodic(i);
	 		 
	 		if (uip_len > 0)													// If the above function invocation resulted in data that should be sent out on the network,
			  network_send(IP);												//the global variable uip_len is set to a value > 0.
		}
	}
}
