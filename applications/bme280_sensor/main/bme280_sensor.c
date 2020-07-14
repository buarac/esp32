#include "esp_log.h"
#include "bme280.h"

static const char* TAG = "bme280_sensor";


bme280_t        bme;


esp_err_t bme280_sensor_app_init(void) {
    ESP_LOGV(TAG, "bme280_sensor_app_init()");
    
    esp_err_t ret = ESP_OK;
    uint32_t  cnt = 0;

    ret = bme280_init_default(&bme);
    if ( ret  != ESP_OK ) {
        ESP_LOGE(TAG, "bme280 device init failed");
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
        ++cnt;
        printf("-------------------------\n");
        printf("measure     : %07d\n", cnt);
        printf("temperature : %7.2f C\n", m.temp);
        printf("humidity    : %7.2f\n", m.humi);
        printf("pressure    : %7.2f hPa\n", m.pres);

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