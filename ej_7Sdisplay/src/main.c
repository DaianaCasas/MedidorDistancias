#include <display7seg.h>
#include "chip.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define LEDSTICK 0,22

xQueueHandle cola;

void InicioLeds(void){
	//####### LEDS #########
	Chip_IOCON_PinMux(LPC_IOCON, LEDSTICK, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, LEDSTICK);
	Chip_GPIO_SetPinState(LPC_GPIO,LEDSTICK, false);
}

void S7Display_Inicio(void)
{
	// ###### SEGMENTOS #######
	Chip_IOCON_PinMux(LPC_IOCON, BCDA, IOCON_MODE_PULLUP, IOCON_FUNC0);
	Chip_IOCON_PinMux(LPC_IOCON, BCDB, IOCON_MODE_PULLUP, IOCON_FUNC0);
	Chip_IOCON_PinMux(LPC_IOCON, BCDC, IOCON_MODE_PULLUP, IOCON_FUNC0);
	Chip_IOCON_PinMux(LPC_IOCON, BCDD, IOCON_MODE_PULLUP, IOCON_FUNC0);
	Chip_IOCON_PinMux(LPC_IOCON, DP, IOCON_MODE_PULLUP, IOCON_FUNC0);
	Chip_IOCON_PinMux(LPC_IOCON, CLK, IOCON_MODE_PULLUP, IOCON_FUNC0);
	Chip_IOCON_PinMux(LPC_IOCON, RST, IOCON_MODE_PULLUP, IOCON_FUNC0);


	Chip_GPIO_SetPinDIROutput(LPC_GPIO, BCDA);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, BCDB);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, BCDC);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, BCDD);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, DP);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, CLK);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, RST);


	// Secuencia de Apagados de Segmentos
	Chip_GPIO_SetPinState(LPC_GPIO, BCDA, true);
	Chip_GPIO_SetPinState(LPC_GPIO, BCDB, true);
	Chip_GPIO_SetPinState(LPC_GPIO, BCDC, true);
	Chip_GPIO_SetPinState(LPC_GPIO, BCDD, true);
	Chip_GPIO_SetPinState(LPC_GPIO, DP, false);
	Chip_GPIO_SetPinState(LPC_GPIO, CLK, false);
	Chip_GPIO_SetPinState(LPC_GPIO, RST, false);


}


uint8_t * S7Display_conversionvalor(uint32_t value){
	uint8_t i; // indice
	uint8_t auxDisplay[DIGITOS]={0,0,0,0,0,0}; // inicializo los digitos
	uint32_t auxValue = 0; // copia del valor a mostrar en display
	uint32_t sendedValue = auxDisplay; // conversion a una sola variable

	auxValue = value; 	// inicializo copia
	for(i=0;i<DIGITOS;i++){ 	// inicializo los digitos
		auxDisplay[i] = 0;
	}
	for (i=(DIGITOS-1); i; i--){  // separo por digito
		auxDisplay[i] = auxValue%10;
		auxValue /= 10;
	}
	auxDisplay[0] = auxValue;
	return auxDisplay;
}

static void Task_Generacion_Valores(void *pvParameters){
	uint8_t i; // indice
	uint8_t auxDisplay[DIGITOS]={0,0,0,0,0,0}; // inicializo los digitos
	uint32_t auxValue = 0; // copia del valor a mostrar en display
	uint32_t sendedValue = auxDisplay; // conversion a una sola variable

	uint32_t value=0;
	while(1){

		auxValue = value; 	// inicializo copia
		for(i=0;i<DIGITOS;i++){ 	// inicializo los digitos
			auxDisplay[i] = 0;
		}
		for (i=5; i; i--){  // separo por digito
			auxDisplay[i] = auxValue%10;
			auxValue /= 10;
		}
		auxDisplay[0] = auxValue;

		/*sendedValue = S7Display_conversionvalor(value);*/

		xQueueSendToBack(cola, &sendedValue , portMAX_DELAY);
		vTaskDelay(1000/portTICK_RATE_MS); // Espero 1 segundo
		value += 100;
		if (value == 100000){
			value = 0;
		}
	}

}



static void Task_Barrido_Display7seg( void *pvParameters )
{
	portTickType xLastWakeTime;
	static uint8_t	i = 0;
	uint8_t *valorDisplay;// array de 8 bits
	uint32_t auxValue;
	xLastWakeTime = xTaskGetTickCount();
	portBASE_TYPE xStatus;
	uint8_t aux_valorDisplay;
	while(1){
		if(uxQueueMessagesWaiting(cola)!= 0){
			xStatus = xQueueReceive(cola, &auxValue, 0);
			valorDisplay = auxValue;
		}
		//apago dígitos
		Chip_GPIO_SetPinState(LPC_GPIO, BCDA, true);
		Chip_GPIO_SetPinState(LPC_GPIO, BCDB, true);
		Chip_GPIO_SetPinState(LPC_GPIO, BCDC, true);
		Chip_GPIO_SetPinState(LPC_GPIO, BCDD, true);
		Chip_GPIO_SetPinState(LPC_GPIO, DP, false);

		for(i=0; i < DIGITOS ; i++){
			if (!i){
				Chip_GPIO_SetPinState(LPC_GPIO, RST, true);
				Chip_GPIO_SetPinState(LPC_GPIO, RST, false);
			}
			else {
				Chip_GPIO_SetPinState(LPC_GPIO, CLK, false); // cambiamos el clock para que avance al proximo digito
				Chip_GPIO_SetPinState(LPC_GPIO, CLK, true);
			}
			Chip_GPIO_SetPinState(LPC_GPIO,BCDA, ( valorDisplay[i] & (uint8_t)0x01));
			Chip_GPIO_SetPinState(LPC_GPIO,BCDB, (( valorDisplay[i] >> 1 ) & (uint8_t)0x01));
			Chip_GPIO_SetPinState(LPC_GPIO,BCDC, (( valorDisplay[i] >> 2 ) & (uint8_t)0x01));
			Chip_GPIO_SetPinState(LPC_GPIO,BCDD, (( valorDisplay[i] >> 3 ) & (uint8_t)0x01));

			vTaskDelayUntil(&xLastWakeTime,1/portTICK_RATE_MS);

		}
	}
}


void Configuracion_inicio_uC(void){

	//####### GPIO #########

	Chip_GPIO_Init(LPC_GPIO);
	// Inicio de Leds
	InicioLeds();
	// Inicio de Display 7segmentos
	S7Display_Inicio();
}


/****************************************************************************************************/
/**************************************** MAIN ******************************************************/
/****************************************************************************************************/

int main(void){

	/* Levanta la frecuencia del micro */
	SystemCoreClockUpdate();

	/* Configuración inicial del micro */
	Configuracion_inicio_uC();

	cola = xQueueCreate(1,sizeof(uint32_t));

    /* Creacion de tareas*/
	xTaskCreate(Task_Barrido_Display7seg, (char *) "",
    			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
    			(xTaskHandle *) NULL);

	xTaskCreate(Task_Generacion_Valores, (char *) "",
	    		configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
	    		(xTaskHandle *) NULL);

     /*Inicia el scheduler */
	vTaskStartScheduler();

	/* Nunca debería arribar aquí */

    return 0;
}

