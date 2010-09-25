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
 *  Header file for LinkManagement.c.
 */

#ifndef _LINK_MANAGEMENT_H_
#define _LINK_MANAGEMENT_H_

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
		#include "TCPIP.h"
	
	/* Enums: */
		enum Link_Management_States_t
		{
			LINKMANAGEMENT_STATE_Idle,
			LINKMANAGEMENT_STATE_SendModemDialCommands,
			LINKMANAGEMENT_STATE_SendNetworkDialCommands,
			LINKMANAGEMENT_STATE_WaitForLink,
			LINKMANAGEMENT_STATE_InitializeTCPStack,
			LINKMANAGEMENT_STATE_ConnectToRemoteHost,
			LINKMANAGEMENT_STATE_ManageTCPConnection
		};
	
	/* External Variables: */
		extern uint8_t IPAddr1, IPAddr2, IPAddr3, IPAddr4;	// The IP address allocated to us by the remote end
		extern uint8_t  WatchdogTicks;
		extern uint16_t SystemTicks;
		extern RingBuff_t Modem_SendBuffer;
		extern RingBuff_t Modem_ReceiveBuffer;

		extern const char* ModemDialCommands[];
		extern const char* NetworkDialCommands[];

	/* Function Prototypes: */
		void LinkManagement_ManageConnectionState(void);
				
		#if defined(INCLUDE_FROM_LINKMANAGEMENT_C)
			static bool LinkManagement_DialConnection(const char** DialCommands);
		#endif
	
#endif
