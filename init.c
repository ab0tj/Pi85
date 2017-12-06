#include "Pi85.h"
#include <stdio.h>
#include <unistd.h>

int main()
{
	printf("Setting initial GPIO values...\r\n");
	Pi85_init();
	printf("Resetting 8085 and ethernet...\r\n");
	usleep(100000);	// Pi85_init sets reset lines, let them stay that way for 100ms
	digitalWrite(PIN_BRQ, HIGH);	// Don't let the 8085 take the bus when we release it from reset
	digitalWrite(PIN_85_RST, HIGH);
	digitalWrite(PIN_ETH_RST, HIGH);
	printf("Requesting bus control...\r\n");
	takeBus();

	return 0;
}
