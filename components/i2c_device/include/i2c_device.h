#ifndef _I2C_DEVICE_H_
#define _I2C_DEVICE_H_

#include "driver/i2c.h"


typedef struct {
    uint8_t         addr;
    i2c_port_t      port;
    i2c_config_t    conf;
} i2c_device_t;

void i2c_device_info(i2c_device_t* dev);

esp_err_t i2c_device_init(i2c_device_t* device);

#endif // _I2C_DEVICE_H