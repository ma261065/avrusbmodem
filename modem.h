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
