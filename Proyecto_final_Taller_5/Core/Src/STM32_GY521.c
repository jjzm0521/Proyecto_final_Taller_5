#include "STM32_GY521.h"
#include <math.h>


#include "stm32f4xx_hal.h"

void GY521_Init(GY521 *dev, void* i2c_handle, uint8_t address) {
    dev->i2c_handle = i2c_handle;
    dev->address = address;
    dev->_raw2g = GY521_RAW2G;
    dev->_raw2dps = GY521_RAW2DPS;
    GY521_reset(dev);
}

bool GY521_begin(GY521 *dev) {
    if (GY521_isConnected(dev)) {
        GY521_wakeup(dev);
        GY521_setAccelSensitivity(dev, 0);
        GY521_setGyroSensitivity(dev, 0);
        return true;
    }
    return false;
}

bool GY521_isConnected(GY521 *dev) {
    return HAL_I2C_IsDeviceReady(dev->i2c_handle, dev->address << 1, 1, 1000) == HAL_OK;
}

void GY521_reset(GY521 *dev) {
    dev->rawAx = dev->rawAy = dev->rawAz = 0;
    dev->rawGx = dev->rawGy = dev->rawGz = 0;
    dev->rawTemp = 0;
    dev->ax = dev->ay = dev->az = 0;
    dev->gx = dev->gy = dev->gz = 0;
    dev->temperature = 0;
}

bool GY521_wakeup(GY521 *dev) {
    uint8_t data = 0x00;
    return HAL_I2C_Mem_Write(dev->i2c_handle, dev->address << 1, GY521_PWR_MGMT_1, 1, &data, 1, 1000) == HAL_OK;
}

int16_t GY521_read(GY521 *dev) {
    uint8_t data[14];
    if (HAL_I2C_Mem_Read(dev->i2c_handle, dev->address << 1, GY521_ACCEL_XOUT_H, 1, data, 14, 1000) != HAL_OK) {
        return GY521_ERROR_READ;
    }

    dev->rawAx = (int16_t)(data[0] << 8 | data[1]);
    dev->rawAy = (int16_t)(data[2] << 8 | data[3]);
    dev->rawAz = (int16_t)(data[4] << 8 | data[5]);
    dev->rawTemp = (int16_t)(data[6] << 8 | data[7]);
    dev->rawGx = (int16_t)(data[8] << 8 | data[9]);
    dev->rawGy = (int16_t)(data[10] << 8 | data[11]);
    dev->rawGz = (int16_t)(data[12] << 8 | data[13]);

    dev->ax = dev->rawAx * dev->_raw2g;
    dev->ay = dev->rawAy * dev->_raw2g;
    dev->az = dev->rawAz * dev->_raw2g;

    dev->gx = dev->rawGx * dev->_raw2dps;
    dev->gy = dev->rawGy * dev->_raw2dps;
    dev->gz = dev->rawGz * dev->_raw2dps;

    dev->temperature = (dev->rawTemp / 340.0) + 36.53;

    return GY521_OK;
}

bool GY521_setAccelSensitivity(GY521 *dev, uint8_t as) {
    if (as > 3) return false;
    uint8_t val;
    if (HAL_I2C_Mem_Read(dev->i2c_handle, dev->address << 1, GY521_ACCEL_CONFIG, 1, &val, 1, 1000) != HAL_OK) return false;
    val &= 0xE7;
    val |= (as << 3);
    if (HAL_I2C_Mem_Write(dev->i2c_handle, dev->address << 1, GY521_ACCEL_CONFIG, 1, &val, 1, 1000) != HAL_OK) return false;
    dev->_raw2g = (1 << as) * GY521_RAW2G;
    return true;
}

bool GY521_setGyroSensitivity(GY521 *dev, uint8_t gs) {
    if (gs > 3) return false;
    uint8_t val;
    if (HAL_I2C_Mem_Read(dev->i2c_handle, dev->address << 1, GY521_GYRO_CONFIG, 1, &val, 1, 1000) != HAL_OK) return false;
    val &= 0xE7;
    val |= (gs << 3);
    if (HAL_I2C_Mem_Write(dev->i2c_handle, dev->address << 1, GY521_GYRO_CONFIG, 1, &val, 1, 1000) != HAL_OK) return false;
    dev->_raw2dps = (1 << gs) * GY521_RAW2DPS;
    return true;
}
