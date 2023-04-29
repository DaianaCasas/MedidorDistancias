

#include <sd_card.h>


FATFS FatFs;
FIL File;

volatile UINT Timer;



/*---------------------------------------------------------*/
/* User Provided Timer Function for FatFs module           */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called from     */
/* FatFs module. Any valid time must be returned even if   */
/* the system does not support a real time clock.          */
/* This is not required in read-only configuration.        */
DWORD get_fattime(void)
{
	RTC rtc;

	rtc.year = 2018;
	rtc.month = 12;
	rtc.mday = 1;
	rtc.hour = 12;
	rtc.min = 0;
	rtc.sec = 0;


	return	  ((DWORD)(rtc.year - 1980) << 25)
			| ((DWORD)rtc.month << 21)
			| ((DWORD)rtc.mday << 16)
			| ((DWORD)rtc.hour << 11)
			| ((DWORD)rtc.min << 5)
			| ((DWORD)rtc.sec >> 1);
}


/* Función de inicialización de la memoria	*/
void disk_init(void)
{
	uint8_t sd_status;
	sd_status = disk_initialize(0);
	f_mount(&FatFs, "", 0);
}


/* Función de temporización para la memoria	*/
void memory_clock(void)
{
	Timer++;
	disk_timerproc();
}


/* Escribe un archivo  */
MEMORY_RESULT writeFile(MY_DATA* profile)
{
	MEMORY_RESULT result;
	FRESULT estado;

	estado = f_open(&File,"Mediciones.txt",FA_OPEN_ALWAYS|FA_WRITE|FA_READ);
	if(estado==FR_OK)
	{
		uint32_t contador;

		f_write(&File, profile, sizeof(MY_DATA), &contador);

		result = OK;

		f_close(&File);
	}

	else
		result = FAILED;

	return result;
}


/* Abre el archivo del perfil en modo lectura	*/
MEMORY_RESULT readFile(MY_DATA* profile)
{
	MEMORY_RESULT result;

	if(f_open(&File,"PROFILE.dat",FA_READ|FA_OPEN_EXISTING)==FR_OK)
	{
		uint32_t contador;

		f_read(&File, profile, sizeof(MY_DATA), &contador);

		result = OK;

		f_close(&File);
	}

	else
		result = FAILED;

	return result;
}
