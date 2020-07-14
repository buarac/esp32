#include "esp_log.h"
#include "bme280.h"

static const char* TAG = "bme280_sensor";

#define BME280_I2C_PORT CONFIG_BME280_I2C_PORT
#define BME280_I2C_ADDR CONFIG_BME280_I2C_ADDR
#define BME280_I2C_SDA  CONFIG_BME280_I2C_SDA
#define BME280_I2C_SCL  CONFIG_BME280_I2C_SCL

bme280_t        bme;
bme280_params_t bme_params;


esp_err_t bme280_sensor_app_init(void) {
    ESP_LOGV(TAG, "bme280_sensor_app_init()");
    
    esp_err_t ret = ESP_OK;

    ret = bme280_init(&bme, BME280_I2C_PORT, BME280_I2C_ADDR, BME280_I2C_SDA, BME280_I2C_SCL);
    if ( ret  != ESP_OK ) {
        ESP_LOGE(TAG, "bme280 device init failed");
        return ret;
    }
    ESP_LOGI(TAG, "bme 280 chip id = 0x%02x", bme.chip_id);

    bme280_params_default(&bme, &bme_params);

    ret = bme280_init_params(&bme, &bme_params);
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "bme280 failed to init");
        bme280_done(&bme);
        return ret;
    }

    bme280_measure_t m;

    while(1) {

        ret = bme280_read_forced(&bme, &m);
        if ( ret != ESP_OK ) {
            ESP_LOGE(TAG, "failed to read data");
            return ret;
        }

        ESP_LOGI(TAG, "temperature : % 7.2f C", m.temp);
        ESP_LOGI(TAG, "humidity    : % 7.2f ", m.humi);
        ESP_LOGI(TAG, "pressure    : % 7.2f hPa", m.pres / 100.0);

        vTaskDelay(5000/portTICK_RATE_MS);
    }
    return ret;
}

void bme280_sensor_app_done(void) {
    bme280_done(&bme);
}

void app_main(void) {

    ESP_ERROR_CHECK(bme280_sensor_app_init());
}