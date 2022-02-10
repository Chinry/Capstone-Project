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

#include <messaging.h>

#ifndef MESSAGING_H_
#define MESSAGING_H_

#define loc(c) c - 48 // ASCII to index


typedef struct message_t {

};

// Send a string via UART0
void UART0StringPut(const char* str, const uint32_t len) {
    uint32_t strIdx;
    for(strIdx = 0; strIdx < len; strIdx++) {
        UARTCharPut(UART0_BASE, str[strIdx]);
    }
}

// Send the board via UART0
void UART0MessagePut(void) {
    uint32_t i;

    // Top of the board
    UARTCharPut(UART0_BASE, '.');
    for (i = 0; i < 7; i++) {
        UARTCharPut(UART0_BASE, '-');
    }
}

#endif /* MESSAGING_H_ */
