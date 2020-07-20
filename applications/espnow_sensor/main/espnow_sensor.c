/* ESPNOW Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
   This example shows how to use ESPNOW.
   Prepare two device, one for sending ESPNOW data and another for receiving
   ESPNOW data.
*/
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "esp_sleep.h"


/* ESPNOW can work in both station and softap mode. It is configured in menuconfig. */
#if CONFIG_ESPNOW_WIFI_MODE_STATION
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
#endif

#define ESPNOW_QUEUE_SIZE           20

typedef enum {
    SENSOR_UNDEFINED_STATE = 0,
    SENSOR_NOT_CONFIGURED ,
    SENSOR_CONFIGURED,
    SENSOR_CAPTURING_DATA,
    SENSOR_CAPTURE_DONE,
    SENSOR_SENDING_DATA,
    SENSOR_SEND_DATA_DONE

} sensor_state_t;

typedef struct {
    sensor_state_t  prev_state;
    sensor_state_t  state;
} sensor_event_t;

typedef struct {
    float   temp;
    float   humi;
    float   pres;
} sensor_info_t;

static const char *TAG = "espnow_sensor";

static xQueueHandle sensor_queue;

static RTC_DATA_ATTR uint8_t master_addr[ESP_NOW_ETH_ALEN] = { 0, 0, 0, 0, 0, 0 };
static uint8_t broadcast_addr[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint8_t null_addr[ESP_NOW_ETH_ALEN] = { 0, 0, 0, 0, 0, 0 };

static sensor_state_t state = SENSOR_UNDEFINED_STATE;
static sensor_info_t  info;

static char tmp_mac_addr[20];
static inline void format_mac_addr(uint8_t* mac_addr) {
    sprintf(tmp_mac_addr,
        "%02x:%02x:%02x:%02x:%02x:%02x",
        mac_addr[0],
        mac_addr[1],
        mac_addr[2],
        mac_addr[3],
        mac_addr[4],
        mac_addr[5]
    );
}

static inline uint8_t IS_NULL_ADDR(uint8_t* addr) {
    return (memcmp(addr, null_addr, ESP_NOW_ETH_ALEN) == 0);
}

static inline uint8_t IS_BROADCAST_ADDR(uint8_t* addr) {
    return (memcmp(addr, broadcast_addr, ESP_NOW_ETH_ALEN) == 0);
}

static void set_sensor_state(sensor_state_t new_state) {
    sensor_event_t evt;

    evt.prev_state = state;
    state = new_state;
    evt.state = new_state;
    if ( xQueueSend(sensor_queue, &evt, portMAX_DELAY) != pdTRUE ) {
        ESP_LOGW(TAG, "set_sensor_state error: send queue fail");
    }
}

/* ESPNOW sending or receiving callback function is called in WiFi task.
 * Users should not do lengthy operations from this task. Instead, post
 * necessary data to a queue and handle it from a lower priority task. */
static void app_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {

    if ( mac_addr == NULL ) {
        ESP_LOGE(TAG, "send cb error: mac_addr is null");
        return;
    }
    /*
    master_event_t evt;
    evt.type = MASTER_EVENT_SEND_CB;
    evt.status = status;
    memcpy(&evt.addr, mac_addr,ESP_NOW_ETH_ALEN);

    if (xQueueSend(sensor_queue, &evt, portMAX_DELAY) != pdTRUE) {
        ESP_LOGW(TAG, "send cb error: send queue fail");
    }
    */
    ESP_LOGI(TAG, "app_espnow_send_cb(status=%d)", status);
    ESP_LOGI(TAG, "current sensor state %d", state);
    if ( state > SENSOR_NOT_CONFIGURED ) {
        set_sensor_state(SENSOR_SEND_DATA_DONE);
    }
}

static void app_espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len) {

    if (mac_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "receive cb error: bad arguments");
        return;
    }
    /*
    master_event_t evt;
    evt.type = MASTER_EVENT_RECV_CB;
    memcpy(&evt.addr, mac_addr, ESP_NOW_ETH_ALEN);
    evt.data = malloc(len);
    if (evt.data == NULL) {
        ESP_LOGE(TAG, "receive cb error: malloc receive data fail");
        return;
    }
    memcpy(evt.data, data, len);
    evt.len = len;
    if (xQueueSend(sensor_queue, &evt, portMAX_DELAY) != pdTRUE) {
        ESP_LOGW(TAG, "receive cb error: send queue fail");
        free(evt.data);
    }
    */
    ESP_LOGI(TAG, "app_espnow_recv_cb(len=%d)", len);
    memcpy(master_addr, mac_addr, ESP_NOW_ETH_ALEN);
    set_sensor_state(SENSOR_CONFIGURED);
}



