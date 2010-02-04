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

#ifndef _MODEM_H_
#define _MODEM_H_

#include "USBModem.h"
#include <stdbool.h>

#define MODEM_RX_BUFFER_MASK (MODEM_RX_BUFFER_SIZE - 1)
#define MODEM_TX_BUFFER_MASK (MODEM_TX_BUFFER_SIZE - 1)
#define MODEM_NO_DATA          0x0100              // No receive data available
#define MODEM_BUFFER_OVERFLOW  0x0200			   // Buffer is full

// Size of the circular receive buffer, must be power of 2
#ifndef MODEM_RX_BUFFER_SIZE
#define MODEM_RX_BUFFER_SIZE 256
#endif

// Size of the circular transmit buffer, must be power of 2
#ifndef MODEM_TX_BUFFER_SIZE
#define MODEM_TX_BUFFER_SIZE 256
#endif

extern void modem_init(void);
extern unsigned int modem_getc(void);
extern void modem_putc(unsigned char data);
extern void modem_puts(const char *s);
extern unsigned int modem_getTxBuffer(unsigned char *s, int maxSize);
extern unsigned int modem_putRxBuffer(unsigned char *s, int size);
extern bool modem_TxBufferEmpty(void);
extern bool modem_RxBufferEmpty(void);

#endif
