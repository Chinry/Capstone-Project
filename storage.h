/*
 * storage.h
 *
 *  Created on: Feb 10, 2022
 *      Author: Nate
 *
 *  Description:
 *      Single-file short library for persisting data
 */

#ifndef STORAGE_H_
#define STORAGE_H_

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define PARAM_KEY_MAX 16
#define PARAM_VAL_MAX 16

#define PARAM_INTERVAL "interval"
#define PARAM_WAVE "wave"

#define WAVE_TYPE_COUNT 2
#define WAVE_SQUARE 0x00000000
#define WAVE_SINE 0x00000001

#define STATUS_PARAM_UPDATE_SUCCESS 0x00000000
#define STATUS_PARAM_NOT_FOUND 0x00000001
#define STATUS_PARAM_UPDATE_FAIL 0x00000002
#define STATUS_PARAM_BAD_FORMAT 0x00000003

#define num(c) c - 48

void StorateSetup(void);


// Storage status code type
typedef uint8_t status_code_t;

// CONFIGURATION STRUCT
typedef struct {

    // Harmony interval rational expression
    uint8_t intervalN;
    uint8_t intervalD;

    // Wave type
    uint8_t waveType;

} config_store_t;


// Initialize
void StorageSetup(void);

// Read and write config store struct to onboard storage
bool WriteStore(void);
bool ReadStore(void);

// Update config store
status_code_t UpdateStore(const char* keyval);
status_code_t ParamIntervalUpdateHandler(const char* val);
status_code_t ParamWaveUpdateHandler(const char* val);

// Utility
bool GetNumUntil(char* ptr, uint32_t* num, const char delim);
int32_t GetStrUntil(const char* str, uint32_t beginIdx, const char delim);


// Current configuration
config_store_t config_store;


// Default configuration
config_store_t config_default = {
    .intervalN      = 1,
    .intervalD      = 1,
    .waveType       = WAVE_SQUARE
};



// Storage function ===========================================
// Called on startup
void StorageSetup(void) {

    bool inStorage = false;

    // Read from storage
    if (inStorage) {
        if(! ReadStore()) {
            config_store = config_default;
        }
    }

    // Copy from default config
    else {
        config_store = config_default;
    }
}

// Persist config_store to storage
bool WriteStore(void) {
    return true;
}

// Load config_store from storage
bool ReadStore(void) {
    return true;
}

// Update param by name "param" to value "val"
status_code_t UpdateStore(const char* keyval) {

    // Separate key=value
    char param[PARAM_KEY_MAX];
    char val[PARAM_VAL_MAX];


    // Seek key substring
    int32_t eqIdx = GetStrUntil(keyval, 0, '=');

    if (eqIdx < 0) return STATUS_PARAM_BAD_FORMAT;
    if (eqIdx >= PARAM_KEY_MAX - 1) return STATUS_PARAM_BAD_FORMAT;

    strncpy(param, keyval, eqIdx);
    param[eqIdx] = '\0';


    // Seek value substring
    int32_t endIdx = GetStrUntil(keyval, eqIdx + 1, ' ');
    if (eqIdx < 0) return STATUS_PARAM_BAD_FORMAT;
    endIdx -= eqIdx + 1;

    if (endIdx >= PARAM_VAL_MAX - 1) return STATUS_PARAM_BAD_FORMAT;

    strncpy(val, &keyval[eqIdx + 1], endIdx);
    val[endIdx] = '\0';


    // Find a handler
    if (strcmp(param, PARAM_INTERVAL) == 0) return ParamIntervalUpdateHandler(val);
    if (strcmp(param, PARAM_WAVE) == 0) return ParamWaveUpdateHandler(val);

    // Param not found
    return STATUS_PARAM_NOT_FOUND;
}



// Param update handlers ===========================================
// Interval param update handler
status_code_t ParamIntervalUpdateHandler(const char* val) {

    char* c = (char*) *(&val);

    // Get numerator
    uint32_t num;
    if (! GetNumUntil(c, &num, '/')) return STATUS_PARAM_UPDATE_FAIL;

    // jump over '/'
    c++;

    // Get denominator
    uint32_t den;
    if (! GetNumUntil(c, &den, ' ')) return STATUS_PARAM_UPDATE_FAIL;

    config_store.intervalN = num;
    config_store.intervalD = den;

    return STATUS_PARAM_UPDATE_SUCCESS;
}

// Wave param update handler
status_code_t ParamWaveUpdateHandler(const char* val) {

    char* c = (char*) *(&val);

    uint32_t waveNum;

    if (! GetNumUntil(c, &waveNum, ' ')) return STATUS_PARAM_UPDATE_FAIL;

    if (waveNum < WAVE_TYPE_COUNT) {
        config_store.waveType = waveNum;
    }
    else {
        return STATUS_PARAM_UPDATE_FAIL;
    }

    return STATUS_PARAM_UPDATE_SUCCESS;
}



// Parse utility ===========================================
// Get index of substring end until delimiting character
int32_t GetStrUntil(const char* str, uint32_t beginIdx, const char delim) {

    uint32_t endIdx = beginIdx;

    while (str[endIdx] && str[endIdx] != delim) {
        endIdx++;
    };

    if (! str[endIdx]) {
        return -1;
    }

    return endIdx;
}


// Parse unsigned integer from string until a delimiting character
bool GetNumUntil(char* ptr, uint32_t* num, const char delim) {

    *num = 0;

    while (*ptr && *ptr != delim) {
        uint8_t digit = (uint8_t) num((uint8_t) *ptr);

        if (digit <= 9) {
            *num *= 10;
            *num += digit;
        }
        else {
            return false;
        }

        ptr++;
    }

    return *ptr;
}


#endif /* STORAGE_H_ */
