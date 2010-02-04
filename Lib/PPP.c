#include "ppp.h"

unsigned char addr1, addr2, addr3, addr4;						// Assigned IP address
unsigned int rx_ptr, tx_ptr, tx_end;							// Pointers into buffers
unsigned int checksum1, checksum1last, checksum1secondlast, checksum2;	// Rx and Tx checksums
unsigned char number;											// Unique packet ID
unsigned char tx_str[MaxRx + 1];								// Transmitter buffer
unsigned char rx_str[MaxTx + 1];								// Receiver buffer

unsigned int PacketType = NONE;									// Type of the last received packet
bool EscapeFlag;												// Flag if last character was an escape sequence
bool LocalReady, RemoteReady;									// Flags for the ready-state of this end and the remote end

enum {LCPState, PAPState, IPCPState} PPPState;					// PPP negotiation states


// The main loop, login script, PPP state machine
void DoPPP(void)
{
	signed int c;												// Received serial character
	bool ExitFlag = 0;
	int CharCount = 0;

	// This will kick-start each phase of the PPP negotiations (LCP, PAP, IPCP)
	MakeInitialPacket();

	// Stay in this loop until we have either received a full PPP packet, or until we have sent a full PPP packet, or we have nothing to do
	do
	{
		// Receive a full PPP packet
		if (Modem_ReceiveBuffer.Elements)		// Incoming character?
		{
			c = Buffer_GetElement(&Modem_ReceiveBuffer);		// Get the character
	     
		 	if (c == 0x7e)										// Start or end of a packet 
			{	
	        	if (rx_ptr == 0)								// Start of packet
				{
					Debug_Print("\r\n\r\nReceive:\r\n7e ");
					CharCount = 1;
				}
				else											// End of packet
				{
					Debug_Print("7e\r\n");

					// If CRC passes, then accept packet
				 	if (~checksum1secondlast == (rx_str[rx_ptr - 1] * 256 + rx_str[rx_ptr - 2]))
					{
						Debug_Print("CRC OK\r\n");
	           			PacketType = rx_str[2] * 256 + rx_str[3];

						ProcessReceivedPacket();				// Process the packet we received
						ExitFlag = true;
					}
					else
					{
						Debug_Print("*** BAD CRC ***\r\n");
						ExitFlag = true;
					}
	        	}
				
				EscapeFlag = false;								// Clear escape character flag
			   	rx_ptr = 0;										// Get ready for next packet
	        	checksum1 = 0xffff;								// Start new checksum
	     	}
		  	else if (c == 0x7d)									// If escape character then set escape flag
			{
				EscapeFlag = true;
	     	}
			else												// Process the character
			{
	        	CharCount++;
				
				if (EscapeFlag)									// If escape flag set from previous character
				{
	           		c ^= 0x20;									// Recover next character
	           		EscapeFlag = false;							// Clear escape flag
	        	}
	        
				// Uncompress PPP framing header
				if (rx_ptr == 0 && c != 0xff)					// If Address-and-Control-Field-Compression is switched on
					rx_str[rx_ptr++] = 0xff;					// then the remote end is allowed to leave out the 0xff 0x03 header
	        													// so we add it back in if its missing
				if (rx_ptr == 1 && c != 0x03)
					rx_str[rx_ptr++] = 0x03;
			
				// Uncompress PPP protocol header
				if (rx_ptr == 2 && (c & 1))						// If Protocol-Field-Compression is switched on
					rx_str[rx_ptr++] = 0x00;					// the remote end is allowed to leave out the initial 0x00 in the protocol field

		        rx_str[rx_ptr++] = c;							// Insert character in buffer
	        
				if (rx_ptr > MaxRx) 
					rx_ptr = MaxRx;								// Increment pointer. Max is end of buffer
				
				Debug_PrintHex(c);

				if (CharCount % 16 == 0)
					Debug_Print("\r\n");

	        	checksum1secondlast = checksum1last;			// Keep a rolling count of the last 3 checksums
				checksum1last = checksum1;						// Eventually we need to see if the checksum is valid
				checksum1 = CRC(checksum1, c);					// and we need the one two back (as the current one includes the checksum itself)
	     	}
	  	} 
	   	
		// Send a full PPP packet
		else if (tx_end)										// Do we have data to send?
		{
	     	c = tx_str[tx_ptr];									// Get character from buffer

	     	if (tx_ptr == tx_end)								// Was it the last character
			{
	        	tx_end = 0;										// Mark buffer empty
	        	c = 0x7e;										// Send end-of-frame character
				
				Debug_Print("7e\r\n");
				
				ExitFlag = true;
	     	}
			else if (EscapeFlag)								// Sending escape sequence?
			{
				CharCount++;
				Debug_PrintHex(c);

				if (CharCount % 16 == 0)
					Debug_Print("\r\n");

				c ^= 0x20;										// Yes then convert character
	        	EscapeFlag = false;								// Clear escape flag
	        	tx_ptr++;										// Point to next character
	     	}
			else if (c < 0x20 || c == 0x7d || c == 0x7e) 		// If escape sequence required?
			{
	        	EscapeFlag = true;									// Set escape flag
	        	c = 0x7d;										// Send escape character
	     	}
			else 
			{		
	       		if (!tx_ptr)
				{
					c = 0x7e;									// Send frame character if first character of packet
					Debug_Print("\r\n\r\nSend:\r\n");
					CharCount = 0;
					EscapeFlag = false;
				}

	       		CharCount++;
				Debug_PrintHex(c);

				if (CharCount % 16 == 0)
					Debug_Print("\r\n");

				tx_ptr++;
			}

			Buffer_StoreElement(&Modem_SendBuffer, c);			// Put character in transmitter
		}
		else
		{
			// Nothing to Send or Receive
			ExitFlag = true;
		}
	}
	while (ExitFlag == 0);
}


