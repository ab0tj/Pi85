#include "Pi85.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "console.h"
#include "disk.h"

void initHW(void);
void getCommand(void);
void dump(char*, char*);
void fill(char*, char*, char*);
void inputToMem(unsigned int);
void ioIn(char*);
void ioOut(char*, char*);
void getHex(char*);
unsigned int substrhex(char*, int, int);
void showStatus(void);
void reset(void);
void boot(void);
void memTest(unsigned char);
void cpuTest(void);
void cleanUp();

int main()
{	// Get everything ready to go and then process commands from the user
	initHW();	// Set up GPIO, etc
	signal(SIGINT, cleanUp);	// Catch Ctrl-C and do some cleanup
	wiringPiISR(PIN_REQ, INT_EDGE_RISING, &intHandler);	// Register ISR
	tcgetattr(STDIN_FILENO, &old_tio);	// Save termios so we can restore them later
	startupDisks();		// Attach 0.dsk, 1.dsk, etc
	printf("\nWelcome to Pi85!\n\n");
	
	for (;;)
	{
		getCommand();	// Main loop to process user commands
	}

	return 0;
}

void initHW()
{	// Set up the bus interface and stuff
	printf("Setting initial GPIO values...\n");
	Pi85_init();
	printf("Resetting 8085 and ethernet...\n");
	usleep(100000);			// Pi85_init sets reset lines, let them stay that way for 100ms
	digitalWrite(PIN_BRQ, HIGH);	// Don't let the 8085 take the bus when we release it from reset
	digitalWrite(PIN_85_RST, HIGH);	// Let go of reset or HLDA will never get set and we'll be waiting for it forever
	digitalWrite(PIN_ETH_RST, HIGH);
	printf("Requesting bus control...\n");
	takeBus();

	printf("Testing memory...\n");
	memTest(0xff);	// Test with FF's
	memTest(0x00);	// Test with 00's

	printf("Testing 8085 interface...\n");
	cpuTest();	// Make sure the 8085 is listening
}

void getCommand()
{	// Monitor interface routine
	printf("Pi85>");

	char buff[128];	// Command line buffer
	fgets(buff, sizeof(buff), stdin);
	buff[strcspn(buff, "\n")] = 0;	// Terminate the string at the first newline
	if (strlen(buff) == 0) return;	// User didn't actually enter anything
	char* cmd = strtok(buff, " ");	// Split the string into a command and up to 3 arguments
	char* arg1 = strtok(NULL, " ");
	char* arg2 = strtok(NULL, " ");
	char* arg3 = strtok(NULL, " ");

	switch (cmd[0])
	{	// Process the command 
		case 'x':	// Exit
			cleanUp();
			break;
		
		case 'd':	// Dump from memory
			dump(arg1, arg2);
			break;
		
		case 'f':	// Fill memory
			fill(arg1, arg2, arg3);
			break;

		case 'o':	// Output to IO port
			ioOut(arg1, arg2);
			break;

		case 'i':	// Input from IO port
			ioIn(arg1);
			break;

		case ':':	// Intel hex record
			getHex(cmd);
			break;

		case 's':	// Show status of control signals
			showStatus();
			break;

		case 'r':	// Reset the 8085 and ethernet module
			reset();
			break;

		case 'g':	// Let the 8085 run without resetting it
			doConsole(false, -1);
			break;

		case 't':	// Grab the bus. TODO: Is this necessary anymore?
			takeBus();
			break;

		case 'b':	// Boot the 8085, optionally from disk
			if (arg1 == NULL)
			{
				doConsole(true, -1);
			}
			else
			{
				doConsole(true, strtol(arg1, NULL, 10));
			}
			break;

		case 'l':	// Load a disk image into the specified drive
			if (arg1 != NULL && arg2 != NULL) loadImage(strtol(arg1, NULL, 10), arg2);
			break;

		default:	// Something else. Nice try, user.
			printf("Unknown command.\r\n");
			break;
	}
	return;
}

