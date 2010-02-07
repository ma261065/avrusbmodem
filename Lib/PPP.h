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

#ifndef _PPP_H_
#define _PPP_H_

	/* Includes: */
		#include <util/crc16.h>
		#include <stdbool.h>

		#include "LinkManagement.h"
		#include "Lib/RingBuff.h"
		#include "Lib/Debug.h"
	
	/* Enums: */
		typedef enum
		{
			PPP_STATE_LCPNegotiation  = 0,
			PPP_STATE_PAPNegotiation  = 1,
			PPP_STATE_IPCPNegotiation = 2,
		} PPP_States_t;

	/* Macros: */
		#define CALC_CRC16(crcvalue, c)    _crc_ccitt_update(crcvalue, c);

		// Defines for Internet constants
		#define	REQ	1 												// Request options list for PPP negotiations
		#define	ACK	2												// Acknowledge options list for PPP negotiations
		#define	NAK	3												// Not acknowledged options list for PPP negotiations
		#define	REJ	4												// Reject options list for PPP negotiations
		#define	TERM 5												// Termination packet for LCP to close connection
		#define	IP 0x0021											// Internet Protocol packet
		#define	IPCP 0x8021											// Internet Protocol Configure Protocol packet
		#define	CCP	0x80FD											// Compression Configure Protocol packet
		#define	LCP	0xC021											// Link Configure Protocol packet
		#define	PAP	0xC023											// Password Authentication Protocol packet
		#define CHAP 0xC223											// Challenge Handshake Authentication Protocol packet
		#define NONE 0

		#define	MaxRx	512											// Maximum size of receive buffer
		#define	MaxTx	512											// Maximum size of transmit buffer (was 46)

	/* Function Prototypes: */
		void    PPP_ManagePPPNegotiation(void);
		
		#if defined(INCLUDE_FROM_PPP_C)
			static void    PPP_AddToPacket(uint8_t c);
			static void    PPP_CreatePacket(uint16_t protocol, uint8_t packetType, uint8_t packetID, const uint8_t *str);
			static uint8_t PPP_TestOptions(uint16_t option);
			static void    PPP_ProcessReceivedPacket(void);
			static void    PPP_MakeInitialPacket(void);
		#endif
		
#endif


