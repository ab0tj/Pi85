#include <unistd.h>
#include <stdio.h>
#include "Pi85.h"

/* Low level interface functions for the Pi85 system */

void Pi85_init(void)
{	// Set up GPIO to do our bidding
	// Initialize wiringPi
	wiringPiSetup();

	// Get all the GPIOs to their initial state
	giveBus();				// Set bus lines to input
	pullUpDnControl(PIN_AD0, PUD_DOWN);	// Pull down to help keep the 8085's outputs below 3.3v
	pullUpDnControl(PIN_AD1, PUD_DOWN);
	pullUpDnControl(PIN_AD2, PUD_DOWN);
	pullUpDnControl(PIN_AD3, PUD_DOWN);
	pullUpDnControl(PIN_AD4, PUD_DOWN);
	pullUpDnControl(PIN_AD5, PUD_DOWN);
	pullUpDnControl(PIN_AD6, PUD_DOWN);
	pullUpDnControl(PIN_AD7, PUD_DOWN);
	pullUpDnControl(PIN_IOM, PUD_DOWN);
	pullUpDnControl(PIN_RD, PUD_DOWN);
	pullUpDnControl(PIN_WR, PUD_DOWN);
	digitalWrite(PIN_85_RST, LOW);		// Directions and inital states for control lines
	pinMode(PIN_85_RST, OUTPUT);
	digitalWrite(PIN_ETH_RST, LOW);
	pinMode(PIN_ETH_RST, OUTPUT);
	digitalWrite(PIN_ALE_HI, LOW);
	pinMode(PIN_ALE_HI, OUTPUT);
	digitalWrite(PIN_ALE_LO, LOW);
	pinMode(PIN_ALE_LO, OUTPUT);
	digitalWrite(PIN_BRQ, LOW);
	pinMode(PIN_BRQ, OUTPUT);
	pinMode(PIN_EN, INPUT);
	digitalWrite(PIN_INT, LOW);
	pinMode(PIN_INT, OUTPUT);
	digitalWrite(PIN_ACK, LOW);
	pinMode(PIN_ACK, OUTPUT);
	pinMode(PIN_REQ, INPUT);
	pinMode(PIN_BP, INPUT);
	digitalWrite(PIN_BP_EN, LOW);
	pinMode(PIN_BP_EN, OUTPUT);
	digitalWrite(PIN_BP_RST, HIGH);		// Reset any pending breakpoint while we're at it
	pinMode(PIN_BP_RST, OUTPUT);
	digitalWrite(PIN_BP_RST, LOW);
	digitalWrite(PIN_BP_LE, LOW);
	pinMode(PIN_BP_LE, OUTPUT);
}

bool haveBusControl()
{	// Check if we have control of the bus
	if (!digitalRead(PIN_EN))
	{
		printf("I don't have control of the bus!\r\n");
		return false;
	}
	return true;
}

void giveBus(void)
{	/* Give the shared bus back to the 8085 */
	pinMode(PIN_AD0, INPUT);
	pinMode(PIN_AD1, INPUT);
	pinMode(PIN_AD2, INPUT);
	pinMode(PIN_AD3, INPUT);
	pinMode(PIN_AD4, INPUT);
	pinMode(PIN_AD5, INPUT);
	pinMode(PIN_AD6, INPUT);
	pinMode(PIN_AD7, INPUT);
	pinMode(PIN_IOM, INPUT);
	pinMode(PIN_RD, INPUT);
	pinMode(PIN_WR, INPUT);
	digitalWrite(PIN_BRQ, LOW);
}

void takeBus(void)
{	/* Request and then take control of shared bus */
	digitalWrite(PIN_BRQ, HIGH);	// Ask for bus control
	while(!digitalRead(PIN_EN));	// Wait for bus control
	pinMode(PIN_IOM, OUTPUT);	// Take control of bus control signals
	digitalWrite(PIN_RD, HIGH);
	pinMode(PIN_RD, OUTPUT);
	digitalWrite(PIN_WR, HIGH);
	pinMode(PIN_WR, OUTPUT);
}

void setBusMode(int mode)
{	// Set AD bus to input or output
	if (!digitalRead(PIN_EN)) {	// We don't have control of the bus!
		giveBus();		// Make sure we're got trying to use it
		return;
	}

	pinMode(PIN_AD0, mode);
	pinMode(PIN_AD1, mode);
	pinMode(PIN_AD2, mode);
	pinMode(PIN_AD3, mode);
	pinMode(PIN_AD4, mode);
	pinMode(PIN_AD5, mode);
	pinMode(PIN_AD6, mode);
	pinMode(PIN_AD7, mode);
}

void setData(unsigned char data)
{	// Output a byte on AD bus
	digitalWrite(PIN_AD0, (data & 0x01));
	digitalWrite(PIN_AD1, (data & 0x02));
	digitalWrite(PIN_AD2, (data & 0x04));
	digitalWrite(PIN_AD3, (data & 0x08));
	digitalWrite(PIN_AD4, (data & 0x10));
	digitalWrite(PIN_AD5, (data & 0x20));
	digitalWrite(PIN_AD6, (data & 0x40));
	digitalWrite(PIN_AD7, (data & 0x80));
}

void setAddress(unsigned int addr)
{	// Put the specified address into the address latches
	// Note: glue logic prevents us from setting ALE_LO if we don't have bus control
	setData(addr >> 8);
	digitalWrite(PIN_ALE_HI, HIGH);
	digitalWrite(PIN_ALE_HI, LOW);
	setData(addr);
	digitalWrite(PIN_ALE_LO, HIGH);
	digitalWrite(PIN_ALE_LO, LOW);
}

void busWrite(bool io, unsigned int addr, unsigned char data)
{	// Write a byte to the specified IO port or memory address
	setBusMode(OUTPUT);
	// Safe to do the rest because setBusMode will hand the bus back if PI_EN isn't set
	digitalWrite(PIN_IOM, io);
	setAddress(addr);
	setData(data);
	digitalWrite(PIN_WR, LOW);
	digitalWrite(PIN_WR, HIGH);
	setBusMode(INPUT);
}

unsigned char busRead(bool io, unsigned int addr)
{	// Read a byte from the specified IO port or memory address
	unsigned char data = 0;
	digitalWrite(PIN_IOM, io);
	setBusMode(OUTPUT);	// So we can talk to the address latches
	setAddress(addr);
	setBusMode(INPUT);	// So we can read in the byte
	digitalWrite(PIN_RD, LOW);
	digitalRead(PIN_AD0);	// Short delay to allow stuff to respond
	data |= digitalRead(PIN_AD0);
	data |= (digitalRead(PIN_AD1) << 1);
	data |= (digitalRead(PIN_AD2) << 2);
	data |= (digitalRead(PIN_AD3) << 3);
	data |= (digitalRead(PIN_AD4) << 4);
	data |= (digitalRead(PIN_AD5) << 5);
	data |= (digitalRead(PIN_AD6) << 6);
	data |= (digitalRead(PIN_AD7) << 7);
	digitalWrite(PIN_RD, HIGH);

	return data;
}
