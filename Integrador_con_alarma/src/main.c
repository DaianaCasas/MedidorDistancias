#include <Sensor.h>
#include <display7seg.h>
#include <alarma.h>

#include "chip.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define LEDSTICK 0,22

xQueueHandle colaMediciones;
xQueueHandle colaDisplay;

/* Interrupción de TIMER2
 *
 * Permite el conteo de tiempo del pulso del sensor HCSR04 ultrasonico.
 * Detecta flanco ascendente del pin, habilita la cuenta hasta el flanco descendiente del pin.
 *
 * */
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


void Alarmas_Inicio(void){
	//####### LEDS #########
	Chip_IOCON_PinMux(LPC_IOCON, LEDSTICK, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, LEDSTICK);
	Chip_GPIO_SetPinState(LPC_GPIO,LEDSTICK, false);
	//####### LED alarma minima #########
	Chip_IOCON_PinMux(LPC_IOCON, Amin, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, Amin);
	Chip_GPIO_SetPinState(LPC_GPIO,Amin, false);
	//####### LED alarma maxima #########
	Chip_IOCON_PinMux(LPC_IOCON, Amax, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, Amax);
	Chip_GPIO_SetPinState(LPC_GPIO,Amax, false);
}


void Alarmas_Control(alarma tipo){
	switch(tipo){
		case A_min:
			if ( Chip_GPIO_GetPinState(LPC_GPIO, Amin) == false ){
				Chip_GPIO_SetPinState(LPC_GPIO,Amin, true);
				Chip_GPIO_SetPinState(LPC_GPIO,Amax, false);
			}
		break;
		case A_max:
			if ( Chip_GPIO_GetPinState(LPC_GPIO, Amax) == false ){
				Chip_GPIO_SetPinState(LPC_GPIO,Amin, false);
				Chip_GPIO_SetPinState(LPC_GPIO,Amax, true);
			}
		break;
		case A_normal:
			if ( ( Chip_GPIO_GetPinState(LPC_GPIO, Amin) == true ) | ( Chip_GPIO_GetPinState(LPC_GPIO, Amax) == true ) ){
				Chip_GPIO_SetPinState(LPC_GPIO,Amin, false);
				Chip_GPIO_SetPinState(LPC_GPIO,Amax, false);
			}
		break;
	}
	return;
}


alarma Alarmas_Comparar(uint32_t valor){
	alarma estado;

	if ((Amin_mm < valor ) & (Amax_mm > valor)){
		estado = A_normal;
	}
	else if ((Amin_mm > valor )){
		estado = A_min;
	}
	else{
		estado = A_max;
	}
	return estado;

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
	 *
	 * Distancia minima: 20mm
	 * Distancia maxima: 4000mm
	 * */
	float VELOCIDAD_mm_useg=0.3432; // 343,2 m/1 s

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

	time_usecond = (ticks/frec_usecond) + 5; // ticks * Tusecond, sumo 5useg de error.

	aux = ( time_usecond * VELOCIDAD_mm_useg )/2;

	return aux; // milimetros

}


static void Task_MedirDistancia( void *pvParameters )
{
	portTickType xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();
	while(1){
		sMedicion_CrearPulso();
		Chip_GPIO_SetPinToggle(LPC_GPIO,LEDSTICK);
		vTaskDelayUntil(&xLastWakeTime,500/portTICK_RATE_MS); // Cada 100 mSEG

		}
}


static void Task_ProcesarDistancia( void *pvParameters )
{
	uint32_t aux_valorDistancia=0;
	uint32_t valorDistancia=0;
	portBASE_TYPE estadoLectura;
	uint8_t i; // indice
	uint8_t auxDisplay[DIGITOS]={0,0,0,0,0,0}; // inicializo los digitos
	uint32_t auxValue = 0; // copia del valor a mostrar en display
	uint32_t sendedValue = auxDisplay; // conversion a una sola variable
	alarma valor;
	while(1){
		estadoLectura = xQueueReceive(colaMediciones, &aux_valorDistancia,portMAX_DELAY);
		if (estadoLectura ==  pdPASS){
			valorDistancia = sMedicion_medir(aux_valorDistancia);
			auxValue = valorDistancia; 	// inicializo copia
			for(i=0;i<DIGITOS;i++){ 	// inicializo los digitos
				auxDisplay[i] = 0;
			}
			for (i=5; i; i--){  // separo por digito
				auxDisplay[i] = auxValue%10;
				auxValue /= 10;
			}
			auxDisplay[0] = auxValue;
			xQueueSendToBack(colaDisplay,&sendedValue,0);

			valor = Alarmas_Comparar(valorDistancia);
			Alarmas_Control(valor);
		}
	}
}


static void Task_Barrido_Display7seg( void *pvParameters )
{
	portTickType xLastWakeTime;
	static uint8_t	i = 0;
	uint8_t *valorDisplay;// array de 8 bits
	uint32_t auxValue=0;
	xLastWakeTime = xTaskGetTickCount();
	portBASE_TYPE xStatus;
	uint8_t aux_valorDisplay;
	while(1){
		if(uxQueueMessagesWaiting(colaDisplay)!= 0){
			xStatus = xQueueReceive(colaDisplay, &auxValue, 0);
			valorDisplay = auxValue;
		}
		//apago dígitos
		Chip_GPIO_SetPinState(LPC_GPIO, BCDA, true);
		Chip_GPIO_SetPinState(LPC_GPIO, BCDB, true);
		Chip_GPIO_SetPinState(LPC_GPIO, BCDC, true);
		Chip_GPIO_SetPinState(LPC_GPIO, BCDD, true);
		Chip_GPIO_SetPinState(LPC_GPIO, DP, false);

		for(i=0; i < DIGITOS ; i++){
			aux_valorDisplay = valorDisplay[i];
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
	// Inicio de Alarmas
	Alarmas_Inicio();

	// Inicio del Display
	S7Display_Inicio();

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

	colaDisplay = xQueueCreate(5,sizeof(uint32_t));

	/* Configuración inicial del micro */
	Configuracion_inicio_uC();


    /* Creacion de tareas*/
	xTaskCreate(Task_MedirDistancia, (char *) "",
    			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
    			(xTaskHandle *) NULL);
	xTaskCreate(Task_ProcesarDistancia, (char *) "",
    			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
    			(xTaskHandle *) NULL);
	xTaskCreate(Task_Barrido_Display7seg, (char *) "",
    			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
    			(xTaskHandle *) NULL);

     /*Inicia el scheduler */
	vTaskStartScheduler();

	/* Nunca debería arribar aquí */

    return 0;
}

