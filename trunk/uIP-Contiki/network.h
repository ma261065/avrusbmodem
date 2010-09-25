/*
 * Simple common network interface that all network drivers should implement.
 */

#ifndef __NETWORK_H__
#define __NETWORK_H__

	#include <avr/io.h>
	#include <util/delay.h>

	#include <uIP-Contiki/uip.h>

	#include "USBModem.h"
	#include "Lib/RingBuff.h"
	
	/* External Variables: */
		extern RingBuff_t Modem_SendBuffer;
		extern RingBuff_t Modem_ReceiveBuffer;
		extern bool DebugModeEnabled;

	/* Enums: */
		enum Packet_States_t
		{
			PACKET_STATE_NULL     = 0,
			PACKET_STATE_INHEADER = 1,
			PACKET_STATE_INBODY   = 2,
		};

	/* Function Prototypes: */
		void network_init(void);
		uint16_t network_read(void);
		void network_send(uint16_t protocol);
				
		#if defined(INCLUDE_FROM_NETWORK_C)
			static void DumpPacket(void);
		#endif

	/*Sets the MAC address of the device*/
	//void network_set_MAC(uint8_t* mac);

	/*Gets the MAC address of the device*/
	//void network_get_MAC(uint8_t* mac);

#endif /* __NETWORK_H__ */
