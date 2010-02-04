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

/** \file
 *
 *  Header file for ConnectionManagement.c.
 */

#ifndef _CONNECTION_MANAGEMENT_H_
#define _CONNECTION_MANAGEMENT_H_

	/* Includes: */
		#include <avr/pgmspace.h>
		#include <stdlib.h>
		#include <string.h>

		#include <uIP-Contiki/network.h>
		#include <uIP-Contiki/timer.h>
		#include <uIP-Contiki/uip.h>

		#include "Lib/RingBuff.h"
		#include "Lib/Debug.h"
		#include "Lib/PPP.h"
	
	/* Enums: */
		enum Connection_Management_States_t
		{
			CONNECTION_MANAGE_STATE_DialConnection      = 0,
			CONNECTION_MANAGE_STATE_DoPPPNegotiation    = 1,
			CONNECTION_MANAGE_STATE_InitializeTCPStack  = 2,
			CONNECTION_MANAGE_STATE_ConnectToRemoteHost = 3,
			CONNECTION_MANAGE_STATE_ManageTCPConnection = 4,
		};
	
	/* External Variables: */
		extern uint16_t TIME;								// 10 millseconds counter
		extern uint8_t ConnectedState;
		extern uint8_t IPAddr1, IPAddr2, IPAddr3, IPAddr4;	// The IP address allocated to us by the remote end
	
	/* Function Prototypes: */
		void ConnectionManagement_ManageConnectionState(void);
		void ConnectionManagement_DialConnection(void);
		void ConnectionManagement_InitializeTCPStack(void);
		void ConnectionManagement_ConnectToRemoteHost(void);
		void ConnectionManagement_TCPIPTask(void);

		void TCPCallback(void);
		void SendGET(void);
		void SendPOST(void);

#endif