void dump(char* cstart, char* cend)
{	// Hex dump from 8085's memory space
	if (!haveBusControl()) return;	// We can't access the 8085's memory right now

	char buff[16];	// Line buffer
	unsigned int start = 0;	// Start at address 0 unless otherwise specified
	if (cstart != NULL) start = strtol(cstart, NULL, 16);
	unsigned int end = start + 0x7f;	// Dump 128 bytes unless told otherwise
	if (cend != NULL) end = strtol(cend, NULL, 16);
	if (end > 0xffff) end = 0xffff;	// Nice try but memory doesn't exist there

	end++;	// So the end address is included in the dump
	for (unsigned int a = start; a < end; a += 0x10)
	{	// Do the dump, 16 bytes at a time
		for (unsigned int i = 0; i < 16; i++)
		{	// Read the bytes from memory
			buff[i] = busRead(false, a + i);
		}

		printf("%4.4X: ", a);	// Print the starting address of this line
		for (unsigned int i = 0; i < 16; i++)
		{	// Print the bytes in hex form
			printf("%2.2X ", buff[i]);
		}

		printf("  ");
		for (unsigned int i = 0; i < 16; i++)
		{	// Print the bytes in ASCII form
			if (buff[i] > 31 && buff[i] < 127)	// But only if they're printable
			{
				printf("%c", buff[i]);
			}
			else
			{
				printf(".");
			}
		}
		printf("\n");
	}
}

void fill(char* cstart, char* arg2, char* cval)
{	// Put some bytes into memory
	if (!haveBusControl()) return;	// We can't do that right now

	unsigned int start = 0;
	unsigned int end = 0;
	unsigned char val = 0;

	int args = 0;
	if (cstart != NULL) args++;	// User specified start address
	if (arg2 != NULL) args++;	// This is either the byte value or an end address
	if (cval != NULL) args++;	// 3 arguments means start end and value were specified

	if (args > 0) start = strtol(cstart, NULL, 16);	// Start address
	end = start;					// Just one byte unless told otherwise
	if (args == 2) val = strtol(arg2, NULL, 16);	// 2 arguments means addr, val
	if (args > 2)					// Start, end, val
	{
		end = strtol(arg2, NULL, 16);
		val = strtol(cval, NULL, 16);
	}

	if (end > 0xffff) end = 0xffff;	// Only have memory up to ffff

	if (args < 2)	// No args or only start was specified, so start interactive memory input thing
	{
		inputToMem(start);
		return;
	}
	else
	{	// We have enough info to just do the fill and return
		for (unsigned int a = start; a <= end; a++)	// Loop through the addresses
		{
			busWrite(false, a, val);	// And write value to memory there
		}
	}	
}

void inputToMem(unsigned int start)
{	// Input to memory a byte at a time from user input
	char hex[5];
	for (;;)
	{
		if (start > 0xffff) start = 0;	// Loop back to zero if we reached the end of memory space
		printf("%4.4X: %2.2X=", start, busRead(false, start));	// Print address
		fflush(stdin);	// Won't print otherwise because we didn't include a newline
		fgets(hex, 5, stdin);	// Get hex value from the user
		if (hex[0] == 0x0a) return;	// User hit enter with no input
		busWrite(false, start, strtol(hex, NULL, 16));	// Write the byte to memory
		start++;	// Increment the memory address
	}
}

void ioIn(char* port)
{	// Grab a value from an IO port
	if (!haveBusControl()) return;
	if (port == NULL) return;	// Helps to know where to read from
	printf("%2.2X\r\n", busRead(true, strtol(port, NULL, 16)));
}

void ioOut(char* port, char* val)
{	// Output val to port
	if (!haveBusControl()) return;
	if (port == NULL || val == NULL) return;	// Can't do much without both args
	busWrite(true, strtol(port, NULL, 16), strtol(val, NULL, 16));
}

void showStatus()
{	// Print the current value of some control lines
	printf("85_RST:  %i\r\n", digitalRead(PIN_85_RST));
	printf("ETH_RST: %i\r\n", digitalRead(PIN_ETH_RST));
	printf("BRQ:     %i\r\n", digitalRead(PIN_BRQ));
	printf("EN:      %i\r\n", digitalRead(PIN_EN));
	printf("INT:     %i\r\n", digitalRead(PIN_INT));
	printf("REQ:     %i\r\n", digitalRead(PIN_REQ));
	printf("BP:      %i\r\n", digitalRead(PIN_BP));
	printf("BP_EN:   %i\r\n", digitalRead(PIN_BP_EN));
}

