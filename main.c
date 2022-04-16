
/*
 * main.c - core device execution
 *
 *  Created on: Apr 4, 2022
 *      Authors: Nate, Nick
 */

#include "main.h"


int main(void) {

    // configure the system clock at which system will run
    SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

#ifdef RUN_TEST

    ConnectionTest();

#endif // RUN_TEST

#ifdef USE_UART

    // connection.c UART setup
    UARTSetup();
    StorageSetup();

#endif


    // CONFIGURE DSP PROCESS STATE ============================================
    // Set clock loads
    uint32_t samplingPeriod = SysCtlClockGet() / SAMPLE_FREQ;
    uint32_t outputPeriod = SysCtlClockGet() / OUTPUT_FREQ;

    // Initialize counters
    process_count = 0;
    sample_count = 0;
    buffer_filled = 0;
    timer_triggers = 0;

    // Initialize sample memory
    samples_index = 0;
    sample_value = 0;


    // CONFIGURE SAMPLING TIMER ===============================================
    // Enable Timer 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    // Set as a full width periodic timer
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

    // Set processing and sampling loads
    TimerLoadSet(TIMER0_BASE, TIMER_A, samplingPeriod);

    // Timer 0, Timer A, trigger the ADC - using timer interrupt instead
    //TimerControlTrigger(TIMER0_BASE, TIMER_A, true);

    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);



    // CONFIGURE OUTPUT TIMER =================================================
    // Enable Timer 1
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);

    // Periodic
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);

    // Set timer 1, timer A load
    TimerLoadSet(TIMER1_BASE, TIMER_A, outputPeriod);

    IntEnable(INT_TIMER1A);
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);



    // CONFIGURE PWM
    SysCtlPWMClockSet(SYSCTL_PWMDIV_64);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_0);



    //CONFIGURE AUDIO ADC =====================================================
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
    ADCIntEnable(ADC0_BASE, 1);

    // Starts timers
    TimerEnable(TIMER0_BASE, TIMER_A);
    TimerEnable(TIMER1_BASE, TIMER_A);

    // Master interrupt enable
    IntMasterEnable();



    // Enter main execution loop
    while(1) {

    // buffer is full and ready to be processed
        if (buffer_filled) {

            // snapshot buffer for FFT
            // disable ADC
            IntDisable(INT_TIMER0A);
            ADCIntDisable(ADC0_BASE, 1);
            int i, j;
            for (i = 0; i < BUF_SIZE; i++) {
                fr[i] = BufferValue(j);
                fi[i] = 0;
            }
            IntEnable(INT_TIMER0A);
            ADCIntEnable(ADC0_BASE, 1);

            // this can be interrupted, snapshot is already taken
            RunDSP();
        }
    }
}

// Process buffer
void RunDSP(void) {

    // copy in circular buffer contents to fft buffer
    int i, j;
    uint32_t rms;

    // envelope is moving, update RMS one value at a time
    if (env_moving) {
        if (samples_index != env_last_index) {
            if ((env_last_index < ENV_SIZE) == 0) {
                j = env_last_index - ENV_SIZE + BUF_SIZE;
            }
            else {
                j = env_last_index - ENV_SIZE;
            }

            rms = env_follower_rms * env_follower_rms + ((samples[env_last_index] * samples[env_last_index]) - (samples[j] * samples[j])) / ENV_SIZE;
            rms = (uint16_t) sqrt((double) rms);
            env_follower_rms = rms;

            // update latest contributing value to the RMS
            env_last_index = samples_index;
        }
    }

    // compute total envelope over ENV_SIZE subset of buffer
    else {
        rms = 0;
        for (i = 0; i < ENV_SIZE; i++) {
            j = BufferIndex(samples_index - ENV_SIZE);
            rms += samples[j] * samples[j];
        }
        rms /= ENV_SIZE;
        rms = (uint16_t) sqrt((double) rms);
        env_follower_rms = rms;

    }

    playing = env_follower_rms > PLAYBACK_THRESHOLD;

    // only do DSP if there should be playback
    if (playing && samples_last_index != samples_index) {

        // snapshot buffer for FFT
        for (i = 0; i < BUF_SIZE; i++) {
            fr[i] = BufferValue(j);
            fi[i] = 0;
        }

        fix_fft(fr, fi, 9, 0);
    }


    process_count += 1;
}


void AwaitADC(void) {

    // wait for ADC
    while (!ADCIntStatus(ADC0_BASE, 1, false)) {}

    ADCSequenceDataGet(ADC0_BASE, 1, input_buffer);
    sample_value = (input_buffer[0] + input_buffer[1] + input_buffer[2] + input_buffer[3]) / 4;
    sample_value = sample_value << 4;
    samples[samples_index] = SHRT_MIN + (int16_t) sample_value;

    // rotate
    samples_last_index = samples_index;
    if (samples_index < BUF_SIZE)
    {
        samples_index += 1;
    }
    else {
        buffer_filled = 1; // ready for dsp
        samples_index = 0;
    }

    sample_count++;

    // clear ADC
    ADCIntClear(ADC0_BASE, 1);
}



// Triggered by sampling timer
void Timer0IntHandler(void) {
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    ADCProcessorTrigger(ADC0_BASE, 1);
    AwaitADC();

    timer_triggers++;
}


// Triggered by output timer
void Timer1IntHandler(void) {
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    // update playback wave
    //EnableSignal(playing);
    if (playing) {
        //UpdateSignal(0, 0);
    }
}



