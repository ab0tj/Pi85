#include "Pi85.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		printf("Usage: fill start [end] byte\r\n");
		return -1;
	}

	unsigned int start = strtol(argv[1], NULL, 16);
	unsigned int end = start;
	unsigned char val = 0;

	if (argc > 3)
	{
		end = strtol(argv[2], NULL, 16);
		val = strtol(argv[3], NULL, 16);
	} else {
		val = strtol(argv[2], NULL, 16);
	}

	if (start > end)
	{
		printf("end should be greater than start.\r\n");
		return -1;
	}

	wiringPiSetup();
	if (!digitalRead(PIN_EN))
	{
		printf("I don't have control of the bus!\r\n");
		return -1;
	}

	for (unsigned int i = start; i <= end; i++)
	{
		busWrite(false, i, val);
	}

	return 0;
}
