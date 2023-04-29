#ifndef ALARMA_H_
#define ALARMA_H_

#include "chip.h"

#define Amin_mm 45
#define Amax_mm 80


#define Amin 2,0 // P2[0]
#define Amax 2,2 // P2[2]

typedef enum{
	A_min,
	A_max,
	A_normal
} alarma;


void Alarmas_Inicio(void);


void Alarmas_Control(alarma);


alarma Alarmas_Comparar(uint32_t);

#endif /* ALARMA_H_ */
