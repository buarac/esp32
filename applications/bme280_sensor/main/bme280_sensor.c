#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_sleep.h"
#include "bme280.h"
#include "sensor.h"
#include <string.h>

#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA

static uint8_t master_mac[ESP_NOW_ETH_ALEN] = {0x24, 0x6f, 0x28, 0xa8, 0xb2, 0x1c};

static RTC_DATA_ATTR uint32_t measureId = 1;

static const char* TAG = "bme280_sensor";

bme280_t        bme;
uint8_t     measureSent;

static void example_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(ESPNOW_WIFI_MODE) );
    ESP_ERROR_CHECK( esp_wifi_start());

}

static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    ESP_LOGI(TAG, "measure sent with status = %d", status);
    measureSent = 1;
}

static esp_err_t example_espnow_init(void) {


    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(example_espnow_send_cb) );

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL) {
        ESP_LOGE(TAG, "Malloc peer information fail");
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = 1; // config
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, master_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(peer) );
    free(peer);

    return ESP_OK;
}

esp_err_t sensor_app_init(void) {
    ESP_LOGV(TAG, "bme280_sensor_app_init()");
    
    esp_err_t ret = ESP_OK;
    //uint32_t  cnt = 0;
    sensor_info_t info;
    char tmp[128];

    info.type = BME280_SENSOR;
    sensor_print_info(&info);

    ret = bme280_init_default(&bme);
    if ( ret  != ESP_OK ) {
        ESP_LOGE(TAG, "bme280 device init failed");
        bme280_done(&bme);
        return ret;
    }

    bme280_measure_t m;

    //while(1) {

        ret = bme280_read_forced(&bme, &m);
        if ( ret != ESP_OK ) {
            ESP_LOGE(TAG, "failed to read data");
            return ret;
        }
        //m.id = measureId++;
        printf("-------------------------\n");
        printf("measure id  : %07d\n", measureId++);
        printf("temperature : %7.2f C\n", m.temp);
        printf("humidity    : %7.2f\n", m.humi);
        printf("pressure    : %7.2f hPa\n", m.pres);
        printf("-------------------------\n");
        sprintf(tmp, "/%d/temperature/%.2f", measureId, m.temp);
        printf("%s\n", tmp);
        sprintf(tmp, "/%d/humidity/%.2f", measureId, m.humi);
        printf("%s\n", tmp);
        sprintf(tmp, "/%d/pressure/%.2f", measureId, m.pres);
        printf("%s\n", tmp);

        measureSent = 0;
        esp_now_send(master_mac, (void*)&m, sizeof(bme280_measure_t));
        while(measureSent==0) {
            vTaskDelay(1/portTICK_RATE_MS); 
        }

    //    vTaskDelay(5000/portTICK_RATE_MS);
    //}
    return ret;
}


void app_main(void) {


    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    example_wifi_init();
    ESP_ERROR_CHECK(example_espnow_init());
    ESP_ERROR_CHECK(sensor_app_init());
    esp_now_deinit();
    //esp_wifi_stop();
    printf("Enabling timer wakeup, %ds\n", 120);
    esp_sleep_enable_timer_wakeup(120 * 1000000);

    esp_deep_sleep_start();
}