#include "esp_log.h"
#include "i2c_device.h"

static const char* TAG = "i2c_device";

void i2c_device_info(i2c_device_t* dev) {
    ESP_LOGV(TAG, "information about i2c device");
    ESP_LOGV(TAG, "device addr = %d, 0x%02x", dev->addr, dev->addr);
    ESP_LOGV(TAG, "device port = %d", dev->port);
}

esp_err_t i2c_device_init(i2c_device_t* device) {
    ESP_LOGD(TAG, "i2c_device_init");

    esp_err_t ret = ESP_OK;

    if ( (device == NULL) | (device->port < 0) | (device->port >= I2C_NUM_MAX) ) {
        ESP_LOGE(TAG, "bad args");
        return ESP_FAIL;
    }

    return ret;
}