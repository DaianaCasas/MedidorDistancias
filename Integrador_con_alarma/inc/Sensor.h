// Displays
#include "chip.h"

#define sTrigg		0,10
#define TIM3CH0	0

#define sEcho		0,4
#define CAP2CH0	0


#define sTrigg_TimePulse	((Chip_Clock_GetPeripheralClockRate(SYSCTL_PCLK_TIMER3)*3/4)/1E6)*15


#define sSTATUS_HIGH	1
#define sSTATUS_LOW		0



void sMedicion_Inicio(void);

uint32_t sMedicion_medir(uint32_t);

void sMedicion_CrearPulso(void);
