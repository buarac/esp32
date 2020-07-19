#include "sensor.h"
#include "esp_log.h"


static const char *TAG = "sensor";

void sensor_print_info(sensor_info_t* info) {
    ESP_LOGI(TAG, "sensor id = 0");
    ESP_LOGI(TAG, "sensor type = %d", info->type);
    ESP_LOGI(TAG, "sensor len = %d", sizeof(sensor_info_t));
}