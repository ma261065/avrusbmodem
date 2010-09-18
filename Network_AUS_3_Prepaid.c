#include <stdlib.h>

// Each mobile network will have different parameters that must be set on the modem before connecting to their network.
// One way to determine these parameters is to run the modem under Windows and use USBMonitor 
// (http://www.hhdsoftware.com/Downloads/usb-monitor.html) to determine what commands are sent to the modem.
// These dial commands are concatenated with the ones in Network_xxx.c, and will be sent after the
// dial commands defined in Moden_xxx.c
// The last command should be ATDT*99# and the list must be NULL terminated
		
const char* NetworkDialCommands[] = 
{	
	"AT+CGDCONT=1,\"IP\",\"3services\",,0,0\r\n", "OK",
	"ATDT*99#\r\n", "CONNECT",
	NULL, NULL
};
