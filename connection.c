/*
 * connection.c
 *
 *  Created on: Feb 10, 2022
 *      Author: Nate
 *
 *  Description:
 *      Handle the UART connection to desktop
 */

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"

#include "connection.h"
#include "messaging.h"

static const uint32_t UART_BAUD = 115200;
static bool acceptConnections = true;

// UART input received
void UARTIntHandler(void) {
    uint32_t status;
    status = UARTIntStatus(UART0_BASE, true);
    UARTIntClear(UART0_BASE, status);

    if (! acceptConnections) return;

    // Echo test
    uint8_t next_char = '\0';
    if (UARTCharsAvail(UART0_BASE)) {
        next_char = (uint8_t) UARTCharGet(UART0_BASE);

        UARTCharPut(UART0_BASE, (char) next_char);
        UARTCharPut(UART0_BASE, ' ');
    }
}

// Enable code
void UARTSetup(const uint32_t sysclk) {

    acceptConnections = true;

    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTConfigSetExpClk(UART0_BASE, sysclk, UART_BAUD, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    IntEnable(INT_UART0);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
}


// Disable code
void UARTCleanup() {
    acceptConnections = false;

    IntDisable(INT_UART0);
}


