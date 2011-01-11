#include <stdlib.h>

// Each mobile network will have different parameters that must be set on the modem before connecting to their network.
// One way to determine these parameters is to run the modem under Windows and use USBMonitor 
// (http://www.hhdsoftware.com/Downloads/usb-monitor.html) to determine what commands are sent to the modem.
// The format of the list is a command, followed by the expected response to that command. The list must be double-NULL terminated.
// These dial commands are concatenated with the ones in Modem_xxx.c, and will be sent after the dial commands defined in Moden_xxx.c
// The last command should be the dial command (usually ATDT*99#), with an expected response of "CONNECT".
		
const char* NetworkDialCommands[] = 
{	
	"ATDT*99#\r\n", "CONNECT",
	NULL, NULL
};