void ProcessReceivedPacket(void)
{	
	signed int c;	

	// *****************************
	// **         LCP             **
	// *****************************
  	
	if (PacketType == LCP)
	{
		Debug_Print("Got LCP ");

		switch (rx_str[4])										// Switch on packet type
		{
			case REQ:
				RemoteReady = false;							// Clear remote ready flag
           
		   		Debug_Print("REQ ");
				
				if ((c = TestOptions(0x00c7)))					// Options 1, 2, 3, 7, 8
				{											
	            	if (c > 1)
				  	{
	                	c = ACK;								// ACK packet
                 
						RemoteReady = true;						// Set remote ready flag

						Debug_Print("- We ACK\r\n");
	              	}
					else
					{
	                	rx_str[10] = 0xc0;						// Else NAK password authentication
	                 	c = NAK;
						Debug_Print("- We NAK\r\n");
	              	}
				}
				else											// Else REJ bad options
				{
					Debug_Print("- We REJ\r\n");
			   		c = REJ;
				}
			
				TIME = 0;									// Stop the timer from expiring
	           	CreatePacket(LCP, c, rx_str[5], rx_str + 7); 	// Create LCP packet from Rx buffer
			break;
        
			case ACK:
				if (rx_str[5] == number)						// Does reply ID match the request
			   	{
					Debug_Print("ACK\r\n");
					LocalReady = true;							// Set local ready flag
				}
				else
					Debug_Print("Out of sync ACK\r\n");
			break;
        
			case NAK:
				Debug_Print("NAK\r\n");
				LocalReady = false;								// Clear local ready flag
	        break;
        
			case REJ:
				Debug_Print("REJ\r\n");
	        	LocalReady = false;								// Clear local ready flag
	        break;
        
			case TERM:
				Debug_Print("TERM\r\n");
	        break;
		}

		if (LocalReady && RemoteReady)							// When both ends ready, go to PAP state
			PPPState = PAPState;
	} 


	// *****************************
	// **         PAP             **
	// *****************************

	else if (PacketType == PAP)
	{
		Debug_Print("Got PAP ");

    	switch (rx_str[4])										// Switch on packet type
		{
	        case REQ:
				Debug_Print("REQ\r\n");				
	        break;												// Ignore incoming PAP REQ
	        case ACK:
				Debug_Print("ACK\r\n");
	           	PPPState = IPCPState;							// PAP ACK means that this state is done
	        break;
	        case NAK:
				Debug_Print("NAK\r\n");
	        break;												// Ignore incoming PAP NAK
		}
  	}

	// *****************************
	// **         IPCP            **
	// *****************************

	else if (PacketType == IPCP)
	{
		// IPCP option 0x03 IP address
		// IPCP option 0x81 Primary DNS Address
		// IPCP option 0x82 Primary NBNS Address
		// IPCP option 0x83 Secondary DNS Address
		// IPCP option 0x84 Secondary NBNS Address

		Debug_Print("Got IPCP ");

    	switch (rx_str[4])										// Switch on packet type
		{		
        	case REQ:
				Debug_Print("REQ ");	
        		
				if (TestOptions(0x0004))						// Option 3 - We got an IP address
		   		{
					c = ACK;									// We ACK
					Debug_Print("- We ACK\r\n");
           		}
				else
				{				
              		c = REJ;									// Otherwise we reject bad options
					Debug_Print("- We REJ\r\n");
           		}
           		
				CreatePacket(IPCP, c, rx_str[5], rx_str + 7);	// Create IPCP packet from Rx buffer
			break; 				
        
			case ACK:
				Debug_Print("ACK\r\n");	
				
				if (rx_str[5] == number)						// If IPCP response id matches request id
				{
					Debug_Print("**LINK CONNECTED**\r\n");
					PPPState = LCPState;						// Move into initial state for when we get called next time (after link disconnect)
					ConnectedState = CONNECTION_MANAGE_STATE_InitializeTCPStack;
           		}
			break;
       
			case NAK:   										// This is where we get our address
				Debug_Print("NAK\r\n");	
				IPAddr1 = addr1 = rx_str[10];
				IPAddr2 = addr2 = rx_str[11];   				// Store address for use in IP packets 
				IPAddr3 = addr3 = rx_str[12];
				IPAddr4 = addr4 = rx_str[13];
				
				number++;										// If we get a NAK, send the next REQ packet with a new number
				
				CreatePacket(IPCP, REQ, number, rx_str + 7);	// Make IPCP packet from Rx buffer
			break; 				

			case REJ:
				Debug_Print("REJ\r\n");	
			break;												// Ignore incoming IPCP REJ
			
			case TERM:
				Debug_Print("TERM\r\n");	
			break;												// Ignore incoming IPCP TERM
		}
	}

	else if (PacketType == IP)
	{
		Debug_Print("Got IP\r\n");									// Should never get this as we should have handed over to the TCP stack
	} 

	else if (PacketType == CHAP)								// Should never get this as we ask for PAP authentication
	{
		Debug_Print("Got CHAP\r\n");
	}

	else if (PacketType)										// Ignore any other received packet types
	{				
		Debug_Print("Got unknown packet\r\n");
  	}

	PacketType = NONE;											// Indicate that packet is processed
}


