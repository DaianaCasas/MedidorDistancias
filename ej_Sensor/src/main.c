#include <Sensor.h>
#include "chip.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define LEDSTICK 0,22

xQueueHandle colaMediciones;


/* Interrupción de TIMER2 */
void TIMER2_IRQHandler(void){

	uint32_t contadorPulso=0;
	static portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;


	// Interrupcion por flanco ascendente
	if (Chip_TIMER_CapturePending(LPC_TIMER2, CAP2CH0) & Chip_GPIO_GetPinState(LPC_GPIO,sEcho)){
		Chip_TIMER_Reset(LPC_TIMER2); // Reseteo a 0 Timer y Prescale y los Sincroninzo.
		Chip_TIMER_Enable(LPC_TIMER2); // Habilito el timer para que comience a CONTAR
	}
	// Interrupcion por flanco descendente
	else{
		Chip_TIMER_Reset(LPC_TIMER2); // Reseteo a 0 Timer y Prescale y los Sincroninzo.
		Chip_TIMER_Disable(LPC_TIMER2); // Deshabilito el timer para que se pare

		/* Lee el valor del ADC y lo guarda en la variable dataADC */
		contadorPulso = Chip_TIMER_ReadCapture(LPC_TIMER2, CAP2CH0);
		NVIC_DisableIRQ(TIMER2_IRQn);//Habilita la interrupción de TIMER2

		/* Pone el dato en una cola para su procesamiento */
		xQueueSendToBackFromISR(colaMediciones,&contadorPulso,&xHigherPriorityTaskWoken);

		/* Fuerza la ejecución del scheduler para así retornar a una tarea más prioritaria de ser necesario */
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
	Chip_TIMER_ClearCapture(LPC_TIMER2,CAP2CH0);

}


void InicioLeds(void){
	//####### LEDS #########
	Chip_IOCON_PinMux(LPC_IOCON, LEDSTICK, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, LEDSTICK);
	Chip_GPIO_SetPinState(LPC_GPIO,LEDSTICK, false);
}


void sMedicion_CrearPulso(void){
	// Crear el pulso
	Chip_TIMER_Reset(LPC_TIMER3); // Reseteo a 0 Timer y Prescale y los Sincroninzo.
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER3,sSTATUS_HIGH,TIMER_EXTMATCH_TOGGLE, TIM3CH0);// Seteo el pin en HIGH para cuando termine de contar lo cambie a LOW (toggle)
	Chip_TIMER_MatchEnableInt(LPC_TIMER3, TIM3CH0);

	NVIC_EnableIRQ(TIMER2_IRQn);//Habilita la interrupción de TIMER2
	Chip_TIMER_Enable(LPC_TIMER3); // Habilito el timer para que comience a CONTAR
	Chip_TIMER_Enable(LPC_TIMER2); // Habilito el timer para que comience a ver el CAPTURE
	Chip_TIMER_CaptureEnableInt(LPC_TIMER2,CAP2CH0);


}


uint32_t sMedicion_medir(uint32_t ticks){
	/*
	 * Resolucion de 3mm
	 * */
	float VELOCIDAD_mm_useg=0.1716; // 343,2 m/1 s --> 29.412µs (microseconds) per centimeter.

	uint32_t aux;
	float frec_second;
	float frec_usecond;
	float time_usecond;

	if (Chip_TIMER_ReadPrescale(LPC_TIMER3)){
		frec_second = Chip_Clock_GetPeripheralClockRate(SYSCTL_PCLK_TIMER3)/Chip_TIMER_ReadPrescale(LPC_TIMER3);
	}
	else{
		frec_second = Chip_Clock_GetPeripheralClockRate(SYSCTL_PCLK_TIMER3);
	}
	frec_usecond =  (frec_second) / 1E6;

	time_usecond = ticks/frec_usecond;

	aux = time_usecond * VELOCIDAD_mm_useg;

	return aux; // milimetros

}


