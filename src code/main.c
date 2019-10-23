
/**
 * @file    main.c
 * @brief   Main function body for the interrupt-driven monitor program.
 * @author  Manuel Burnay
 * @date    2019.09.17  (Created)
 * @date    2019.10.10  (Last Modified)
 */

/**
 * @mainpage    Interrupt-driven monitor
 *
 * @section     Project Information
 *              This monitor allows for the user to query certain aspects of the system in real time.
 *              The current supported queries are: Display/Set time, Display/Set date, Set/Clear alarm.
 *              This monitor uses interrupts to fetch user data and to keep track of time, making it "interrupt-driven".
 *
 * @section     Communications
 *              Monitor communicates with the user via UART,
 *              which can be accessed by a computer via a Serial COM port and an emulated terminal program like PuTTY.
 *
 * @subsection  Serial Port Settings
 *              * 115200 baud rate,
 *              * 8 data bits,
 *              * 1 stop bit,
 *              * NO parity,
 *              * NO flow control.
 *
 *              Check device manager (or equivalent) to see which COM port the board is connected to.
 *                  - Name of board is "Stellaris Virtual Serial Port"
 *
 *              It is also recommended that you enable implicit CR in every LR & implicit LR in every CR on your terminal settings.
 *
 * @section     Queries
 *              All query entries are case insensitive.
 *              Keep in mind the format of the 'set' queries (the time seperators are different from the date seperators).
 *
 *              Display Time Query: <time>. \n
 *              Set Time Query: <time hh:mm:ss.t>. (all values are decimal)
 *
 *              Display Date Query: <date>. \n
 *              Set Date Query: <date dd-"mmm"-yyyy>. (days and years are decimal values, month is the first three letters of the month)
 *
 *              Clear alarm Query: <alarm>. \n
 *              Set Alarm Query: <alarm hh:mm:ss.t> (all values are decimal)
 */


#include <stdbool.h>
#include "uart.h"
#include "systick.h"
#include "systime.h"
#include "query_handler.h"

/**
 * @brief   Entry point to the monitor program
 */
int main(void)
{
    uart_descriptor_t uart = {.echo = true};    // initialize uart descriptor.

    UART0_Init(&uart);      // initialize uart driver.
    systime_init();         // initialize systime.
    QueryHandler_Init();    // initialize the Query Handler.

    SysTick_Start();


    while (1) {
        if (buffer_size(&uart.rx)) {
            QueryHandler_Update(&uart.rx);
        }
    }

	return 0;
}
