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
		#include <stdint.h>

		#include "LinkManagement.h"
		#include "Lib/RingBuff.h"
		#include "Lib/Debug.h"
	
	/* Enums: */
		typedef enum
		{
			PPP_PHASE_Dead,
			PPP_PHASE_Establish,
			PPP_PHASE_Authenticate,
			PPP_PHASE_Network,
			PPP_PHASE_Terminate
		} PPP_Phases_t;

		typedef enum
		{
			PPP_STATE_Initial,
			PPP_STATE_Starting,
			PPP_STATE_Closed,
			PPP_STATE_Stopped,
			PPP_STATE_Closing,
			PPP_STATE_Stopping,
			PPP_STATE_Req_Sent,
			PPP_STATE_Ack_Rcvd,
			PPP_STATE_Ack_Sent,
			PPP_STATE_Opened
		} PPP_States_t;

		typedef enum
		{
			PPP_EVENT_Up,
			PPP_EVENT_Down,
			PPP_EVENT_Open,
			PPP_EVENT_Close,
			PPP_EVENT_TOPlus,
			PPP_EVENT_TOMinus,
			PPP_EVENT_RCRPlus,
			PPP_EVENT_RCRMinus,
			PPP_EVENT_RCA,
			PPP_EVENT_RCN,
			PPP_EVENT_RTR,
			PPP_EVENT_RTA,
			PPP_EVENT_RUC,
			PPP_EVENT_RXJPlus,
			PPP_EVENT_RXJMinus,
			PPP_EVENT_RXR
		} PPP_Events_t;

	/* Type Defines: */
		typedef struct
		{
			uint8_t Type;
			uint8_t Length;
			uint8_t Data[];
		} PPP_Option_t;		

		typedef struct
		{
			uint8_t Code;
			uint8_t PacketID;
			uint16_t Length;
			PPP_Option_t Options[];
		} PPP_Packet_t;

	/* Macros: */
		#define CALC_CRC16(crcvalue, c)    _crc_ccitt_update(crcvalue, c);

		// Defines for LCP Negotiation Codes
		#define	REQ						1							// Request options list for PPP negotiations
		#define	ACK						2							// Acknowledge options list for PPP negotiations
		#define	NAK						3							// Not acknowledged options list for PPP negotiations
		#define	REJ						4							// Reject options list for PPP negotiations
		#define	TERMREQ					5							// Termination request for LCP to close connection
		#define	TERMREPLY				6							// Termination reply
		#define CODEREJ					7							// Code reject
		#define PROTREJ					8							// Protocol reject
		#define ECHOREQ					9							// Echo Request
		#define ECHOREPLY				10							// Echo Reply
		#define DISC 					11							// Discard request

		// Packet Types (Protocols)
		#define	IP 						0x0021						// Internet Protocol packet
		#define	IPCP 					0x8021						// Internet Protocol Configure Protocol packet
		#define	LCP						0xC021						// Link Configure Protocol packet
		#define	PAP						0xC023						// Password Authentication Protocol packet
		#define CHAP 					0xC223						// Challenge Handshake Authentication Protocol packet
		#define NONE 					0x0000

		#define LCP_OPTION_Maximum_Receive_Unit 					0x1		// LCP Option 1
		#define LCP_OPTION_Async_Control_Character_Map 				0x2		// LCP Option 2
		#define LCP_OPTION_Authentication_Protocol 					0x3		// LCP Option 3
		#define LCP_OPTION_Quality_Protocol 						0x4		// LCP Option 4
		#define LCP_OPTION_Magic_Number 							0x5		// LCP Option 5
		#define LCP_OPTION_Reserved			 						0x6		// LCP Option 6
		#define LCP_OPTION_Protocol_Field_Compression 				0x7		// LCP Option 7
		#define LCP_OPTION_Address_and_Control_Field_Compression 	0x8		// LCP Option 8

		#define IPCP_OPTION_IP_Compression_Protocol					0x2		// IPCP Option 2
		#define IPCP_OPTION_IP_address								0x3		// IPCP Option 3
		#define IPCP_OPTION_Primary_DNS								0x81	// IPCP Option 81
		#define IPCP_OPTION_Secondary_DNS							0x83	// IPCP Option 83

		#define MAX_RESTARTS										5

	/* Function Prototypes: */
		void PPP_ManageLink(void);
		void PPP_InitPPP(void);
		void PPP_LinkTimer(void);
		void PPP_LinkUp(void);
		void PPP_LinkOpen(void);

		#if defined(INCLUDE_FROM_PPP_C)
			static PPP_Option_t* PPP_GetNextOption(const PPP_Packet_t* const ThisPacket,
			                                       const PPP_Option_t* const CurrentOption);
			static void PPP_RemoveOption(PPP_Packet_t* const ThisPacket,
			                             const uint8_t Type);
			static void PPP_AddOption(PPP_Packet_t* const ThisPacket,
			                          const PPP_Option_t* const Option);
			static void PPP_ChangeOption(PPP_Packet_t* const ThisPacket,
			                             const PPP_Option_t* const Option);
			static void PPP_ProcessNAK(void);
			static void PPP_ProcessREJ(void);
			static bool PPP_TestForNAK(const PPP_Option_t* const Option);
			static bool PPP_TestForREJ(const uint8_t Options[],
			                           const uint8_t NumOptions);
			static void PPP_ManageState(const PPP_Events_t Event,
										PPP_States_t* const State);
			static void Send_Configure_Request(void);
			static void Send_Configure_Ack(void);
			static void Send_Configure_Nak_Rej(void);
			static void Send_Terminate_Request(void);
			static void Send_Terminate_Ack(void);
			static void Send_Code_Reject(void);
			static void Send_Echo_Reply(void);
			static void This_Layer_Up(void);
			static void This_Layer_Down(void);
			static void This_Layer_Started(void);
			static void This_Layer_Finished(void);
			static void FreePacketMemory(void);
		#endif
		
#endif


