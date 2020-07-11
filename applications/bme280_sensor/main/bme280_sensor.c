#include "esp_log.h"
#include "i2c_device.h"

static const char* TAG = "bme280_sensor";


void app_main(void) {
    ESP_LOGV(TAG, "app_main");
    i2c_device_info();
}