static void do_sensor_configuration() {
    if ( IS_NULL_ADDR(master_addr) ) {
        uint16_t ping = 1973;
        esp_now_send(broadcast_addr, (void*)&ping, 2);
    }
    else {
        set_sensor_state(SENSOR_CONFIGURED);
    }
}


static esp_err_t app_add_peer(uint8_t* addr) {
    /* Add broadcast peer information to peer list. */
     esp_err_t ret = ESP_OK;
    if ( esp_now_is_peer_exist(addr)  == false ) {
        esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
        if (peer == NULL) {
            ESP_LOGE(TAG, "Malloc peer information fail");
            vSemaphoreDelete(sensor_queue);
            esp_now_deinit();
            return ESP_FAIL;
        }
        memset(peer, 0, sizeof(esp_now_peer_info_t));
        peer->channel = CONFIG_ESPNOW_CHANNEL;
        peer->ifidx = ESPNOW_WIFI_IF;
        peer->encrypt = false;
        memcpy(peer->peer_addr, addr, ESP_NOW_ETH_ALEN);
        ret =  esp_now_add_peer(peer);
        free(peer);
        ESP_LOGI(TAG, "peer added");
    }
    else {
        ESP_LOGI(TAG, "peer already exists");
    }
    return ret;
}


/* WiFi should start before using ESPNOW */
static void app_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(ESPNOW_WIFI_MODE) );
    ESP_ERROR_CHECK( esp_wifi_start());
}

static esp_err_t app_espnow_init(void) {

    sensor_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(sensor_event_t));
    if (sensor_queue == NULL) {
        ESP_LOGE(TAG, "Create mutex fail");
        return ESP_FAIL;
    }

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(app_espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(app_espnow_recv_cb) );

    ESP_ERROR_CHECK( app_add_peer(broadcast_addr) );

    format_mac_addr(master_addr);
    ESP_LOGI(TAG, "master mac addr: %s", tmp_mac_addr);

    return ESP_OK;
}

static void do_capture_data() {
    info.temp = 19.73;
    info.humi = 13.89;
    info.pres = 1013.25;
    set_sensor_state(SENSOR_CAPTURE_DONE);
}

static void do_send_data() {
    ESP_LOGI(TAG, "do_send_data()");
    esp_now_send(master_addr, (void*)&info, sizeof(sensor_info_t));
}

static void do_deep_sleep() {
    ESP_LOGI(TAG, "Enabling timer wakeup, %ds\n", 30);
    esp_sleep_enable_timer_wakeup(30 * 1000000);
    esp_deep_sleep_start();
}

static void sensor_event_handler(void *pvParameter) {

    sensor_event_t evt;

    while (xQueueReceive(sensor_queue, &evt, portMAX_DELAY) == pdTRUE) {
        printf("------------- <NEW SENSOR EVENT> ---------------\n");
        printf("sensor state changed %d -> %d\n", evt.prev_state, evt.state);
        switch (evt.state) {
            case SENSOR_NOT_CONFIGURED:
                do_sensor_configuration();
                break;
            case SENSOR_CONFIGURED:
                format_mac_addr(master_addr);
                ESP_LOGI(TAG, "master addr [%s]", tmp_mac_addr);
                app_add_peer(master_addr);
                do_capture_data();
                break;
            case SENSOR_CAPTURE_DONE:
                ESP_LOGI(TAG, "capture data done");
                do_send_data();
                break;
            case SENSOR_SEND_DATA_DONE:
                ESP_LOGI(TAG, "data send done");
                do_deep_sleep();
                break;
            default:
                ESP_LOGI(TAG, "state %d not processed yet", evt.state);
        }
    }
}

void app_main(void)  {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    app_wifi_init();
    app_espnow_init();

    xTaskCreate(sensor_event_handler, "sensor_event_handler", 2048, NULL, 4, NULL);

    set_sensor_state(SENSOR_NOT_CONFIGURED);
}
