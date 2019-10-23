
/**
 * @file    query_handler.h
 * @brief   Contains all the definitions and function prototypes for the query handler.
 * @author  Manuel Burnay
 * @date    2019.09.26 (Created)
 * @date    2019.10.10 (Last Modified)
 */


#ifndef	QUERY_HANDLER_H
	#define QUERY_HANDLER_H

	#include "uart.h"
	#include "systime.h"

    /**
     * @brief   all query types supported by the handler.
     * @brief   Each query has a "set" variant
     */
	enum QUERY_TYPES{TIME, DATE, ALARM};


    #define VALID_DATE_SCAN 3
    #define VALID_TIME_SCAN 4
    #define VALID_ALARM_SCAN VALID_TIME_SCAN

	/**
	 * @brief   escape code buffer.
	 *          Used to map an escape cursor code to it's individual parameters.
	 */
	typedef struct escape_code_{
	    char sqbkrt;
	    char code;
	}escape_code_t;

	/**
	 * @brief	Query buffer structure.
	 * @details It is simply a circular buffer with an extra variable that is used 
	 			to keep track of the length of the entry as characters are inputted to the monitor.
	 			(the write pointer of the circular buffer is the "cursor", 
	 			so it can be moved while there's vald ata in front of it)
	 */
	typedef struct query_buffer_ {
	    circular_buffer_t buffer;
	    uint32_t entry_ptr;
	} query_buffer_t;

	void QueryHandler_Init();

	void QueryHandler_Update(circular_buffer_t* rx_buf);
	bool QueryCheck();

	bool SetTime(char* clock_str);
	void DisplayTime(void);

	bool SetDate(char* date_str);
	void DisplayDate(void);

	bool SetAlarm(char* alarm_str);

	void Alarm_callback(void);

	void CursorCodeCheck(circular_buffer_t* rx_buf);

#endif	// COMMAND_HANDLER_H
