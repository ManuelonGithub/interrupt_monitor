
/**
 * @file    systime.c
 * @brief   Middleware for the systick driver.
 *          Contains all the functionality to maintain and keep track of time, date, and user-set alarms.
 * @author  Manuel Burnay
 * @date    2019.09.24 (Created)
 * @date    2019.10.03 (Last Modified)
 *
 * @details Configures systick to activate every tenth of a second,
 *          and uses systick to maintain and upkeep an
 *          accurate time, date and a user-set alarm.
 */

#include "systime.h"

static const uint8_t MONTH_DAYS[2][12] =  {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};  /// 2-D array that contains the valid day count for every month, for both leap years and non-leap years.

// Functions internal to the systime module
void systime_IncDate_callback(void);
inline uint8_t DaysInMonth(uint8_t month, uint16_t year);
inline uint32_t systime_ConvertClock(clock_t* clock);
inline clock_t systime_ConvertTickCounter(uint32_t t_count);

static systime_t time;  /// system time data structure.

/**
 * @brief   Initializes the systime middleware.
 * @details Sets the system time and date to initial/default values,
 *          and configures/initializes the systick driver.
 */
void systime_init()
{
    time.date.year = 0;
	time.date.month = 1;   // initialize month with a valid value
	time.date.day = 1;     // initialize day with a valid value

	time.systick.tick_rate = 10; // Systick is triggered 10 times per second

	time.systick.counter.value = 0;
	time.systick.counter.cmp_en = true;
	time.systick.counter.cmp = TSEC_IN_DAY;
	time.systick.counter.counter_cb = systime_IncDate_callback;

	time.systick.countdown.en = false;
	time.systick.countdown.value = 0;
	time.systick.countdown.countdown_cb = NULL;

	SysTick_Init(&time.systick);
}

/**
 * @brief   Sets the system time to a new time.
 * @param   [in] new_clock: new time for the system to be set to.
 * @return  [bool] True if the system time was successfully changed,
 *          False if not (Error in the new time's values).
 * @details This function safely sets the system time,
 *          meaning that it'll check that the new_clock param is valid
 *          before setting it as the new time.
 */
bool systime_SetTime(clock_t* new_clock)
{
    bool retval = false;
    if (new_clock->t_sec < TSEC_IN_SEC  &&
        new_clock->sec < SEC_IN_MIN     &&
        new_clock->min < MIN_IN_HOUR    &&
        new_clock->hour < HOUR_IN_DAY) {
            // todo: make this safer (via systick) to remove edge case corruption
            time.systick.counter.value = systime_ConvertClock(new_clock);
            retval = true;
        }

    return retval;
}

/**
 * @brief   Gets the current system time.
 * @param   [out] ret_clock: pointer to clock_t structure
 *          where the current system time will be copied to.
 */
void systime_GetTime(clock_t* ret_clock)
{
	*ret_clock = systime_ConvertTickCounter(time.systick.counter.value);
}

/**
 * @brief   Sets the system date to a new date.
 * @param   [in] new_date: new date for the system to be set to.
 * @return  [bool] True if the system date was successfully change, false if not.
 * @details This function safely sets the system date to a new date,
 *          meaning that it'll check the values in new_date to make sure they represent
 *          a valid date.
 */
bool systime_SetDate(date_t* new_date)
{
	bool retval = false;

	if (new_date->year < 9999   &&
        new_date->month > 0     && new_date->month <= MONTH_IN_YEAR &&
        new_date->day > 0       && new_date->day <= DaysInMonth(new_date->month-1, new_date->year)) {
	    // todo: Although the risk is small, look into doing this safely (via systick) to prevent edge case corruption
        time.date = *new_date;

        retval = true;
    }

    return retval;
}

/**
 * @brief   Gets the current system date.
 * @param   [out] ret_date: pointer to date_t structure
 *          where the system date will be copied to
 */
void systime_GetDate(date_t* ret_date)
{
	*ret_date = time.date;
}

/**
 * @brief   Sets an alarm for the system to track.
 * @param   [in] alarm_clock: clock for the alarm to be set to.
 * @param   [in] alarm_cb: callback function to be called for when the alarm's time has elapsed.
 * @return  [bool] Always true. Returns boolean for application layer compatibility.
 */
bool systime_SetAlarm(clock_t* alarm_clock, void (*alarm_cb)(void))
{
    time.systick.countdown.countdown_cb = alarm_cb;
    time.systick.countdown.value = systime_ConvertClock(alarm_clock);
    time.systick.countdown.en = true;

    return true;
}

/**
 * @brief   Clears the alarm being tracked by the system.
 */
void systime_ClearAlarm()
{
    time.systick.countdown.en = false;
}

/**
 * @brief   System time increment date callback function.
 * @details This function is sent to the systick driver to be called whenever the tick counter
 *          has reached the amount of ticks that would occur in a 24h day.
 * @details It increments the date safely, and it cascades date value overflows.
 * @todo    Wrap year back to 0 when it has elapsed 9999.
 */
void systime_IncDate_callback(void)
{
	time.date.day++;

	if (time.date.day > DaysInMonth(time.date.month-1, time.date.year)) {
	    time.date.day = 1;
	    time.date.month++;

		if (time.date.month > MONTH_IN_YEAR) {
		    time.date.month = 1;
		    time.date.year++;
		}
	}
}

/**
 * @brief   Converts a clock structure to tenth of seconds count.
 * @param   [in] clock: pointer to clock structure to be converted.
 * @return  [uint32_t] Amount of tenth of seconds equivalent to the clock.
 */
inline uint32_t systime_ConvertClock(clock_t* clock)
{
	return (clock->t_sec			+ 
			clock->sec   * TSEC_IN_SEC  +
			clock->min   * TSEC_IN_MIN  +
			clock->hour  * TSEC_IN_HOUR);
}

/**
 * @brief   Converts tenth of seconds count to a clock structure.
 * @param   [in] t_count: tenth of seconds count to be converted.
 * @return  [clock_t] Clock structure equivalent to the count.
 * @todo    This function should get a pointer to a clock structure to put the values in,
 *          instead of returning a clock structure (<- less efficient).
 */
inline clock_t systime_ConvertTickCounter(uint32_t t_count)
{
	clock_t retval;

	retval.hour = t_count/TSEC_IN_HOUR;
	t_count -= (TSEC_IN_HOUR * retval.hour);

	retval.min = t_count/TSEC_IN_MIN;
	t_count -= (TSEC_IN_MIN * retval.min);

	retval.sec = t_count/TSEC_IN_SEC;
	t_count -= (TSEC_IN_SEC * retval.sec);

	retval.t_sec = t_count;

	return retval;
}

/**
 * @brief   Finds the number of days in a month, while considering leap years.
 * @param   [in] month: month to find the amount of days in it.
 * @param   [in] year: Used to determine if it's a leap year
 *          (where February contains 29 days and not 28).
 */
inline uint8_t DaysInMonth(uint8_t month, uint16_t year)
{
    return MONTH_DAYS[IS_LEAP_YR(year)][month];
}
