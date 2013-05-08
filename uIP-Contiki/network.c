#define  INCLUDE_FROM_NETWORK_C
#include "network.h"

static bool Escape = false;
static uint8_t PacketState = PACKET_STATE_NULL;
static uint16_t PacketProtocol = 0;
static uint16_t PacketLength = 0;
static uint16_t CurrentChecksum, OneBackChecksum, TwoBackChecksum;


// Framed packet looks like:
// Framing:     0x7e
// Address:     0xff
// Control:     0x03
// Protocol:    0xnn 0xnn
// Information: 0xnn 0xnn 0xnn 0xnn ...
// Checksum:    0xnn 0xnn
// Framing:     0x7e
//
// If Address-and-Control-Field-Compression is switched on then the remote end is allowed to leave out the 0xff 0x03 in the header
// If Protocol-Field-Compression is switched on and the protocol < 0x00ff then the remote end is allowed to leave out the initial 0x00 in the protocol field. 
// We can check for Protocol-Field-Compression by seeing if the first byte of the protocol is odd (the first byte of a full two-byte protocol value must be even).

uint16_t network_read(void)
{
	uint8_t c;
	
	while (!RingBuffer_IsEmpty(&Modem_ReceiveBuffer))
	{
		c = RingBuffer_Remove(&Modem_ReceiveBuffer);

		switch (PacketState)
		{
			case PACKET_STATE_NULL:
				if (c == 0x7e)												// New Packet
				{
					PacketState = PACKET_STATE_INHEADER;					// We're now in the header
					PacketLength = 0;
					CurrentChecksum = 0xffff;								// Start new checksum
				}
			break;

			case PACKET_STATE_INHEADER:
				if (c == 0x7d)												// Escaped character. Set flag and process next time around
				{
					Escape = true;
					continue;
				}
				else if (Escape)											// Escaped character. Process now.
				{
					Escape = false;
					c = (c ^ 0x20);
				}
				
				if (PacketLength == 1 && c != 0xff)							// Should be 0xff. If not then Address-and-Control-Field-Compression is switched on
				{
					PacketLength += 2;										// Adjust the length as if the 0xff 0x03 was there
				}		

				if (PacketLength == 3)
				{
					if (c & 1)												// Should be even. If it's odd then Protocol-Field-Compression is switched on
					{
						PacketProtocol = 0x00;								// Add in the initial 0x00
						PacketLength++;										// Adjust the length as if the 0x00 was there
					}
					else
						PacketProtocol = c * 256;							// Store the MSB of the protocol
				}

				if (PacketLength == 4)										// End of header
				{														
					PacketProtocol |= c;									// Store the LSB of the protocol
					PacketLength = -1;										// This will be incremented to 0 at the end of this loop
					Escape = false;
					PacketState = PACKET_STATE_INBODY;						// We're now in the body
				}

				CurrentChecksum = CALC_CRC16(CurrentChecksum, c);			// Calculate checksum
			break;

			case PACKET_STATE_INBODY:
				if (c == 0x7e)												// End of packet
				{
					uip_len = PacketLength - 2;								// Strip off the checksum and framing
					Escape = false;
					PacketState = PACKET_STATE_NULL;						// Back to waiting for a header

					Debug_Print("\r\nReceive ");
					DumpPacket();

					if (~TwoBackChecksum == (*(uip_buf + PacketLength - 1) * 256 + *(uip_buf + PacketLength - 2)))
						return PacketProtocol;
					else
						Debug_Print("Bad CRC\r\n");
				}
				else
				{	
					if (c == 0x7d)											// Escaped character. Set flag and process next time around
					{
						Escape = true;
						continue;
					}
					else if (Escape)										// Escaped character. Process now.
					{
						Escape = false;
						c = (c ^ 0x20);
					}
					
					*(uip_buf + PacketLength) = c;							// Store the character in the buffer

					TwoBackChecksum = OneBackChecksum;						// Keep a rolling count of the last 3 checksums
					OneBackChecksum = CurrentChecksum;						// Eventually we need to see if the checksum is valid
					CurrentChecksum = CALC_CRC16(CurrentChecksum, c);		// and we need the one two back (as the current one includes the checksum itself)
				}
			break;
		}
		
		PacketLength++;														// Increment the length of the received packet
	}

	return 0;																// No data or packet not complete yet 
}

