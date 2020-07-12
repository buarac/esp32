#ifndef _BME280_DEVICE_H_
#define _BME280_DEVICE_H_

#include "i2c_device.h"

// BME280 registers
#define BME280_REG_CHIP_ID  0xd0
#define BME280_REG_RESET    0xe0
#define BME280_REG_CONFIG   0xf5
#define BME280_REG_STATUS   0xf3
#define BME280_REG_PRES_MSB     0xf7
#define BME280_REG_PRES_LSB     0xf8
#define BME280_REG_PRES_XLSB    0xf9
#define BME280_REG_TEMP_MSB     0xfa
#define BME280_REG_TEMP_LSB     0xfb
#define BME280_REG_TEMP_XLSB    0xfc
#define BME280_REG_HUM_MSB      0xfd
#define BME280_REG_HUM_LSB      0xfe

// BME 280 CHID ID
#define BME280_CHIP_ID  0x60
// BME 280 SOFT RESET WORD
#define BME280_RESET_VALUE 0xb6

typedef enum {
    BME280_OVERSAMPLING_SKIPPED = 0,
    BME280_OVERSAMPLING_1,
    BME280_OVERSAMPLING_2,
    BME280_OVERSAMPLING_4,
    BME280_OVERSAMPLING_16
} bme280_oversampling_t;

typedef enum {
    BME280_SLEEP_MODE = 0,
    BME280_FORCED_MODE,
    BME280_NORMAL_MODE = 3
} bme280_mode_t;

typedef enum {
    BME280_FILTER_OFF = 0,
    BME280_FILTER_2,
    BME280_FILTER_4,
    BME280_FILTER_8,
    BME280_FILTER_16
} bme280_filter_t;

typedef struct {
    i2c_device_t device;
    uint8_t      chip_id;
    uint8_t      cur_reg;
} bme280_t;

esp_err_t bme280_init(bme280_t* bme, i2c_port_t port, uint8_t addr, uint8_t sda, uint8_t scl);
esp_err_t bme280_done(bme280_t* bme);

esp_err_t bme280_chip_id_get(bme280_t* bme);  
esp_err_t bme280_soft_reset(bme280_t* bme);

#endif // _BME280_DEVICE_H_