// Make the first packet to start each phase of the PPP negotiations (LCP, PAP, IPCP)
void MakeInitialPacket(void)
{
	if (tx_end)													// Don't make a new packet if we have data in the buffer already
		return;

	if (PPPState == LCPState && TIME > 300)						// Once every 3 seconds try negotiating LCP
	{		
		Debug_Print("\r\nMaking LCP Packet");
		
		TIME = 0;											// Reset timer
		number++;												// Increment ID to make packets unique

		// Request LCP options 2, 5, 7, 8, 0D, 11, 13
		CreatePacket(LCP, REQ, number, (unsigned char*)"\x12\x01\x04\x05\xa0\x02\x06\x00\x0a\x00\x00\x07\x02\x08\x02");
	}

	else if (PPPState == PAPState && TIME > 100)				// Once every second try negotiating password
	{	
		Debug_Print("\r\nMaking PAP Packet");
		
		TIME = 0;											// Reset timer
		number++;												// Increment ID to make packets unique

     	CreatePacket(PAP, REQ, number, (unsigned char*)"\x06\x00\x00"); 
  	}

	else if (PPPState == IPCPState && TIME > 500)				// Once every 5 seconds try negotiating IPCP
	{
		Debug_Print("\r\nMaking IPCP Packet");

		TIME = 0;											// Reset timer
		number++;												// Increment ID to make packets unique
		
		// Request IPCP options 3 (IP Address), 81 (Primary DNS) & 83 (Secondary DNS) with addr 0.0.0.0 for each one
		CreatePacket(IPCP, REQ, number, (unsigned char*)"\x16\x03\x06\x00\x00\x00\x00\x81\x06\x00\x00\x00\x00\x83\x06\x00\x00\x00\x00");
	}
}


