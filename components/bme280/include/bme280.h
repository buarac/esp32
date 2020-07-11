#ifndef _BME280_DEVICE_H_
#define _BME280_DEVICE_H_

#include "i2c_device.h"

typedef struct {
    i2c_device_t*   device;

} bme280_t;

void bme280_device_info(i2c_device_t* dev);

esp_err_t bme280_device_init(bme280_t* bme280, uint8_t addr, i2c_port_t port);

#endif // _BME280_DEVICE_H_