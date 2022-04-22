
/*
 * main.c - core device execution
 *
 *  Created on: Apr 4, 2022
 *      Authors: Nate, Nick
 */

#include "main.h"



void set_wave(uint32_t freq, uint32_t width /* 0 - 128 , 0: smallest, 128: halfway*/ )
{
        //Since the clock is divided by 16 (PWM Clock is 5Mhz), 1 step = 200ns

        //The period is (1/(freq * 4) / (1/5000000)
        uint32_t period = 5000000 / (freq<<2);


        //on time is (1/2) * (width/128) * period
        uint32_t ontime = (width * period)>>8;



        //this will be used to set the period
        //PWM_GEN_0: use generator 0 as before
        //the last parameter is the number of clock ticks that will be used to determine the load value
        PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, period);


        //set to output 0
        //last is the number of clock ticks of the positive portion of the wave.
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, ontime);
}





int main(void) {

    // configure the system clock at which system will run
    SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);



    //PWM: Has outputs 0 - 7 that result from generators 0 - 3





    //PB6's output is from M0PWM0
    //this is where we need output from


    //enable these peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_PWM0) && SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB)))
    {
    }


    //setup PB6 for use as a PWM pin
    //bit packet representation of which pins to select
    GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_6);


    GPIOPinConfigure(GPIO_PB6_M0PWM0);





    SysCtlPWMClockSet(SYSCTL_PWMDIV_64);


    //First two parameters: PWM peripheral base address, use PWM generator 0,
    //The last parameter is a configuration integer
    //PWM_GEN_MODE_DOWN: specifies that the counter will start at a load value and count to zero. When it hits zero it will jump back to the load value
    //OR PWM_GEN_MODE_GEN_SYNC_LOCAL: updates happen only on 0;
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_GEN_SYNC_LOCAL);

    set_wave(1000, 128);


    //used to start/stop timers
    // Start the timers in generator 0.
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);

    //enable/disable PWM outputs

    //ui32PWMOutBits: which output to enable
    //true = enable these bits
    //false = disable these bits
    PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT, true);




    // Enter main execution loop
    while(1) {}


}





