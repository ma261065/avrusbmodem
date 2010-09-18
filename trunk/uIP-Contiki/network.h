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

	/* Enums: */
	enum Packet_States_t
	{
		PACKET_STATE_NULL     = 0,
		PACKET_STATE_INHEADER = 1,
		PACKET_STATE_INBODY   = 2,
	};

	/*Initialize the network*/
	void network_init(void);

	/*Read from the network, returns protocol*/
	uint16_t network_read(void);

	/*Send using the network*/
	void network_send(uint16_t protocol);

	void DumpPacket(void);

	/*Sets the MAC address of the device*/
	//void network_set_MAC(uint8_t* mac);

	/*Gets the MAC address of the device*/
	//void network_get_MAC(uint8_t* mac);

#endif /* __NETWORK_H__ */
