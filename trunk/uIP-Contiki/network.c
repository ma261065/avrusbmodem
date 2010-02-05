#include "network.h"

char InHeader = 0;
char InPacket = 0;
char Escape = 0;
uint16_t len = 0;

unsigned int network_read(void)
{
	int c;
	
	while (Modem_ReceiveBuffer.Elements)
	{
		c = Buffer_GetElement(&Modem_ReceiveBuffer);
	
		if (InPacket == 0 && InHeader == 0 && c == 0x7e)				// New Packet
		{
			InHeader = 1;
			len = 0;
		}

		if (InHeader == 1)
		{
			if (c == 0x21)												// End of Header (Header is 00 21)
			{
				InHeader = 0;
				InPacket = 1;
				len = -1;
			}
			else if (len == 1 && c != 0x00)								// Got a non-SLIP packet, probably LCP-TERM. Need to re-establish link.
			{
				return -1;
			}
		}
		else if (InPacket == 1)
		{
			if (c == 0x7d)												// Escaped character. Set flag and process next time around
			{
				Escape = 1;
				len--;
			}
			else if (Escape == 1)										// Escaped character. Process now.
			{
				Escape = 0;
				*(uip_buf + len) = (c ^ 0x20);
			}
			else if (c == 0x7e)											// End of packet
			{
				InPacket = 0;
				Debug_Print("\r\n");
				return len - 2; 										// (-2) = Strip off checksum and framing
			}
			else
			{	
				*(uip_buf + len) = c;
			}
		}			
		
		len++;
	}

	return 0;	// Packet not finished yet if we got here.
}

void network_send(void)
{
	unsigned int checksum = 0xffff;
	int i, j;

	InPacket = 0; InHeader = 0;
	

	/********************** Debug **********************/
	Debug_Print("\r\nSend:\r\n");
	
	for (i = 0; i < uip_len; i += 16)
	{	
		for (j = 0; j < 16; j++)
		{
			if ((i + j) >= uip_len)
				break;

			Debug_PrintHex(*(uip_buf + i + j));

		}
		
		Debug_Print("\r\n");	
		
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
	/********************** Debug **********************/

	// Send out the packet with HDLC-like framing
	// See http://tools.ietf.org/html/rfc1662
	
	// Start with the framing
	Buffer_StoreElement(&Modem_SendBuffer, 0x7e);

	// Header	
	Buffer_StoreElement(&Modem_SendBuffer, 0xff);
	checksum = CALC_CRC16(checksum, 0xff);

	Buffer_StoreElement(&Modem_SendBuffer, 0x03);
	checksum = CALC_CRC16(checksum, 0x03);

	Buffer_StoreElement(&Modem_SendBuffer, 0x00);
	checksum = CALC_CRC16(checksum, 0x00);

	Buffer_StoreElement(&Modem_SendBuffer, 0x21);
	checksum = CALC_CRC16(checksum, 0x21);

	// Add the data, escaping it if necessary
	for (int i = 0; i < uip_len; i++)
	{
		if (*(uip_buf + i) == 0x7d || *(uip_buf + i) == 0x7e)
		{
			Buffer_StoreElement(&Modem_SendBuffer, 0x7d);
			Buffer_StoreElement(&Modem_SendBuffer, *(uip_buf + i) ^ 0x20);
		}
		else
		{
			Buffer_StoreElement(&Modem_SendBuffer, *(uip_buf + i));
		}

		checksum = CALC_CRC16(checksum, *(uip_buf + i));
	
		if (i % 64 == 0 && i != 0)										// Periodically flush the buffer to the modem
			USBManagement_SendReceivePipes();
	}

	// Add the checksum to the end of the packet, escaping it if necessary
	checksum = ~checksum;

	if ((checksum & 255) == 0x7d || (checksum & 255) == 0x7e)
	{
		Buffer_StoreElement(&Modem_SendBuffer, 0x7d);
		Buffer_StoreElement(&Modem_SendBuffer, (checksum & 255) ^ 0x20);
	}
   	else
	{
		Buffer_StoreElement(&Modem_SendBuffer, checksum & 255);			// Insert checksum MSB
	}
	
	if ((checksum / 256) == 0x7d || (checksum / 256) == 0x7e)
	{
		Buffer_StoreElement(&Modem_SendBuffer, 0x7d);
		Buffer_StoreElement(&Modem_SendBuffer, (checksum / 256) ^ 0x20);
	}
	else
	{
		Buffer_StoreElement(&Modem_SendBuffer, checksum / 256);			// Insert checksum LSB
	}
   
	Buffer_StoreElement(&Modem_SendBuffer, 0x7e);						// Framing


	USBManagement_SendReceivePipes();									// Flush the rest of the buffer
}

void network_init(void)
{
	//Initialise the device
	InPacket = InHeader = Escape = len = 0;	
}

