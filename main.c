
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/adc.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"


// Features
#define USE_ADC
#define USE_UART


// Conditional includes
#ifdef USE_ADC
#include "fix_fft.h"
#endif

#ifdef USE_UART
#include "connection.h"
#endif



// ADC/DSP data
#ifdef USE_ADC

#define BUF_SIZE 512
#define SAMP_FREQ 3000

int16_t fr[BUF_SIZE];
int16_t fi[BUF_SIZE];

uint32_t input_buffer[4];
int16_t samples[BUF_SIZE];
uint32_t samples_index;
uint32_t sample_value;

uint32_t buffer_filled = 0;
uint32_t timer_triggers = 0;

volatile unsigned short ulADC0Value;

#endif


/*
 * plan
 * - Have ADC run on a timer and put values into buffer
 * - fft reupdate always for pitch
 *
 *
 *
 */


int main(void) {

    //configure the system clock at which system will run
    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

#ifdef RUN_TEST

    ConnectionTest();

#endif // RUN_TEST

#ifdef USE_UART

    // connection.c UART setup
    UARTSetup();
    StorageSetup();

#endif

#ifdef USE_ADC

    timer_triggers = 0;

    buffer_filled = 0;
    samples_index = 0;
    sample_value = 0;

    uint32_t ui32Period = SysCtlClockGet() / 8000;

    // CONFIGURE ADC TIMER
    // Enable Timer 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    // Set as a full width periodic timer
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

    // Set to timer A when using full width, set the load value to 3000
    TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period);

    // Timer 0, Timer A, trigger the ADC
    TimerControlTrigger(TIMER0_BASE, TIMER_A, true);

    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);




    //CONFIGURE AUDIO ADC
    // Enable ADC0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    // 8x Oversampling (since the ADC only can acquire 10 bit samples it will make up for lack of fidelity by averaging multiple samples together)
    ADCHardwareOversampleConfigure(ADC0_BASE, 8);

    // Reference the internal 3v
    ADCReferenceSet(ADC0_BASE, ADC_REF_INT);

    // ADC0, sample sequencer 0, triggered explicitly by processor, priority low 3 -> high 0
    //ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_TIMER, 0);
    ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);

    //ADC0, sample sequencer 0, step 0, (sample channel 0, cause an interrupt when complete, configured as the last step in the sampling sequence)
    ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_CH0);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_CH0);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_CH0);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 3, ADC_CTL_CH0|ADC_CTL_IE|ADC_CTL_END);

    // Set up sequence 0 to be able to be triggered
    ADCSequenceEnable(ADC0_BASE, 1);


    // Trigger interrupt from timer
    //ADCIntEnableEx(ADC0_BASE, ADC_INT_SS0);

    // Starts timer
    TimerEnable(TIMER0_BASE, TIMER_A);

    // Master interrupt enable
    IntMasterEnable();

    // Main execution
    while(1) {

        // wait for ADC
        while (!ADCIntStatus(ADC0_BASE, 1, false)) {

            // continuous analysis of the buffer
            // buffer is full and ready to be processed
            if (buffer_filled) {

                // copy in circular buffer contents to fft buffer
                int i, j;
                for (i = 0; i < BUF_SIZE; i++) {
                    j = i + samples_index;
                    if (j >= BUF_SIZE) {
                        j -= BUF_SIZE;
                    }
                    fr[i] = samples[j];
                    fi[i] = 0;
                }

                fix_fft(fr, fi, 9, 0);
            }
        }

        // clear ADC
        ADCIntClear(ADC0_BASE, 1);

        //uint32_t immediate_value;

        ADCSequenceDataGet(ADC0_BASE, 1, input_buffer);
        sample_value = (input_buffer[0] + input_buffer[1] + input_buffer[2] + input_buffer[3]) / 4;
        sample_value = sample_value << 4;
        samples[samples_index] = SHRT_MIN + (int16_t) sample_value;

//        //convert unsigned 32bit to signed 16bit
//        int32_t tmp = immediate_value + 0x10000000;
//        //store into circular buffer
//        samples[samples_index] = (int16_t)(tmp >> 16);

        // rotate
        if (samples_index < BUF_SIZE)
        {
            samples_index += 1;
        }
        else {
            buffer_filled = 1; // ready for dsp
            samples_index = 0;
        }
    }

#endif // USE_ADC
}


// Triggered by ADC timer
void Timer0IntHandler(void) {
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    timer_triggers++;

    // trigger ADC
    ADCProcessorTrigger(ADC0_BASE, 1);
}


// ADC handler - not used
void ADC0_Handler(void)
{
#ifdef USE_ADC

    ADCIntClear(ADC0_BASE, 1);

    uint32_t immediate_value;

    ADCSequenceDataGet(ADC0_BASE, 1, &immediate_value);

    //convert unsigned 32bit to signed 16bit
    int32_t tmp = immediate_value + 0x10000000;
    //store into circular buffer
    samples[samples_index] = (int16_t)(tmp >> 16);

    //advance buffer write index
    if (samples_index < BUF_SIZE)
    {
        samples_index++;
    }
    else {
        samples_index = 0;
    }
#endif
}


