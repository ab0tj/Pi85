#include <termios.h>

void intHandler(void);
void doConsole(bool, int);

struct termios old_tio;
