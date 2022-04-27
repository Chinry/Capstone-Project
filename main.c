
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


    // CONFIGURE EEPROM =======================================================
    SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);
    EEPROMInit();

    LoadWindow();


    // CONFIGURE LEDS FOR PROCESS INFO ========================================
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0x00);


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


    // CONFIGURE INPUT
#ifdef USE_BUTTON_CALIBRATE
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
    GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0, GPIO_DIR_MODE_IN);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
#endif



    // CONFIGURE PWM ==========================================================
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_PWM0) && SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB))) {}

    GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_6);
    GPIOPinConfigure(GPIO_PB6_M0PWM0);
    SysCtlPWMClockSet(SYSCTL_PWMDIV_64);

    //First two parameters: PWM peripheral base address, use PWM generator 0,
    //The last parameter is a configuration integer
    //PWM_GEN_MODE_DOWN: specifies that the counter will start at a load value and count to zero. When it hits zero it will jump back to the load value
    //OR PWM_GEN_MODE_GEN_SYNC_LOCAL: updates happen only on 0;
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_GEN_SYNC_LOCAL);

    SetWave(1000, 128);

    //used to start/stop timers
    // Start the timers in generator 0.
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);

    //true = enable these bits
    //false = disable these bits
    PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT, true);



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

        // calibrating = GPIO button read

        // calibrate button down - skip DSP
        if (calibrating) {
            Calibrate();
            continue;
        }

        // check full buffer and state
        if (! buffer_filled || ! playing) {
            continue;
        }

        // pre-dsp stuff
        // disable interrupts while snapshot is made
        IntDisable(INT_TIMER0A);
        ADCIntDisable(ADC0_BASE, 1);

        // snapshot buffer for FFT
        int i, j;
        double mult;
        i = 0;
        j = samples_index; // head of ring buffer
        for (; i < BUF_SIZE; i++) {
#ifdef WINDOW_ON
            if (i < WINDOW_SIZE) {
                mult = window.window_d[WINDOW_SIZE - i];
                fr[i] = (int16_t) (mult * samples[j] + (1 - mult) * midpoint);
            }
            else if (i >= BUF_SIZE - WINDOW_SIZE) {
                mult = window.window_d[WINDOW_SIZE + i - BUF_SIZE + 1];
                fr[i] = (int16_t) (mult * samples[j] + (1 - mult) * midpoint);
            }
            else {
                fr[i] = samples[j];
            }
#else
            fr[i] = samples[j];
#endif // WINDOW_ON


            fi[i] = 0;

            j++;
            if (j == BUF_SIZE) {
                j = 0;
            }
        }

        // re-enable interrupts
        IntEnable(INT_TIMER0A);
        ADCIntEnable(ADC0_BASE, 1);

        // bulk of dsp work
        // this can be interrupted, snapshot is already taken
        RunDSP();

        // trigger PWM updates here if we want updates only on new data
        //UpdateOuput();
    }
}


// Calibrate the base signal
void Calibrate(void) {

    // disable interrupts while snapshot is made
    IntDisable(INT_TIMER0A);
    ADCIntDisable(ADC0_BASE, 1);

    int i, j;
    uint32_t acc;
    i = 0;
    j = samples_index; // head of ring buffer
    for (; i < BUF_SIZE; i++) {
        acc += samples[j];
        j++;
        if (j == BUF_SIZE) {
            j = 0;
        }
    }

    midpoint = acc / BUF_SIZE;

    // re-enable interrupts
    IntEnable(INT_TIMER0A);
    ADCIntEnable(ADC0_BASE, 1);
}


// Process buffer
void RunDSP(void) {

    fix_fft(fr, fi, 9, 0);

    int i, j, k;
    for (i = 0; i < LOCAL_MAX_SIZE; i++) {
        local_max[i] = 0;
    }

    uint16_t val;
    for (i = 2; i < BUF_ITER_MAX; i += LOCAL_WIDTH) {
        val = 0;
        for (k = 0; k < LOCAL_WIDTH; k++) {
            if (fr[i + k] < 0) {
                val += 0 - fr[i + k];
            }
            else {
                val += fr[i + k];
            }
        }
        val /= LOCAL_WIDTH;

        if (val > LOCAL_THRESHOLD) {
            if (local_max[0] == 0) {
                local_max[0] = val;
                local_max_idx[0] = i;
            }
            else {
                if (val < local_max[LOCAL_MAX_SIZE - 1]) {
                    continue;
                }
                for (j = LOCAL_MAX_SIZE - 1; j >= 0; j--) {
                    if (j > 0) {
                        if (val > local_max[j - 1]) {
                            local_max[j] = local_max[j - 1];
                            local_max_idx[j] = local_max_idx[j - 1];
                        }
                        else {
                            local_max[j] = val;
                            local_max_idx[j] = i + LOCAL_WIDTH / 2;
                            break;
                        }
                    }
                    else {
                        local_max[j] = val;
                        local_max_idx[j] = i + LOCAL_WIDTH / 2;
                    }
                }
            }
        }
    }

    freq_f = local_max_idx[0];

    process_count += 1;
}


