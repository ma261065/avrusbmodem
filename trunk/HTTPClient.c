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

#include "HTTPClient.h"

uip_ipaddr_t     RemoteIPAddress;
struct uip_conn* HTTPConnection;


bool HTTPClient_Connect(void)
{
	// Connect to the remote machine
	uip_ipaddr(&RemoteIPAddress, 192, 0, 32, 10);	// www.example.com
	HTTPConnection = uip_connect(&RemoteIPAddress, HTONS(80));

	Debug_Print("Maximum Segment Size: 0x"); Debug_PrintHex(uip_mss() / 256);
	Debug_Print("0x"); Debug_PrintHex(uip_mss() & 255); 
	Debug_Print("\r\n");	

	if (HTTPConnection != NULL)
	{
		Debug_Print("Connected to host\r\n");
		return true;
	}
	else
	{
		Debug_Print("Failed to Connect\r\n");
		return false;
	}
}

void HTTPClient_TCPCallback(void)
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

	if (uip_poll() && (TIME > 3000))
	{
		TIME = 0;
		
		Debug_Print("\r\nSending GET\r\n");
		HTTPClient_SendGET();
	}
	
	if (uip_rexmit())
	{
		Debug_Print("\r\nRetransmit GET\r\n");
		HTTPClient_SendGET();
	}

	if (uip_newdata())
	{
		HTTPClient_QueueData(uip_appdata, uip_datalen());
		
		if (HTTPClient_IsDataQueueFull())
		  uip_stop();
	}

	if (uip_poll() && uip_stopped(HTTPConnection))
	{
		if (!(HTTPClient_IsDataQueueFull()))
		  uip_restart();
	}
}

void HTTPClient_SendGET(void)
{
	char GETRequest[] = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: Keep-Alive\r\n\r\n";

	uip_send(GETRequest, strlen(GETRequest));
}

void HTTPClient_QueueData(char *x, int len)
{
	Debug_Print("\r\nData:\r\n");
	WatchdogTicks = 0;							// Reset the watchdog count
	
	for (int i = 0; i < len; i++)
			putchar(*(x + i));
	
	Debug_Print("\r\n");
}

bool HTTPClient_IsDataQueueFull(void)
{
	return false;
}
