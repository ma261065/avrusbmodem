#ifndef _PPP_H_
#define _PPP_H_

#include "USBModem.h"
#include "Lib/Modem.h"
#include "Lib/Debug.h"
#include "util/crc16.h"

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

unsigned int TIME;											// 10 millseconds counter
#define TIME_SET(a) TIME = a								// Set 10 millisecond counter to value 'a'

unsigned int CRC(unsigned int crcvalue, unsigned char c);
void AddToPacket(unsigned char c);
void CreatePacket(unsigned int protocol, unsigned char packetType, unsigned char packetID, const unsigned char *str);
unsigned char TestOptions(unsigned int option);
void DoPPP(void);
void ProcessReceivedPacket(void);
void MakeInitialPacket(void);

#endif


