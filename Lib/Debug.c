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

#include "Debug.h"

bool DebugModeEnabled = false;

void Debug_PrintChar(const char DebugChar)
{
	if (DebugModeEnabled)
	  putchar(DebugChar);
}

void Debug_Print(const char* DebugText)
{
	if (DebugModeEnabled)
	{
		while (*DebugText)
		  putchar(*(DebugText++));
	}
}

void Debug_PrintHex(const uint8_t DebugChar)
{
	if ((DebugChar >> 4) > 9)
	  Debug_PrintChar((DebugChar >> 4) + 'a' - 10);
	else
	  Debug_PrintChar((DebugChar >> 4) + '0');

	if ((DebugChar & 0x0f) > 9)
		Debug_PrintChar((DebugChar & 0x0f) + 'a' - 10);
	else
		Debug_PrintChar((DebugChar & 0x0f) + '0');
}
