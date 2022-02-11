/*
 * connection.h
 *
 *  Created on: Feb 10, 2022
 *      Author: Nate
 *
 *  Description:
 *      Handle the UART connection to desktop
 */

#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <stdint.h>
#include <stdbool.h>

void UARTSetup();

void UARTIntHandler(void);



void ConnectionTest(void);

#endif /* UART_H_ */
