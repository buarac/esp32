#include "esp_log.h"
#include "bme280.h"

static const char* TAG = "bme280";

void bme280_device_info(void) {
    ESP_LOGV(TAG, "information about bme280 device");
    i2c_device_info();
}