void sMedicion_Inicio(void){
	//####### Pin de Trigger #########
	Chip_IOCON_PinMux(LPC_IOCON, sTrigg, IOCON_MODE_INACT, IOCON_FUNC3);
	Chip_IOCON_EnableOD(LPC_IOCON, sTrigg);


	//####### Timer3 para el Trigger #########
	Chip_TIMER_Init(LPC_TIMER3); // Inicio el Timer 3
	Chip_TIMER_MatchEnableInt(LPC_TIMER3, TIM3CH0); // Lo configuro para que utilice el MR0.
	Chip_TIMER_SetMatch(LPC_TIMER3, 0, sTrigg_TimePulse); // Configuro el pulso
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER3, sSTATUS_LOW,TIMER_EXTMATCH_CLEAR , TIM3CH0); //Seteo el control del pin, lo dejo en LOW

	Chip_TIMER_Disable(LPC_TIMER3); // Deshabilito el timer para que aun no cuente
	Chip_TIMER_MatchDisableInt(LPC_TIMER3, TIM3CH0); // Deshabilito la interrupcion por MATCH

	//####### Pin de Echo #########
	Chip_IOCON_PinMux(LPC_IOCON, sEcho, IOCON_MODE_INACT, IOCON_FUNC3);

	//####### Timer2 para el Echo #########
	Chip_TIMER_Init(LPC_TIMER2); // Inicio el Timer 2
	Chip_TIMER_CaptureRisingEdgeEnable(LPC_TIMER2,CAP2CH0);
	Chip_TIMER_CaptureFallingEdgeEnable(LPC_TIMER2,CAP2CH0);

	NVIC_DisableIRQ(TIMER2_IRQn);//Habilita la interrupción de TIMER2
	Chip_TIMER_CaptureDisableInt(LPC_TIMER2,CAP2CH0);
	Chip_TIMER_Disable(LPC_TIMER2); // Deshabilito el timer para que aun no cuente

}


static void Task_MedirDistancia( void *pvParameters )
{
	portTickType xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();
	uint8_t aux_valorDisplay;
	while(1){
		vTaskDelayUntil(&xLastWakeTime,100/portTICK_RATE_MS); // Cada 100 mSEG
		sMedicion_CrearPulso();
		Chip_GPIO_SetPinOutHigh(LPC_GPIO,LEDSTICK);

		}
}


static void Task_ProcesarDistancia( void *pvParameters )
{
	uint32_t aux_valorDistancia=0;
	uint32_t valorDistancia=0;
	portBASE_TYPE estadoLectura;
	while(1){
		estadoLectura = xQueueReceive(colaMediciones, &aux_valorDistancia,portMAX_DELAY);
		if (estadoLectura){
			valorDistancia = sMedicion_medir(aux_valorDistancia);
			Chip_GPIO_SetPinOutLow(LPC_GPIO,LEDSTICK);
		}
	}
}


void Configuracion_inicio_uC(void){

	//####### GPIO #########

	Chip_GPIO_Init(LPC_GPIO);
	// Inicio de Leds
	InicioLeds();
	// Inicio de Sensor
	sMedicion_Inicio();
}


/****************************************************************************************************/
/**************************************** MAIN ******************************************************/
/****************************************************************************************************/

int main(void){

	/* Levanta la frecuencia del micro */
	SystemCoreClockUpdate();

	colaMediciones = xQueueCreate(1,sizeof(uint32_t));

	/* Configuración inicial del micro */
	Configuracion_inicio_uC();

    /* Creacion de tareas*/
	xTaskCreate(Task_MedirDistancia, (char *) "",
    			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
    			(xTaskHandle *) NULL);
	xTaskCreate(Task_ProcesarDistancia, (char *) "",
    			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
    			(xTaskHandle *) NULL);

     /*Inicia el scheduler */
	vTaskStartScheduler();

	/* Nunca debería arribar aquí */

    return 0;
}

