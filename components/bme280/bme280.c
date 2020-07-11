#include "esp_log.h"
#include "bme280.h"
#include <string.h>

static const char* TAG = "bme280";

void bme280_device_info(i2c_device_t* dev) {
    ESP_LOGV(TAG, "information about bme280 device");
    i2c_device_info(dev);
}

esp_err_t bme280_device_init(bme280_t* bme280, uint8_t addr, i2c_port_t port) {
    ESP_LOGD(TAG, "bme280_device_init");

    esp_err_t ret = ESP_OK;

    if ( bme280 == NULL ) {
        ESP_LOGE(TAG, "bme280 device object is null");
        return ESP_FAIL;
    }

    memset(bme280, 0, sizeof(bme280_t));
    bme280->device = malloc(sizeof(i2c_device_t));
    if ( bme280->device == NULL) {
        ESP_LOGE(TAG, "malloc for i2c device data failed");
        return ESP_FAIL;
    }

    i2c_device_t* dev = bme280->device;
    dev->addr = addr;
    dev->port = port;

    return ret;
}