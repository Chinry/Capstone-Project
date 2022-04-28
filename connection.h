/*
 * connection.h
 *
 *  Created on: Feb 10, 2022
 *      Authors: Nate, Nick
 *
 *  Description:
 *      Handle the UART connection to desktop
 */

#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <stdint.h>
#include <stdbool.h>


#define RAW_MESSAGES true

#define CMD_LEN 64


void UARTSetup();
void UARTIntHandler(void);
void ConnectionTest(void);

#endif /* UART_H_ */
