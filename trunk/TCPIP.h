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
 *  Header file for TCPIP.c.
 */

#ifndef _TCPIP_H_
#define _TCPIP_H_

	/* Includes: */
		#include <stdbool.h>
	
		#include <uIP-Contiki/uip.h>

		#include "Lib/Debug.h"
	
	/* Function Prototypes: */
		bool TCPIP_Connect(void);
		void TCPIP_TCPCallback(void);
		void TCPIP_InitializeTCPStack(void);
		void TCPIP_ConnectToRemoteHost(void);
		void TCPIP_GotNewPacket(void);
		void TCPIP_TCPIPTask(void);

		#if defined(INCLUDE_FROM_TCPIP_C)
			static void TCPIP_SendGET(void);
			static void TCPIP_QueueData(const char* Data,
			                            const uint16_t Length);
			static bool TCPIP_IsDataQueueFull(void);
		#endif
		
#endif