// Create a packet
// The first byte of *str is the length of passed data (including length byte) + 1 byte for the other length byte + 2 bytes for the protocol
//   protocol is the packet protocol (e.g. LCP, PAP, IPCP)
//   packettype is the packet type (e.g. REQ, ACK, NAK)
//   packetID is the packet ID
//   *str is the packet data to be added after the header
// Returns the packet as a string in tx_str
void CreatePacket(unsigned int protocol, unsigned char packetType, unsigned char packetID, const unsigned char *str)
{
	unsigned int length, temp;

	tx_ptr = 1;													// Point to 2nd char in transmit buffer. 1st char is length
	tx_str[0] = ' ';											// Dummy first character. Will be overwritten by frame char (0x7e) when the packet is sent out
	checksum2 = 0xffff;											// Initialise checksum

	AddToPacket(0xff);											// Insert PPP header Oxff
	AddToPacket(0x03);											// Insert PPP header 0x03
	AddToPacket(protocol / 256);								// Insert high byte of protocol field
	AddToPacket(protocol & 255);								// Insert low byte of protocol field

	AddToPacket(packetType);									// Insert packet type (e.g. REQ, ACK, NAK)
	AddToPacket(packetID);										// Insert packet ID number

	AddToPacket(0);												// Insert MSB of length (i.e. max length = 256)
	length = (*str) - 3;										// Calculate the length of data we actually have to copy

	while (length)
	{															// Copy the whole string into packet
		length--;												// Decrement packet length
	  	AddToPacket(*str);										// Add current character to packet
		str++;													// Point to next character
	}

	temp = ~checksum2;
	AddToPacket(temp & 255);									// Insert checksum MSB
	AddToPacket(temp / 256);									// Insert checksum LSB

	tx_end = tx_ptr;											// Set end of buffer marker to end of packet
	tx_ptr = 0;													// Point to the beginning of the packet
}


// Test the option list in packet for valid options
//   option is the 16 bit field, where a 1 accepts the option one greater than the bit # (e.g. 0x0004 for option 5) 
//   returns 2 for LCP NAK, 1 is only correct fields found, and zero means bad options
//   return also modifies RX_STR to list unacceptable options if NAK or REJ required
unsigned char TestOptions(unsigned int option)
{
	unsigned int size;											// size is length of option string
	unsigned ptr1 = 8;											// ptr1 points data insert location
    unsigned ptr2 = 8;											// ptr2 points to data origin
	char pass = 3;												// pass is the return value

	size = rx_str[7] + 4;										// size is length of packet
   
	if (size > MaxRx)
		size = MaxRx;											// Truncate packet if larger than buffer

	while (ptr1 < size)											// Scan options in receiver buffer
	{
		if (rx_str[ptr1] == 3 && rx_str[ptr1+2] == 0xc2)
			pass &= 0xfd;										// Found a CHAP request, mark for NAK

		if (!((1 << (rx_str[ptr1] - 1)) & option))
	    	pass = 0;											// Found illegal options, mark for REJ

	  ptr1 += rx_str[ptr1+1];									// Point to start of next option
	}

	if (!(pass & 2))											// If marked for NAK or REJ
	{	
		if (pass & 1)											// Save state for NAK
		{
			option = 0xfffb;
		}

		for (ptr1 = 8; ptr1 < size;)
		{
			if (!((1 << (rx_str[ptr1] - 1)) & option))			// If illegal option
			{
            	for (pass = rx_str[ptr1+1]; ptr1 < size && pass; ptr1++) // Move option
				{ 
					rx_str[ptr2] = rx_str[ptr1];				// Move current byte to new storage
					ptr2++;										// Increment storage pointer
					pass--;										// Decrement number of characters
				}
			}
			else 
			{
				ptr1+=rx_str[ptr1+1];							// Point to next option
			}
		}

		rx_str[7] = ptr2 - 4;									// Save new option string length
		pass = 0;												// Restore state for REJ
		
		if (option == 0xfffb)									// Restore state for NAK
		pass = 1;
   }

   return pass;
}

// Calculate the CRC16 value
unsigned int CRC(unsigned int crcvalue, unsigned char c)
{
	return _crc_ccitt_update(crcvalue, c);

   //unsigned int b;

   //b = (crcvalue ^ c) & 0xFF;
   //b = (b ^ (b << 4)) & 0xFF;				
   //b = (b << 8) ^ (b << 3) ^ (b >> 4);
   
   //return ((crcvalue >> 8) ^ b);
}

// Add character to the new packet & update the checksum
void AddToPacket(unsigned char c)
{
	checksum2 = CRC(checksum2, c);								// Add CRC from this char to running total
  	tx_str[tx_ptr++] = c;										// Store character in the transmit buffer
}
