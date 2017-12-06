#include "Pi85.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	unsigned int start = 0;

	if (argc > 1) start = strtol(argv[1], NULL, 16);
	unsigned int end = start + 0x7f;
	if (argc > 2) end = strtol(argv[2], NULL, 16);

	wiringPiSetup();
	if (!digitalRead(PIN_EN)) {
		printf("I don't have control of the bus.\r\n");
		return -1;
	}

	end++;	// So the end address gets included in the dump
	for (unsigned int i = start; i < end; i = i + 0x10)
	{
		printf("%4.4X: ", i);

		for (int a = 0; a < 16; a++)
		{
			printf("%2.2X ", busRead(false, i + a));
		}
		printf("\r\n");
	}

	return 0;
}
