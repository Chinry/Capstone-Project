/*
 * connection.c
 *
 *  Created on: Feb 10, 2022
 *      Author: Nate
 *
 *  Description:
 *      Handle the UART connection to desktop
 */


static bool acceptConnections = true;

// UART input received
void UARTIntHandler(void) {
    uint32_t status;
    status = UARTIntStatus(UART0_BASE, true);
    UARTIntClear(UART0_BASE, status);

    if (! acceptConnections) return;

    uint8_t next_char = '\0';

    if (UARTCharsAvail(UART0_BASE)) {
        next_char = (uint8_t) UARTCharGet(UART0_BASE);

        UARTCharPut(UART0_BASE, (char) next_char);
        UARTCharPut(UART0_BASE, '\n');
        UARTCharPut(UART0_BASE, '\r');
    }
}