// ADC interrupt routine
void AwaitADC(void) {

    // wait for ADC
    while (!ADCIntStatus(ADC0_BASE, 1, false)) {}

    // get ADC sequence and shift 12 bit samples -> 16 bit
    ADCSequenceDataGet(ADC0_BASE, 1, input_buffer);
    input_buffer[0] = input_buffer[0] << 3;
    input_buffer[1] = input_buffer[1] << 3;
    input_buffer[2] = input_buffer[2] << 3;
    input_buffer[3] = input_buffer[3] << 3;

    // take average of samples to produce single sample
    sample_value = (input_buffer[0] + input_buffer[1] + input_buffer[2] + input_buffer[3]) >> 2;

    //samples[samples_index] = SHRT_MIN + (int16_t) sample_value;
    samples[samples_index] = (int16_t) sample_value;

    // rotate buffer
    samples_last_index = samples_index;
    if (samples_index < BUF_SIZE)
    {
        samples_index += 1;
    }
    else {
        buffer_filled = 1; // buffer full = ready for dsp
        samples_index = 0;
    }

    sample_count++;

    uint16_t amp1;
    if (sample_value < midpoint) {
        amp1 = midpoint - sample_value;
    }
    else {
        amp1 = sample_value - midpoint;
    }

    // envelope accumulator isn't full yet
    if (! env_moving) {
        env_follower_avg += amp1;

        // finished filling the envelope accumulator
        if (samples_last_index == env_length - 1) {
            env_follower_avg = env_follower_avg >> ENV_SIZE;
            env_oldest_index = 0; // samples[0] is the oldest contribution to average
            env_moving = true;
        }
    }

    // accumulator full, move the average by one sample
    else {

        //env_follower_avg = (0 - amp2 + amp1) >> ENV_SIZE;
        env_follower_avg = ((env_follower_avg * (env_length - ENV_WEIGHT)) + (amp1 * ENV_WEIGHT)) >> ENV_SIZE;

        env_oldest_index++;
        if (env_oldest_index == BUF_SIZE) {
            env_oldest_index = 0;
        }
    }

    // amplitude above threshold?
    if (env_follower_avg > PLAYBACK_THRESHOLD) {
        env_keepalive = PLAYBACK_KEEPALIVE;
        playing = true;
    }
    else {
        if (env_keepalive == 0) {
            playing = false;
        }
        else {
            env_keepalive -= 1;
        }
    }

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


// Triggered by output timer (if we want periodic wave updates)
void Timer1IntHandler(void) {
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    // Update the PWM wave
    UpdateOuput();
}


// Update the PWM with frequency
void SetWave(uint32_t freq, uint32_t width /* 0 - 128 , 0: smallest, 128: halfway*/ ) {
    
    //Since the clock is divided by 16 (PWM Clock is 5Mhz), 1 step = 200ns
    //The period is (1/(freq * 4) / (1/5000000)
    uint32_t period = 5000000 / (freq << 2);

    // On time is (1/2) * (width/128) * period
    uint32_t ontime = (width * period) >> 8;

    //this will be used to set the period
    //PWM_GEN_0: use generator 0 as before
    //the last parameter is the number of clock ticks that will be used to determine the load value
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, period);


    //set to output 0
    //last is the number of clock ticks of the positive portion of the wave.
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, ontime);
}


// Update the PWM
void UpdateOuput() {
    if (calibrating) {
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0x04);
        return;
    }

    if (playing) {

#ifdef USE_PLAYBACK_LED
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0x02);
#endif

        // enable signal
        PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT, true);

        // update signal
        // set first param (frequency) based on tracked fundamental frequency
        // set second param (width) based on signal amplitude
        SetWave(1000, 128);
    }
    else {

#ifdef USE_PLAYBACK_LED
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0x00);
#endif

        // disable signal
        PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT, false);
    }
}


// One-time write for gaussian values
void LoadWindow(void) {
    EEPROMRead(window.window_uint, WINDOW_ADDR, sizeof(window.window_uint));

    if (window.window_uint[0] != WINDOW_FLAG) {
        EEPROMMassErase();

        // calculate gaussian
        int i;
        double j;
        window.window_uint[0] = WINDOW_FLAG;
        for (i = 1; i <= WINDOW_SIZE; i++) {
            window.window_d[i] = pow(_E, (-0.5 * pow(((double) (i - 1) / WINDOW_GAUSS_A), 2))) / (WINDOW_GAUSS_A * sqrt(2 * _PI));
            if (i == 1) {
                j = window.window_d[1];
            }
            window.window_d[i] /= j;
        }

        // write to eeprom
        EEPROMProgram(window.window_uint, WINDOW_ADDR, sizeof(window.window_uint));
    }

    window.window_uint[0] = 0;
}
