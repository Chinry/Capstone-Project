/*
 * main.h
 *
 *  Created on: Apr 4, 2022
 *      Authors: Nate, Nick
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
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/adc.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/eeprom.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"


// Features
#define USE_UART
#define USE_PWM_TEST
//#define USE_BUTTON_CALIBRATE

// Modes
#define WINDOW_ON


#include "fix_fft.h"

#ifdef USE_UART
#include "connection.h"
#endif



#define _E 2.71828182845
#define _PI 3.14159265359


// Number of 16-bit samples stored in a ring buffer
#define BUF_SIZE 512

// Sample rate
#define SAMPLE_FREQ 3000

// Update output signal rate
#define OUTPUT_FREQ 4000


// ENVELOPE ===================================================================
// Define size of envelope follower, where actual size = 2 ^ ENV_SIZE
// Must be: 1 < actual size < BUF_SIZE (cannot be equal to BUF_SIZE)
#define ENV_SIZE 6

// Weight of the latest incoming sample on the moving average (ex. 2/64 for ENV_SIZE=6)
#define ENV_WEIGHT 2

// Distance between MIDPOINT and envelope average required for playback - main sensitivity factor
#define PLAYBACK_THRESHOLD 150

// Time in terms of sample frequency where playback should sustain, even after the envelope falls below the threshold
#define PLAYBACK_KEEPALIVE 1000 // 1000 = 1/3 of a second (PLAYBACK_KEEPALIVE / SAMPLE_FREQ)

// This value represents the bit-shifted ADC reading at "0" input. Might have to be re-calibrated over time
#define MIDPOINT 25000
// ============================================================================


// WINDOWING ==================================================================
#define WINDOW_ADDR 0x00
#define WINDOW_FLAG 123
#define WINDOW_SIZE 16
#define WINDOW_GAUSS_A 6.0
// ============================================================================

// PEAKS ======================================================================
#define LOCAL_MAX_SIZE 1 // number of maxima to store, ordered
#define LOCAL_THRESHOLD 100
#define LOCAL_WIDTH 1 // look for local maxima over averaged values with this width

#define BUF_ITER_MAX (BUF_SIZE >> 1) - LOCAL_WIDTH + 1
// ============================================================================



// For testing...
#ifdef USE_PWM_TEST
#define PWM_FREQUENCY 80
volatile uint32_t pwm_load;
volatile uint32_t pwm_adjust = 0;
#endif


// FFT buffers
int16_t fr[BUF_SIZE];
int16_t fi[BUF_SIZE];

// ADC sequence buffer
uint32_t input_buffer[4];

// circular input buffer
int16_t samples[BUF_SIZE];

// window function store
union window_t {
    uint32_t window_uint[WINDOW_SIZE + 1];
    double window_d[WINDOW_SIZE + 1];
} window;

// local maximums
uint16_t local_max[LOCAL_MAX_SIZE];
uint16_t local_max_idx[LOCAL_MAX_SIZE];
uint16_t* local_smallest = &local_max[0];

// frequency
uint16_t freq_f = 0;

// circular buffer next write index
uint32_t samples_index = 0;
uint32_t samples_last_index = 0;

// averaged sequence value
uint32_t sample_value = 0;

// moving envelope follower
bool env_moving = false;
uint32_t env_oldest_index = 0;
uint32_t env_follower_avg = 0;
uint32_t env_length = 0x1 << ENV_SIZE;
uint32_t env_keepalive = 0;

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
void LoadWindow(void);

void SetWave(uint32_t freq, uint32_t width);

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


#endif /* MAIN_H_ */
