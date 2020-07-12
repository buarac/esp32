#include "esp_log.h"
#include "bme280.h"
#include <string.h>

static const char* TAG = "bme280";

esp_err_t bme280_init(bme280_t* bme, i2c_port_t port, uint8_t addr, uint8_t sda, uint8_t scl) {
    ESP_LOGV(TAG, "bme280_init()");

    esp_err_t ret = ESP_OK;
 
    if ( bme == NULL ) {
        ESP_LOGE(TAG, "bme280 object is null");
        return ESP_FAIL;
    }

    memset(bme, 0, sizeof(bme280_t));
    ret = i2c_device_init(&bme->device, port, addr, sda, scl);
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "i2c device init failed");
        return ret;
    }

    // check if bme280 device
    bme280_chip_id_get(bme);
    if ( bme->chip_id != BME280_CHIP_ID ) {
        ESP_LOGE(TAG, "Not a bme280 device. expected chip id = %d, found %d", BME280_CHIP_ID, bme->chip_id);
        return ESP_ERR_INVALID_ARG;
    }

    ret = bme280_soft_reset(bme);
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "soft reset failed");
        return ret;
    }
    // init bme280 params

    // load calibrate registers

    return ret;
}

esp_err_t bme280_done(bme280_t* bme) {
    ESP_LOGV(TAG, "bme280_done()");


    return ESP_OK;
}


esp_err_t bme280_chip_id_get(bme280_t* bme) {
    ESP_LOGV(TAG, "bme280_chip_id_get()");

    bme->cur_reg = BME280_REG_CHIP_ID;
    esp_err_t ret = i2c_device_read(&bme->device, &bme->cur_reg, 1, &bme->chip_id, 1);
    if ( ret != ESP_OK ) {
        bme->chip_id = 0;
    }
    return ret;
}

esp_err_t bme280_soft_reset(bme280_t* bme) {
    ESP_LOGV(TAG, "bme280_soft_reset");

    bme->cur_reg = BME280_REG_RESET;
    uint8_t data = BME280_RESET_VALUE;
    return i2c_device_write(&bme->device, &bme->cur_reg, 1, &data, 1);
}