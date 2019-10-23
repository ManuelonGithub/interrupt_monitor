
/**
 * @file    query_handler.c
 * @brief   Defines all the functionality regarding query handling of the monitor.
 * @author  Manuel Burnay
 * @date    2019.09.26 (Created)
 * @date    2019.10.10 (Last Modified)
 */


#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "query_handler.h"
#include "uart.h"

/** All valid month entries for setting the date*/
static const char* const MONTHS[] = {
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

/* all supported query keywords */

const char TIME_QUERY[] = {"TIME"};     /// Time query keyword
const char DATE_QUERY[] = {"DATE"};     /// Date query keyword
const char ALARM_QUERY[] = {"ALARM"};   /// Alarm query keyword

char CURSOR_LEFT[] = {"\x1b[D"};
char CURSOR_RIGHT[] = {"\x1b[C"};
char CURSOR_UP[] = {"\x1b[A"};
char CURSOR_DOWN[] = {"\x1b[B"};
char CLEAR_SCREEN[] = {"\x1b[2J"};
char CURSOR_HOME[] = {"\x1b[H"};
char ALARM_BELL[] = {"\x07"};

// This function is only needed in this module so no need to make it available elsewhere.
uint8_t FindMonthValue(char* month_str);

static query_buffer_t query; /** Query character buffer */

/**
 * @brief   Initializes the query handler's buffer and the terminal entry point.
 * @details Make sure the UART driver has been initialized prior to calling this function,
 *          otherwise you will cause a memory access fault.
 */
void QueryHandler_Init()
{
    circular_buffer_init(&query.buffer);

    UART0_puts(CLEAR_SCREEN);
    UART0_puts(CURSOR_HOME);
    UART0_puts("> ");
}

/**
 * @brief   Query Handler update function
 * @param   [in, out] rx_buf: Receive buffer that contains the data being inputed by the user.
 * @details This function normally just transfers bytes from the RX buffer to the query buffer,
 *          but checks for certain key characters that effect the behavior of the query buffer,
 *          namely the delete/backspace char, the ENTER char, and the start of an ANSI escape code.
 */
void QueryHandler_Update(circular_buffer_t* rx_buf)
{
    char data = dequeuec(rx_buf);

    switch (data) {
        case '\b':
        case 0x7F: {
            if (query.buffer.wr_ptr > 0) {
                query.buffer.wr_ptr--;
                query.entry_ptr--;
            }
            else {
                UART0_puts(" ");
            }
        } break;

        case '\r':
        case '\n': {
//            enqueuec_s(&query.buffer, toupper(data), false);
            if (!QueryCheck(query.buffer.data, query.buffer.wr_ptr)) {
                UART0_puts("? \n");
            }
//            memset(query.buffer.data, 0, query.entry_ptr);
            query.entry_ptr = 0;
            query.buffer.wr_ptr = 0;

            UART0_puts("> ");
        } break;

        case 0x1B: {
            CursorCodeCheck(rx_buf);
        } break;

        default: {
            if (!enqueuec_s(&query.buffer, toupper(data), false)) UART0_puts("\b");
            if (query.entry_ptr < query.buffer.wr_ptr) query.entry_ptr = query.buffer.wr_ptr;
        } break;
    }
}

/**
 * @brief   Checks the data currently in the Query buffer for a valid query
 *          and services valid queries.
 * @return  [bool] True if there is a valid query in the buffer, False in not.
 */
bool QueryCheck()
{
    bool valid_command = false;

    // Make these equivalences just to improve readability
    // I made my animal sacrifices to ensure the gcc compiler gods will optimize this
    char* query_str = query.buffer.data;
    uint32_t length = query.entry_ptr;

    char* keyword;
    char* set_data;

    int i = 0;
    // Find the begin of they query entry
    while (i <= length && query_str[i] == ' ') i++;
    keyword = query_str + i;

    // Find the end of the query keyword
    while (i < length && query_str[i] != ' ') i++;
    query_str[i++] = '\0';    // null-terminate the keyword to make decoding easier.

    // Find the begin of the query set data (if it exists)
    while (i < length && query_str[i] == ' ') i++;
    set_data = (i < length) ? (query_str + i) : NULL;

    if (strcmp(keyword, TIME_QUERY) == 0) {
        if (set_data != NULL) {
            valid_command = SetTime(set_data);
        }
        else {
            DisplayTime();
            valid_command = true;
        }
    }
    else if (strcmp(keyword, DATE_QUERY) == 0) {
        if (set_data != NULL) {
            valid_command = SetDate(set_data);
        }
        else {
            DisplayDate();
            valid_command = true;
        }
    }
    else if(strcmp(keyword, ALARM_QUERY) == 0) {
        if (set_data != NULL) {
            valid_command = SetAlarm(set_data);
        }
        else {
            systime_ClearAlarm();
            UART0_puts("Alarm has been cleared\n");
            valid_command = true;
        }
    }

    return valid_command;
}

/**
 * @brief   Gets a new time from a string for Systime to track/maintain.
 * @param   [in] new_time_str: char string with the new time to be set.
 * @bool    True if a new time was set, false if not.
 * @details Setting the time can fail in two ways: An error in the time string format,
 *          or an error in the time values.
 */
bool SetTime(char* new_time_str)
{
    clock_t clock_temp;
    bool retval = false;
    char time_str[128];

    int scan_res = sscanf(new_time_str, "%2hhu:%2hhu:%2hhu.%1hhu",
                          &clock_temp.hour, &clock_temp.min,
                          &clock_temp.sec, &clock_temp.t_sec);

    if (scan_res == VALID_TIME_SCAN && systime_SetTime(&clock_temp)) {
        retval = true;

        sprintf(time_str, "%02u:%02u:%02u.%u",
                clock_temp.hour, clock_temp.min,
                clock_temp.sec, clock_temp.t_sec);

        UART0_puts(time_str);
        UART0_puts(" \n");
    }

    return retval;
}

/**
 * @brief    Displays the current time in Systime to UART.
 */
void DisplayTime(void)
{
    clock_t clock_temp;
    systime_GetTime(&clock_temp);

    char time_str[128];
    sprintf(time_str, "%02u:%02u:%02u.%u",
            clock_temp.hour, clock_temp.min,
            clock_temp.sec, clock_temp.t_sec);

    UART0_puts(time_str);
    UART0_puts(" \n");
}

/**
 * @brief   Sets a new date for Systime to track/maintain based on a char string's data.
 * @param   [in] new_date_str: char string with new date to be set.
 * @return  [bool] True if a new date was set, false if not.
 * @details Setting the date can fail in two ways: An error in the date string format,
 *          or an error in the date values.
 */
bool SetDate(char* new_date_str)
{
    bool retval = false;
    date_t date_temp;
    char month_str[10];
    char date_str[128];

    int scan_res = sscanf(new_date_str, "%hhu-%3s-%hu", &date_temp.day, month_str, &date_temp.year);
    date_temp.month = FindMonthValue(month_str)+1;

    if (scan_res == VALID_DATE_SCAN && systime_SetDate(&date_temp)) {
        retval = true;
        sprintf(date_str, "%02u-%3s-%04u",
                date_temp.day, MONTHS[(date_temp.month-1)], date_temp.year);

        UART0_puts(date_str);
        UART0_puts(" \n");
    }

    return retval;
}

/**
 * @brief   Find month number (0..11) of a written month.
 * @param   [in] month_str: char string containing the written month.
 * @return  [uint8_t] Month number between 0..12 (see details).
 * @details 0  -> January,
 *          ...
 *          11 -> December,
 *          12 -> Bad string.
 */
uint8_t FindMonthValue(char* month_str)
{
    int i = 0;

    while (i < MONTH_IN_YEAR && memcmp(month_str, MONTHS[i], 3)) {
        i++;
    }

    return i;
}

/**
 * @brief    Displays the current date in Systime to UART.
 */
void DisplayDate()
{
    date_t date_temp;
    systime_GetDate(&date_temp);

    char date_str[128];
    sprintf(date_str, "%02u-%3s-%04u",
            date_temp.day, MONTHS[(date_temp.month-1)], date_temp.year);

    UART0_puts(date_str);
    UART0_puts(" \n");
}

/**
 * @brief   Sets an alarm of Systime to configure based on a char string's data.
 * @param   [in] alarm_str: char string containing the alarm time.
 * @return  [bool] True if alarm was successfully set, false if not.
 * @details Setting the alarm can fail in two ways: An error in the alarm string format,
 *          or an error in the time values.
 */
bool SetAlarm(char* alarm_str)
{
    clock_t clock_temp, current_time;
    bool retval = false;
    char time_str[128];

    int scan_res = sscanf(alarm_str, "%2hhu:%2hhu:%2hhu.%1hhu",
                       &clock_temp.hour, &clock_temp.min,
                       &clock_temp.sec, &clock_temp.t_sec);

    if (scan_res == VALID_ALARM_SCAN) {
        retval = systime_SetAlarm(&clock_temp, Alarm_callback);
        systime_GetTime(&current_time);

        clock_temp.t_sec += current_time.t_sec;
        clock_temp.sec += current_time.sec;
        clock_temp.min += current_time.min;
        clock_temp.hour += current_time.hour;

        if (clock_temp.t_sec > TSEC_IN_SEC) {
            clock_temp.t_sec -= TSEC_IN_SEC;
            clock_temp.sec++;
        }
        if (clock_temp.sec > SEC_IN_MIN) {
            clock_temp.sec -= SEC_IN_MIN;
            clock_temp.min++;
        }
        if (clock_temp.min > MIN_IN_HOUR) {
            clock_temp.min -= MIN_IN_HOUR;
            clock_temp.hour++;
        }
        clock_temp.hour = clock_temp.hour % HOUR_IN_DAY;

        sprintf(time_str, "%02u:%02u:%02u.%u",
                clock_temp.hour, clock_temp.min,
                clock_temp.sec, clock_temp.t_sec);

        UART0_puts("Alarm at ");
        UART0_puts(time_str);
        UART0_puts(" \n");
    }

    return retval;
}

/**
 * @brief   Alarm Callback function.
 * @details Function is called when a set alarm's time has elapsed.
 */
void Alarm_callback(void)
{
    UART0_puts(ALARM_BELL);
    UART0_puts("\n* ALARM * ");

    clock_t clock_temp;
    systime_GetTime(&clock_temp);

    char time_str[128];
    sprintf(time_str, "%02u:%02u:%02u.%u",
            clock_temp.hour, clock_temp.min,
            clock_temp.sec, clock_temp.t_sec);

    UART0_puts(time_str);
    UART0_puts(" * \n");
    UART0_puts("> ");
}

/**
 * @brief   Checks for a cursor escape code in the receive buffer
 *          and acts according to the cursor code found.
 * @param   [in, out]   rx_buf: Pointer to receive buffer with escape code
 *                              at the read pointer.
 * @details This function only checks for cursor codes that come from the arrow keys.
 *          Any other escape codes (including cursor code with multiple "moves") are not handled.
 * @details This function assumes that the escape char (x1b) has been previously detected and removed from the rx buffer.
 * @todo    Change this function so it handles more escape codes (or just handles them better)
 *          HINT: Escape code only contain one alphabetic character, and it's always at the end of the code.
 * @0todo:  create a query save buffer with the last couple of query entries and
            have the 'UP cursor' escape code select one of the save query entries.
 */
void CursorCodeCheck(circular_buffer_t* rx_buf)
{
    escape_code_t esc_seq;
    while (buffer_size(rx_buf) < sizeof(esc_seq)) ; // wait until the smallest escape command is in the buffer;

    dequeue(rx_buf, (uint8_t*)&esc_seq, sizeof(esc_seq));

    switch (esc_seq.code) {
        case 'A': {
            UART0_puts(CURSOR_DOWN);
            /*
             * todo:
             * create a query save buffer with the last couple of query entries and
             * have this escape code select one of the save query entries.
             */
        } break;
        case 'B': {
            UART0_puts(CURSOR_UP);
            while (query.buffer.wr_ptr < query.entry_ptr) {
                UART0_puts(CURSOR_RIGHT);
                query.buffer.wr_ptr++;
            }
        } break;
        case 'C': {
            if (query.buffer.wr_ptr < query.entry_ptr) {
                query.buffer.wr_ptr++;
            }
            else {
                UART0_puts(CURSOR_LEFT);
            }
        } break;
        case 'D': {
            if (query.buffer.wr_ptr > 0) {
                query.buffer.wr_ptr--;
            }
            else {
                UART0_puts(CURSOR_RIGHT);
            }
        } break;
    }
}
