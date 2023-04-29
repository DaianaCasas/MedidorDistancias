

#ifndef MEMORY_DRIVERS_H_
#define MEMORY_DRIVERS_H_

#include "chip.h"
#include <string.h>
#include <cr_section_macros.h>

#include "diskio.h"
#include "ff.h"


typedef enum{
	OK,
	FAILED
}MEMORY_RESULT;


typedef struct{
	uint32_t data1;
	uint32_t data2;
	uint32_t data3;
}MY_DATA;


typedef struct {
	WORD	year;	/* 1..4095 */
	BYTE	month;	/* 1..12 */
	BYTE	mday;	/* 1.. 31 */
	BYTE	wday;	/* 1..7 */
	BYTE	hour;	/* 0..23 */
	BYTE	min;	/* 0..59 */
	BYTE	sec;	/* 0..59 */
} RTC;

/*---------------------------------------------------------*/
/* User Provided Timer Function for FatFs module           */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called from     */
/* FatFs module. Any valid time must be returned even if   */
/* the system does not support a real time clock.          */
/* This is not required in read-only configuration.        */
DWORD get_fattime(void);


void disk_init(void);

void memory_clock(void);

MEMORY_RESULT writeFile(MY_DATA* profile);

MEMORY_RESULT readFile(MY_DATA* profile);


#endif /* MEMORY_DRIVERS_H_ */
