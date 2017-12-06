#include "Pi85.h"

int main()
{
	wiringPiSetup();
	digitalWrite(PIN_85_RST, LOW);
	giveBus();
	digitalWrite(PIN_85_RST, HIGH);
	return 0;
}
