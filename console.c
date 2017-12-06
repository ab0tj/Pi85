#include "Pi85.h"
#include "console.h"
#include "disk.h"
#include "file.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>

/* Code to act like a console and do any other things the 8085 asks us to do */

bool running;

void printStr(void);

void intHandler()
{	// PI_REQ flag was set, see what the 8085 wants and then do it
	bool shouldReturnBus = !digitalRead(PIN_EN);	// Did the 8085 have bus control? Probably, but check anyway.

	if (shouldReturnBus) takeBus();
	unsigned char cmd = busRead(false, SHM_CMD);	// Get the command byte

	//printf(" C:%d ", cmd);

	switch (cmd)
	{
		case 0:		// Return to monitor
			running = false;
			shouldReturnBus = false;
			break;

		case 1:		// Print char
			fputc(busRead(false, SHM_DATA), stdout); //printf("%c", busRead(false, SHM_DATA)); (fputc seems to be a little faster)
			break;

		case 2:		// Print string
			printStr();
			break;

		case 4:		// Read disk sector
			readSector();
			break;

		case 5:		// Write disk sector
			writeSector();
			break;

		case 6:		// Select disk
			selectDisk();
			break;

		case 7:		// Open file
			openFile();
			break;

		case 8:		// Close file
			closeFile();
			break;

		case 9:		// Read from file
			readFile();
			break;

		case 10:	// Write to file
			writeFile();
			break;

		default:	// Dunno.
			break;
	}

	digitalWrite(PIN_ACK, HIGH);	// Reset the PI_REQ flag
	digitalWrite(PIN_ACK, LOW);
	if (shouldReturnBus) giveBus();
}

void doConsole(bool doReset, int bootDisk)
{	// Hand over control to the 8085 and do what it tells us to do
	if (doReset && bootDisk != -1)	// Boot from disk
	{
		if (!isImageLoaded(bootDisk))
		{
			printf("There is no disk in drive %d!\n", bootDisk);
			return;
		}

		if (!doReadSector(bootDisk, 0, 1, 0))	// Load track 0 sector 1 into memory at location 0
		{
			printf("Error reading boot sector from disk %d!\n", bootDisk);
			return;
		}
	}

	struct termios new_tio;	// Set up stdin for unbuffered input
	new_tio = old_tio;
	new_tio.c_lflag &=(~ICANON & ~ECHO & ~ISIG);
	tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

	printf("Starting console. Ctrl-@ to exit.\n\n");

	if (doReset) digitalWrite(PIN_85_RST, LOW);	// Reset the 8085 and hand over control
	giveBus();
	if (doReset) digitalWrite(PIN_85_RST, HIGH);

	running = true;
	int count;

	while (running)
	{
		ioctl(STDIN_FILENO, FIONREAD, &count);	// Check for chars waiting in stdin
		if (count != 0 && !digitalRead(PIN_EN))	// Write to SHM_CHRIN if 8085 is running
		{
			char c = getc(stdin);
			//printf(" %2.2X ", c);
			if (c == 0) break;		// Ctrl-@
			if (c == 0x7f) c = 8;		// Translate backspace
			if (c == '\n') c = '\r';	// CR seems to work in all cases where LF doesn't always
			takeBus();
			busWrite(false, SHM_CHRIN, c);
			giveBus();
		}
		fflush(stdout);	// Flush stdout so any pending output is displayed quickly
		if (count == 0) usleep(10000);	// Delay for about 10ms before looping again so we don't hog all the Pi's CPU time
	}

	takeBus();	// Exiting the above loop means it is time to return to the monitor

	tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);	// Set stdin back to buffered input
	printf("\n");	// Line break to keep everything in order
}

void printStr()
{	// Prints a null-terminated string found at the address in SHM_DATA
	unsigned int addr = busRead(false, SHM_DATA);
	addr += busRead(false, SHM_DATA + 1) << 8;

	char temp;
	while(temp = busRead(false, addr++))
	{
		printf("%c", temp);
	}
}
