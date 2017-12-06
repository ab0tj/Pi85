#include <wiringPi.h>

/* Pin definitions (wiringPi numbering) */
#define PIN_AD0		27
#define PIN_AD1		0
#define PIN_AD2		1
#define PIN_AD3		24
#define PIN_AD4		28
#define PIN_AD5		29
#define PIN_AD6		3
#define PIN_AD7		4
#define PIN_85_RST 	5
#define PIN_ETH_RST	6
#define PIN_ALE_HI	25
#define PIN_ALE_LO	2
#define PIN_IOM		8
#define PIN_RD		9
#define PIN_WR		7
#define PIN_BRQ		21
#define PIN_EN		22
#define PIN_INT		11
#define PIN_ACK		10
#define PIN_REQ		13
#define PIN_BP		12
#define PIN_BP_RST	14
#define PIN_BP_EN	26
#define PIN_BP_LE	23

/* Configuration constants */
#define SHM_BASE	0xfff0		// Memory location where SHM area starts
#define SHM_CHRIN	SHM_BASE	// 1 byte:  Character input from console. 0 if no character is ready.
#define SHM_CMD		SHM_BASE + 1	// 1 byte:  Command number
#define SHM_DATA	SHM_BASE + 2	// 2 bytes: Data associated with command
#define SHM_TRACK	SHM_BASE + 4	// 2 bytes: Selected disk track
#define SHM_SECT	SHM_BASE + 6	// 2 bytes: Selected disk sector
#define SHM_DMA		SHM_BASE + 8	// 2 bytes: DMA address for disk transfer

typedef enum { false, true } bool;

/* Functions */
void Pi85_init(void);
bool haveBusControl(void);
void giveBus(void);
void takeBus(void);
void setBusMode(int);
void setData(unsigned char);
void setAddress(unsigned int);
void busWrite(bool, unsigned int, unsigned char);
unsigned char busRead(bool, unsigned int);
