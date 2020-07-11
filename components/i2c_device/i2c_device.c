#include "esp_log.h"
#include "i2c_device.h"

static const char* TAG = "i2c_device";

void i2c_device_info(void) {
    ESP_LOGV(TAG, "information about i2c device");
}