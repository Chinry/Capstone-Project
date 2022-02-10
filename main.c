#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/adc.h"
#include "driverlib/timer.h"


#include "fix_fft.h"

#define BUF_SIZE 512
#define SAMP_FREQ 3000

int16_t fr[BUF_SIZE];
int16_t fi[BUF_SIZE];


int16_t samples[BUF_SIZE];
uint32_t samples_index;

volatile unsigned short ulADC0Value;


/*
 * plan
 * - Have ADC run on a timer and put values into buffer
 * - fft reupdate always for pitch
 *
 *
 *
 */



int main(void)
{
    samples_index = 0;

    //configure the system clock at which system will run
    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

    //CONFIGURE ADC TIMER

    //enable Timer 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    //set as a full width periodic timer
    TimerConfigure(TIMER0_BASE, TIMER_CFG_A_PERIODIC);

    //set to timer A when using full width, set the load value to 3000
    TimerLoadSet(TIMER0_BASE, TIMER_A, 3000);

    //Timer 0, Timer A, trigger the ADC
    TimerControlTrigger(TIMER0_BASE, TIMER_A, true);




    //CONFIGURE AUDIO ADC
    //enable ADC0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    //8x Oversampling (since the ADC only can acquire 10 bit samples it will make up for lack of fidelity by averaging multiple samples together)
    ADCHardwareOversampleConfigure(ADC0_BASE,8);

    //reference the internal 3v
    ADCReferenceSet(ADC0_BASE,ADC_REF_INT);

    //ADC0, sample sequencer 0, triggered explicitly by processor, priority low 3 -> high 0
    ADCSequenceConfigure(ADC0_BASE,0,ADC_TRIGGER_TIMER,0);

    //ADC0, sample sequencer 0, step 0, (sample channel 0, cause an interrupt when complete, configured as the last step in the sampling sequence)
    ADCSequenceStepConfigure(ADC0_BASE,0,0,ADC_CTL_IE|ADC_CTL_CH0|ADC_CTL_END);

    //trigger interrupt from timer
    ADCIntEnableEx(ADC0_BASE, ADC_INT_SS0);


    //set up sequence 0 to be able to be triggered
    ADCSequenceEnable(ADC0_BASE,0);



    //starts timer
    TimerEnable(TIMER0_BASE, TIMER_A);



    /*
    int i;
    for( i = 0; i < BUF_SIZE; i++)
    {
        fr[i] = (int16_t)(30000 * sin(((2 * M_PI) / 45) * i));
        fi[i] = 0;
    }
    int scale  = fix_fft(fr, fi, 9, 0);
    */

    for(;;){}


    return 0;
}


void
ADC0_Handler(void)
{
    //halt until ADC is ready to be serviced
    while(!ADCIntStatus(ADC0_BASE, 0, false)){}

    //used for transferring the stored sampled value
    uint32_t immediate_value;

    //transfer data into 32 bit buffer
    ADCSequenceDataGet(ADC0_BASE, 0, &immediate_value);

    //convert unsigned 32bit to signed 16bit
    int32_t tmp = immediate_value + 0x10000000;
    //store into circular buffer
    samples[samples_index] = (int16_t)(tmp >> 16);

    //advance buffer write index
    if (samples_index < BUF_SIZE)
    {
        samples_index++;
        return;
    }
    samples_index = 0;
}
