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

#include "messaging.h"


#define UART_BAUD = 115200;


void UARTIntHandler(void);

void UARTSetup(uint32_t);




#endif /* UART_H_ */
