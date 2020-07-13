#ifndef _I2C_DEVICE_H_
#define _I2C_DEVICE_H_

#include "driver/i2c.h"

#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                       /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                             /*!< I2C ack value */
#define NACK_VAL 0x1                            /*!< I2C nack value */

typedef struct {
    uint8_t         addr;
    i2c_port_t      port;
    i2c_config_t    conf;
} i2c_device_t;

esp_err_t i2c_device_init(i2c_device_t* device, i2c_port_t port, uint8_t addr, uint8_t sda, uint8_t scl);

esp_err_t i2c_device_read(i2c_device_t* device, void* out_data, size_t out_size, void* in_data, size_t in_size);
esp_err_t i2c_device_write(i2c_device_t* device, void* reg_data, size_t reg_size, void* out_data, size_t out_size);

esp_err_t i2c_device_read_reg_uint8(i2c_device_t* device, uint8_t reg, uint8_t* data);
esp_err_t i2c_device_read_reg_int8(i2c_device_t* device, uint8_t reg, int8_t* data);
esp_err_t i2c_device_read_reg_uint16(i2c_device_t* device, uint8_t reg, uint16_t* data);
esp_err_t i2c_device_read_reg_int16(i2c_device_t* device, uint8_t reg, int16_t* data);


#endif // _I2C_DEVICE_H