void network_send(uint16_t protocol)
{
	unsigned int checksum = 0xffff;
	
	PacketProtocol = protocol;
	PacketState = PACKET_STATE_NULL;
	
	Debug_Print("\r\nSend ");
	DumpPacket();
	
	// Send out the packet with HDLC-like framing  - see http://tools.ietf.org/html/rfc1662
	
	// Start with the framing flag
	RingBuffer_Insert(&Modem_SendBuffer, 0x7e);

	// Address	
	RingBuffer_Insert(&Modem_SendBuffer, 0xff);
	checksum = CALC_CRC16(checksum, 0xff);

	// Control
	RingBuffer_Insert(&Modem_SendBuffer, 0x03);
	checksum = CALC_CRC16(checksum, 0x03);

	// Protocol
	RingBuffer_Insert(&Modem_SendBuffer, protocol / 256);
	checksum = CALC_CRC16(checksum, protocol / 256);

	RingBuffer_Insert(&Modem_SendBuffer, protocol & 255);
	checksum = CALC_CRC16(checksum, protocol & 255);

	// Add the information, escaping it as necessary
	for (uint16_t i = 0; i < uip_len; i++)
	{
		if (*(uip_buf + i) < 0x20 || *(uip_buf + i) == 0x7d || *(uip_buf + i) == 0x7e)
		{
			RingBuffer_Insert(&Modem_SendBuffer, 0x7d);
			RingBuffer_Insert(&Modem_SendBuffer, *(uip_buf + i) ^ 0x20);
		}
		else
		{
			RingBuffer_Insert(&Modem_SendBuffer, *(uip_buf + i));
		}

		checksum = CALC_CRC16(checksum, *(uip_buf + i));
	
		if (RingBuffer_GetCount(&Modem_SendBuffer) >= 64)    			// Periodically flush the buffer to the modem
			USBManagement_SendReceivePipes();
	}

	// Add the checksum to the end of the packet, escaping it as necessary
	checksum = ~checksum;

	if ((checksum & 255) < 0x20 || (checksum & 255) == 0x7d || (checksum & 255) == 0x7e)
	{
		RingBuffer_Insert(&Modem_SendBuffer, 0x7d);
		RingBuffer_Insert(&Modem_SendBuffer, (checksum & 255) ^ 0x20);
	}
   	else
	{
		RingBuffer_Insert(&Modem_SendBuffer, checksum & 255);			// Insert checksum MSB
	}
	
	if ((checksum / 256) < 0x20 || (checksum / 256) == 0x7d || (checksum / 256) == 0x7e)
	{
		RingBuffer_Insert(&Modem_SendBuffer, 0x7d);
		RingBuffer_Insert(&Modem_SendBuffer, (checksum / 256) ^ 0x20);
	}
	else
	{
		RingBuffer_Insert(&Modem_SendBuffer, checksum / 256);			// Insert checksum LSB
	}
   
	RingBuffer_Insert(&Modem_SendBuffer, 0x7e);						// Framing

	uip_len = 0;

	USBManagement_SendReceivePipes();									// Flush the rest of the buffer
}

void network_init(void)
{
	//Initialise the device
	PacketState = PACKET_STATE_NULL;
	Escape = false;
	uip_len = 0;
}

void DumpPacket(void)
{
	int i, j;

	if (!DebugModeEnabled)
		return;

	Debug_Print("(Protocol = 0x");
	Debug_PrintHex(PacketProtocol  / 256); Debug_PrintHex(PacketProtocol  & 255);
	Debug_Print("):\r\n");
	
	for (i = 0; i < uip_len; i += 16)
	{	
		Debug_PrintHex(i / 256); Debug_PrintHex(i & 255); Debug_Print(": ");

		for (j = 0; j < 16; j++)
		{
			if ((i + j) >= uip_len)
				break;

			Debug_PrintHex(*(uip_buf + i + j));
			Debug_PrintChar(' ');
		}
		
		Debug_Print("\r\n      ");
		
		for (j = 0; j < 16; j++)
		{
			if ((i + j) >= uip_len)
				break;

			if (*(uip_buf + i + j) >= 0x20 && *(uip_buf + i + j) <= 0x7e)
			{
				Debug_PrintChar(' ');
				Debug_PrintChar(*(uip_buf + i + j));
				Debug_PrintChar(' ');
			}
			else
				Debug_Print(" . ");

		}

		Debug_Print("\r\n");
	}
}
