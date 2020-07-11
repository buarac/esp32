#include "esp_log.h"
#include "bme280.h"

static const char* TAG = "bme280_sensor";
#define BME280_I2C_PORT CONFIG_BME280_I2C_PORT
#define BME280_I2C_ADDR CONFIG_BME280_I2C_ADDR

i2c_device_t    device;

void app_main(void) {
    ESP_LOGV(TAG, "app_main on port: %d with address: %d, 0x%02x", BME280_I2C_PORT, BME280_I2C_ADDR, BME280_I2C_ADDR);
    device.addr = BME280_I2C_ADDR;
    device.port = BME280_I2C_PORT;
    bme280_device_info(&device);
}