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

    ESP_LOGI(TAG, "bme280 default params output");
    ESP_LOGI(TAG, "mode = %d", bme_params.mode);
    ESP_LOGI(TAG, "filter = %d", bme_params.filter);
    ESP_LOGI(TAG, "standby = %d", bme_params.standby);
    ESP_LOGI(TAG, "temp oversampling = %d", bme_params.over_samp_temp);
    ESP_LOGI(TAG, "humi oversampling = %d", bme_params.over_samp_humi);
    ESP_LOGI(TAG, "pres oversampling = %d", bme_params.over_samp_pres);

    ret = bme280_init_params(&bme, &bme_params);
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "bme280 failed to init");
        bme280_done(&bme);
        return ret;
    }

    while(1) {

        bme280_raw_data_t raw;

        ret = bme280_read_raw_forced(&bme, &raw);
        if ( ret != ESP_OK ) {
            ESP_LOGE(TAG, "failed to read raw data");
            return ret;
        }

        int32_t fine_temp;
        int32_t comp_temp = bme280_compensate_temperature(&bme, raw.temp, &fine_temp);
        ESP_LOGI(TAG, "temperature output");
        ESP_LOGI(TAG, "raw temperature: %d", raw.temp);
        ESP_LOGI(TAG, "fine temperature: %d", fine_temp);
        ESP_LOGI(TAG, "comp temperature: %d", comp_temp);
        ESP_LOGI(TAG, "temperature: %.2f", (float)(comp_temp/100.0f));

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