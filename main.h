/*
 * main.h
 *
 *  Created on: Apr 4, 2022
 *      Author: Nate
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
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
#include "driverlib/rom.h"


// Features
#define USE_ADC
#define USE_UART
#define ENV_LED
//#define ENV_MOVE


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
#define SAMPLE_FREQ 3000
#define OUTPUT_FREQ 8000
#define PLAYBACK_THRESHOLD 100
#define ENV_SIZE 4

// FFT buffers
int16_t fr[BUF_SIZE];
int16_t fi[BUF_SIZE];


// ADC sequence buffer
uint32_t input_buffer[4];

// circular input buffer
int16_t samples[BUF_SIZE];

// circular buffer next write index
uint32_t samples_index = 0;
uint32_t samples_last_index = 0;

// averaged sequence value
uint32_t sample_value = 0;

// moving envelope follower
bool env_moving = false;
uint32_t env_last_index = 0;
uint16_t env_follower_rms = 0;

// playback state
bool playing = false;
uint16_t playback_freq = 0;
uint16_t playback_amp = 0;

// debug information
uint32_t buffer_filled = 0;
uint32_t timer_triggers = 0;
uint32_t sample_count = 0;
uint32_t process_count = 0;

// functions
void AwaitADC(void);
void RunDSP(void);

void set_wave(uint32_t freq, uint32_t width);

inline uint32_t BufferIndex(int32_t index) {
    index += samples_index;
    while (index < 0) {
        index += BUF_SIZE;
    }
    return index % BUF_SIZE;
}

inline int16_t BufferValue(int32_t index) {
    return samples[BufferIndex(index)];
}

extern void StorageSetup(void);

#endif // USE_ADC

#endif /* MAIN_H_ */