void getHex(char* buff)
{	// Intel hex into 8085 memory space
	if (!haveBusControl()) return;
	unsigned int count = substrhex(buff, 1, 2);	// Number of bytes in data field
	unsigned char cksum = count;			// Start of running checksum value
	cksum += substrhex(buff, 3, 2);			// Add two bytes of address field to checksum
	cksum += substrhex(buff, 5, 2);
	unsigned int addr = substrhex(buff, 3, 4);	// Starting address
	unsigned int type = substrhex(buff, 7, 2);	// Record type
	cksum += type;
	unsigned char data[count];			// Data buffer

	for (unsigned int i = 0; i < count; i++)
	{	// Load data bytes into buffer
		data[i] = substrhex(buff, (i * 2) + 9, 2);
		cksum += data[i];
	}

	cksum += substrhex(buff, (count * 2) + 9, 2);	// Checksum byte.
	if (cksum != 0)		// chksum + running checksum should make a zero
	{
		printf(" Checksum error!\r\n", cksum);
		return;
	}

	if (type == 0)	// Data record
	{
		printf(" OK\r\n");
		for (unsigned int i = 0; i < count; i++)
		{	// Load data into memory
			busWrite(false, addr + i, data[i]);
		}
	}
	else if (type == 1)	// EOF record
	{
		printf(" EOF\r\n");
	}
	else	// The other record types don't make any sense for this system
	{
		printf(" Unsupported record type\r\n");
	}
}

unsigned int substrhex(char* src, int index, int length)
{	// Return a binary value from ascii substring
	if (length > 4) return 0;	// Max 16 bits
	char temp[5];

	for (int i = 0; i < length; i++)
	{	// Copy substring into temporary buffer
		temp[i] = src[index + i];
	}
	temp[length] = 0;	// Terminate the resulting string

	return strtol(temp, NULL, 16);	// And return whatever strtol thinks that makes
}

void reset()
{	// Reset the 8085 and ethernet module
	digitalWrite(PIN_85_RST, LOW);
	digitalWrite(PIN_ETH_RST, LOW);
	usleep(100000);
	digitalWrite(PIN_85_RST, HIGH);
	digitalWrite(PIN_ETH_RST, HIGH);
}

void memTest(unsigned char val)
{	// Fill memory with the specified value and verify that we can read it back successfully
	for (unsigned long a = 0; a < 0x10000; a++)
	{
		busWrite(false, a, val);
	}

	for (unsigned long a = 0; a < 0x10000; a++)
	{
		if (busRead(false, a) != val)
		{
			printf("Error at %4.4X\r\n", a);
		}
	}
}

void cpuTest()
{
	/* mvi a, 10h
	 * adi 08h
	 * sta 0ffffh
	 * out 0
	 * hlt */
	const unsigned char testProg[] = { 0x3e, 0x10, 0xc6, 0x08, 0x32, 0xff, 0xff, 0xd3, 0x00, 0x76 };

	/* Copy the test program into memory */
	for (unsigned int a = 0; a < 10; a++)
	{
		busWrite(false, a, testProg[a]);
	}

	/* Reset the 8085 and let it run */
	digitalWrite(PIN_85_RST, LOW);
	giveBus();
	digitalWrite(PIN_85_RST, HIGH);

	/* Wait for the 8085 to signal it is done */
	unsigned long loops = 0;
	while (!digitalRead(PIN_REQ))
	{
		usleep(100);
		if (loops++ > 10000)	// About 1 sec
		{
			printf ("Timed out waiting for PI_REQ!\n");
		}
	}
	usleep(10);

	/* Take back the bus and ack the request from the 8085 */
	takeBus();
	digitalWrite(PIN_ACK, HIGH);
	digitalWrite(PIN_ACK, LOW);
	
	/* 8085 should have written 0x18 to memory address 0xffff */
	if (busRead(false, 0xffff) != 0x18)
	{
		printf("CPU test failed! Expected 0x18, got 0x%2.2X\n", busRead(false, 0xffff));
		return;
	}

	/* Zero out the test program */
	for (unsigned int a = 0; a < 10; a++)
	{
		busWrite(false, a, 0);
	}
	busWrite(false, 0xffff, 0);
}

void cleanUp()
{
	printf("\nGoodbye!\n");
	takeBus();
	tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
	exit(0);
}
