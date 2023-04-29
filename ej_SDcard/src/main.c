

#include "chip.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include "sd_card.h"

#include <cr_section_macros.h>




xSemaphoreHandle sWrite;
xSemaphoreHandle sRead;



/* Control de tiempo para la SD */
static void vTimeSDTask(void *pvParameters)
{
	while (1)
	{
		/* Esta función es necesaria para la comunicación con la SD */
		/* Se llama a la función periódicamente cada 50mseg         */
		memory_clock();
		vTaskDelay(50/portTICK_RATE_MS);
	}
}


static void vWriteSD(void *pvParameters)
{
	MY_DATA DataWrite;

	DataWrite.data1 = 1;
	DataWrite.data2 = 22;
	DataWrite.data3 = 333;

	disk_init();
	MEMORY_RESULT estadoLectura;
	while(1)
	{
		xSemaphoreTake(sWrite,portMAX_DELAY);
		estadoLectura = writeFile(&DataWrite);
		if(estadoLectura ==OK )
		{

			xSemaphoreGive(sRead);
		}
		else{
			xSemaphoreGive(sWrite);

		}

	}
}


static void vReadSD(void *pvParameters)
{
	MY_DATA DataRead;

	DataRead.data1 = 0;
	DataRead.data2 = 0;
	DataRead.data3 = 0;


	while(1)
	{
		xSemaphoreTake(sRead,portMAX_DELAY);

		readFile(&DataRead);

	}
}




/****************************************************************************************************/
/**************************************** MAIN ******************************************************/
/****************************************************************************************************/
int main(void)
{
	SystemCoreClockUpdate();

	vSemaphoreCreateBinary(sWrite);
	vSemaphoreCreateBinary(sRead);

	xSemaphoreTake(sRead,0);


    xTaskCreate(vTimeSDTask, (char *) "vTaskTimeSD",
            	configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
            	(xTaskHandle *) NULL);
    xTaskCreate(vWriteSD, (char *) "vWriteSD",
                configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
                (xTaskHandle *) NULL);
    xTaskCreate(vReadSD, (char *) "vReadSD",
                configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
                (xTaskHandle *) NULL);



    /* Start the scheduler */
	vTaskStartScheduler();

	/* Nunca debería arribar aquí */

    return 0;
}

