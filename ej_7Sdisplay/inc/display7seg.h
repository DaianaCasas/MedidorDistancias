// Displays
#include "chip.h"

#define BCDA		2,7
#define BCDB		1,29
#define BCDC		4,28
#define BCDD		1,23
#define DP			1,20
#define CLK			0,19
#define RST			3,26

#define DIGITOS 6

void S7Display_Inicio(void);
uint8_t * S7Display_conversionvalor(uint32_t);
