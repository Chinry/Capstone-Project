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
#include "storage.h"

static const uint32_t UART_BAUD = 115200;
static bool acceptConnections = true;


const char test_keyval[] = "interval=2/1";

char cmd[CMD_LEN];
char cmd_snapshot[CMD_LEN];
uint16_t cmd_idx = 0;

char msg_process[] = "Processing command...";
char msg_success[] = "Success!";
char msg_fail[] = "Failure (code=1)";

// Enable code
void UARTSetup() {

    acceptConnections = true;

    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), UART_BAUD, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    IntEnable(INT_UART0);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

    int i;
    for (i = 0; i < CMD_LEN; i++) {
        cmd[i] = '\0';
    }
    cmd_idx = 0;
}


// Disable code
void UARTCleanup() {

    acceptConnections = false;

    IntDisable(INT_UART0);
}


// UART input received
void UARTIntHandler(void) {

    uint32_t status;
    status = UARTIntStatus(UART0_BASE, true);
    UARTIntClear(UART0_BASE, status);

    if (! acceptConnections) return;

    // Echo test
//    uint8_t next_char = '\0';
//    if (UARTCharsAvail(UART0_BASE)) {
//        next_char = (uint8_t) UARTCharGet(UART0_BASE);
//
//        UARTCharPut(UART0_BASE, (char) next_char);
//        UARTCharPut(UART0_BASE, ' ');
//    }

    char next_char = '\0';
    if (UARTCharsAvail(UART0_BASE)) {
        next_char = UARTCharGet(UART0_BASE);

        if (next_char == (char) 10) {

            memcpy(&cmd_snapshot[0], &cmd[0], sizeof(cmd));

#ifdef RAW_MESSAGES
            StringPut(&msg_process[0], sizeof(msg_process));

            status_code_t res;
            res = UpdateStore(&cmd_snapshot[0]);

            if (res == 0) {
                StringPut(&msg_success[0], sizeof(msg_success));
            }
            else {
                msg_fail[14] = (char) (res + 48);
                StringPut(&msg_fail[0], sizeof(msg_fail));
            }

            int i;
            for (i = 0; i < CMD_LEN; i++) {
                cmd[i] = '\0';
            }
            cmd_idx = 0;

            UARTCharPut(UART0_BASE, '\n');
            UARTCharPut(UART0_BASE, '\r');
#endif
        }
        else {
            UARTCharPut(UART0_BASE, (char) next_char);
            if (cmd_idx < CMD_LEN) {
                cmd[cmd_idx++] = next_char;
            }
        }
    }

}

// Test connection
void ConnectionTest(void) {
    status_code_t code = UpdateStore(test_keyval);
}





