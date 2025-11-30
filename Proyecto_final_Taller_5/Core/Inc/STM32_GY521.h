#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "GY521_registers.h"

// ERROR CODES
#define GY521_OK                     0
#define GY521_ERROR_READ            -1
#define GY521_ERROR_WRITE           -2
#define GY521_ERROR_NOT_CONNECTED   -3

// CONVERSION CONSTANTS
#define GY521_RAW2DPS              (1.0 / 131.0)
#define GY521_RAW2G                (1.0 / 16384.0)

typedef struct {
    // I2C handle
    void* i2c_handle;
    uint8_t address;

    int16_t rawAx, rawAy, rawAz;
    int16_t rawGx, rawGy, rawGz;
    int16_t rawTemp;

    float ax, ay, az;
    float gx, gy, gz;
    float temperature;

    float _raw2g;
    float _raw2dps;

} GY521;

void GY521_Init(GY521 *dev, void* i2c_handle, uint8_t address);
bool GY521_begin(GY521 *dev);
bool GY521_isConnected(GY521 *dev);
void GY521_reset(GY521 *dev);
bool GY521_wakeup(GY521 *dev);
int16_t GY521_read(GY521 *dev);
bool GY521_setAccelSensitivity(GY521 *dev, uint8_t as);
bool GY521_setGyroSensitivity(GY521 *dev, uint8_t gs);
