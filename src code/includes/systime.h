
/**
 * @file    systime.h
 * @brief   Middleware for the systick driver.
 *          Contains all the definitions and prototypes for the systime module.
 * @author  Manuel Burnay
 * @date    2019.09.24 (Created)
 * @date    2019.10.03 (Last Modified)
 */

#ifndef SYSTIME_H
	#define SYSTIME_H

	#include <stdint.h>
	#include <stdbool.h>
	#include <string.h>
	#include "systick.h"

    /**
     * @brief   Date structure.
     *          Contains all the member of a date in order.
     * @details used by systime when interfacing with the application &
     *          to track the system date.
     * @details The systime will not recognize January as month 0.
     *          January is month 1.
     */
	typedef struct date_ {
		uint16_t 	year;
		uint8_t 	month;
		uint8_t		day;
	} date_t;

	/**
	 * @brief   Clock structure.
	 * @details Used by systime when interfacing with the application.
	 */
	typedef struct clock_ {
		uint8_t hour;
		uint8_t min;
		uint8_t	sec;
		uint8_t	t_sec;
	} clock_t;

	/**
	 * @brief   alarm data structure
	 * @details Currently not in use.
	 */
	typedef struct alarm_ {
	    bool en;
	    void (*alarm_cb)(void);
	} alarm_t;

	/**
	 * @brief   System time strucure.
	 * @details Contains all the elements the system time middleware controls and maintains/handles.
	 */
	typedef struct systime_ {
		date_t date;
		systick_descriptor_t systick;
	} systime_t;

	#define MSEC_IN_TSEC 100
	#define TSEC_IN_SEC	10
	#define SEC_IN_MIN	60
	#define MIN_IN_HOUR	60
	#define HOUR_IN_DAY	24

	#define MSEC_IN_SEC 	(MSEC_IN_TSEC*TSEC_IN_SEC)
	#define MSEC_IN_MIN		(MSEC_IN_SEC*SEC_IN_MIN)
	#define MSEC_IN_HOUR	(MSEC_IN_MIN*MIN_IN_HOUR)
	#define MSEC_IN_DAY		(MSEC_IN_HOUR*HOUR_IN_DAY)

	#define TSEC_IN_MIN 	(TSEC_IN_SEC*SEC_IN_MIN)
	#define TSEC_IN_HOUR 	(TSEC_IN_MIN*MIN_IN_HOUR)
	#define TSEC_IN_DAY		(TSEC_IN_HOUR*HOUR_IN_DAY)

	#define SEC_IN_DAY	(SEC_IN_MIN*MIN_IN_HOUR*HOUR_IN_DAY)

	#define MONTH_IN_YEAR 12

	/**
	 * @brief   Macro to determine if a year is a leap year.
	 * @details It takes into consideration centuries that aren't divisible by 400.
	 *          (these are not leap years despite being divisible by 4)
	 */
    #define IS_LEAP_YR(yr) ((yr % 4 == 0) && ((yr % 400 == 0) || (yr % 100 != 0)))

	void systime_init();

	bool systime_SetTime(clock_t* new_clock);
	void systime_GetTime(clock_t* ret_clock);
	bool systime_SetDate(date_t* new_date);
	void systime_GetDate(date_t* ret_date);

	bool systime_SetAlarm(clock_t* alarm_clock, void (*alarm_cb)(void));
	void systime_ClearAlarm();

#endif		// SYSTIME_H
