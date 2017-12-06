#include "Pi85.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>

FILE *theFile;

void openFile()
{	// Open a file for reading or writing
	// SHM_DATA should contain memory address of a structure as follows:
	// Byte 0: number of bytes that follow
	// Byte 1: file mode - 'r' = read, 'w' = write, 'a' = append
	// Remaining bytes: filename
	// Returns 0 in SHM_DATA for success, 1 for failure
	//
	// This is basically the CP/M "command tail" (80h) with the first byte of the tail replaced with the file mode.
	
	unsigned int addr = busRead(false, SHM_DATA) + (busRead(false, SHM_DATA + 1) << 8);	// Get address of memory structure
	int chars = busRead(false, addr);	// Number of chars to read to get filename	
	char fileName[chars--];			// Buffer to read filename into
	char mode[3];
	mode[0] = busRead(false, ++addr);	// Get open mode
	mode[1] = 'b';				// Binary mode
	mode[2] = 0;				// String terminator

	int i;
	for (i = 0; i < chars; i++)
	{	// Get the actual filename
		fileName[i] = busRead(false, ++addr);
	}
	fileName[i] = 0;	// Terminate the string

	if (theFile != NULL) fclose(theFile);	// If we already have a file open, close it.

	printf(" \"%s\" ", fileName);
	errno = 0;
	theFile = fopen(fileName, mode);	// Do the actual open operation
	printf(" %d %s ", errno, strerror(errno));

	busWrite(false, SHM_DATA, theFile == NULL);	// theFile will be NULL if we couldn't open the file for some reason
}

void closeFile()
{	// Close file. Self explanatory.
	if (theFile != NULL) fclose(theFile);
	theFile = NULL;
}

void readFile()
{	// Read 128 byte block into address at SHM_DMA - returns bytes read in SHM_DATA
	unsigned int addr = busRead(false, SHM_DMA) + (busRead(false, SHM_DATA + 1) << 8);	// Get DMA address
	unsigned char buff[128];
	
	int bytes = fread(buff, 1, 128, theFile);	// Get the bytes
	busWrite(false, SHM_DATA, bytes);

	for (int i = 0; i < bytes; i++)
	{	// Copy the bytes into memory
		busWrite(false, addr + i, buff[i]);
	}

	for (; bytes < 128; bytes++)
	{
		busWrite(false, addr + bytes, 0x1a);
	}
}

void writeFile()
{	// Reverse of readFile
	unsigned int addr = busRead(false, SHM_DMA) + (busRead(false, SHM_DATA + 1) << 8);	// Get DMA address
	unsigned char buff[128];

	for (int i = 0; i < 128; i++)
	{
		buff[i] = busRead(false, addr + i);
	}

	busWrite(false, SHM_DATA, fwrite(buff, 1, 128, theFile));
}
