#include "disk.h"
#include "Pi85.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

/* Functions to handle disk-related requests */

FILE *diskImage[16];
unsigned char selectedDisk;

void readSector()
{	// Get params from memory and read the specified secotr into memory
	unsigned int track = busRead(false, SHM_TRACK) + (busRead(false, SHM_TRACK + 1) << 8);
	unsigned int sector = busRead(false, SHM_SECT) + (busRead(false, SHM_SECT + 1) << 8);
	unsigned int addr = busRead(false, SHM_DMA) + (busRead(false, SHM_DMA + 1) << 8);
	busWrite(false, SHM_DATA, !doReadSector(selectedDisk, track, sector, addr));	// 0 = success
}

int doReadSector(int disk, unsigned int track, unsigned int sector, unsigned int addr)
{	// Function to do the sector transfer that can be called from other functions
	unsigned char buff[128];
	
	//printf("R: D-%d T-%d S-%d M-%4.4X\n", selectedDisk, track, sector, addr);
	if (diskImage[disk] == NULL || fseek(diskImage[disk], ((track * 32) + (sector - 1)) * 128, SEEK_SET) != 0) return false; // couldn't seek
	
	if (fread(buff, 128, 1, diskImage[disk]) == 1)
	{	// Transfer the sector into memory if we were able to read it from the image file
		for (int i = 0; i < 128; i++)
		{
			busWrite(false, addr + i, buff[i]);
		}
		return true;
	}
	else return false;
}

void writeSector()
{	// Write a sector from memory to the image file - only works from values in memory as is
	// works pretty much like readSector, but in reverse
	unsigned int track = busRead(false, SHM_TRACK) + (busRead(false, SHM_TRACK + 1) << 8);
	unsigned int sector = busRead(false, SHM_SECT) + (busRead(false, SHM_SECT + 1) << 8);
	unsigned int addr = busRead(false, SHM_DMA) + (busRead(false, SHM_DMA + 1) << 8);
	unsigned char buff[128];

	//printf(" W: D-%d T-%d S-%d M-%4.4X ", selectedDisk, track, sector, addr);
	if (diskImage[selectedDisk] == NULL || fseek(diskImage[selectedDisk], ((track * 32) + (sector - 1)) * 128, SEEK_SET) != 0)	// couldn't seek
	{
		busWrite(false, SHM_DATA, 1);	// error flag
		return;
	}

	for (int i = 0; i < 128; i++)
	{
		buff[i] = busRead(false, addr+i);
	}

	busWrite(false, SHM_DATA, (fwrite(buff, 128, 1, diskImage[selectedDisk]) != 1));
}

void selectDisk()
{	// Change the selected disk "drive" - signal an error if we can't
	unsigned char newDisk = busRead(false, SHM_DATA);

	//printf(" S:%d ", newDisk);
	if (newDisk < 16 && diskImage[newDisk] != NULL)	// Image is loaded
	{
		selectedDisk = newDisk;
		busWrite(false, SHM_DATA, 0);	// No error
	}
	else
	{
		busWrite(false, SHM_DATA, 1);	// Error
	}
}

void loadImage(int diskNo, char* filename)
{	// Put a virtual disk into the virtual drive
	if (diskNo > 15 || diskNo < 0)
	{
		printf("Invalid disk number.\n");
		return;
	}
	
	if (diskImage[diskNo] != NULL) fclose(diskImage[diskNo]);	// Close the image file if we already have one loaded

	diskImage[diskNo] = fopen(filename,"rb+");	// Open image file with 'update' access
	if (diskImage[diskNo] == NULL)
	{
		printf("Could not open %s: %s\n", filename, strerror(errno));
	}
	else
	{
		printf("Loaded %s in drive %d\n", filename, diskNo);
	}
}

int isImageLoaded(int disk)
{	// Check if a drive has a disk
	return diskImage[disk] != NULL;
}

void startupDisks()
{	// Call at startup to load initial disks
	if (access("0.dsk", F_OK) != -1) loadImage(0, "0.dsk");
	if (access("1.dsk", F_OK) != -1) loadImage(1, "1.dsk");
	if (access("2.dsk", F_OK) != -1) loadImage(2, "2.dsk");
	if (access("3.dsk", F_OK) != -1) loadImage(3, "3.dsk");
}
