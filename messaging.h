/*
 * uart.h
 *
 *  Created on: Feb 10, 2022
 *      Author: Nate
 *
 *  Description:
 *      Single-file short library for messaging protocol
 *      Also holds string tables for messages, and message structs
 */


#ifndef MESSAGING_H_
#define MESSAGING_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/debug.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/udma.h"


#define MSG_ACTION_LEN = 16;
#define MSG_DATA_LEN = 128;

// [ID] [status] [action] [length] [[key=value]...]
// ex. 123 0 putone 1 wave=square
// ex. 124 0 putmany 2 wave=square interval=1/2
typedef struct {
    uint8_t id;
    uint8_t status;
    char *action;
    uint8_t length;
    char *data;
} data_message_t;


// [ID] [status] [action]
// ex. 125 0 get
typedef struct stat_message_t {
    uint8_t id;
    uint8_t status;
    char *action;
} stat_message_t;


void StringPut(const char*, const uint32_t);
void DataMessagePut(data_message_t);
void StatMessagePut(stat_message_t);




// Send a string via UART0
void StringPut(const char* str, const uint32_t len) {
    uint32_t strIdx;
    for(strIdx = 0; strIdx < len; strIdx++) {
        UARTCharPut(UART0_BASE, str[strIdx]);
    }
}

// Send data message via UART0
void DataMessagePut(data_message_t data_msg) {

    UARTCharPut(UART0_BASE, '.');
}

// Send status message via UART0
void StatMessagePut(stat_message_t stat_msg) {

    UARTCharPut(UART0_BASE, '.');
}





#endif /* MESSAGING_H_ */
