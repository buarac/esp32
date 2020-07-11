#include "esp_log.h"
#include "bme280.h"

static const char* TAG = "bme280_sensor";

#define BME280_I2C_PORT CONFIG_BME280_I2C_PORT
#define BME280_I2C_ADDR CONFIG_BME280_I2C_ADDR
#define BME280_I2C_SDA  CONFIG_BME280_I2C_SDA
#define BME280_I2C_SCL  CONFIG_BME280_I2C_SCL

bme280_t bme;

void app_main(void) {
    ESP_LOGV(TAG, "app_main on port: %d with address: 0x%02x", BME280_I2C_PORT, BME280_I2C_ADDR);
    ESP_LOGV(TAG, "sda pin = %d, scl pin = %d", BME280_I2C_SDA, BME280_I2C_SCL);

    ESP_ERROR_CHECK(bme280_device_init(&bme, BME280_I2C_ADDR, BME280_I2C_PORT));
    bme280_device_info(bme.device);
}