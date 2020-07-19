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
    MASTER_EVENT_SEND_CB = 0x00,
    MASTER_EVENT_RECV_CB = 0x80
} master_event_type_t;

typedef struct {
    master_event_type_t type;
    uint8_t             addr[ESP_NOW_ETH_ALEN];
    uint8_t             status;
    uint8_t             len;
    uint8_t*            data;
} master_event_t;

typedef struct {
    float   temp;
    float   humi;
    float   pres;
} meteo_event_t;

static const char *TAG = "espnow_master";

static xQueueHandle master_queue;

static uint8_t s_broadcast_addr[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

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

static inline uint8_t is_broadcast_addr(uint8_t* addr) {
    return (memcmp(addr, s_broadcast_addr, ESP_NOW_ETH_ALEN) == 0);
}

/* ESPNOW sending or receiving callback function is called in WiFi task.
 * Users should not do lengthy operations from this task. Instead, post
 * necessary data to a queue and handle it from a lower priority task. */
static void app_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {

    if ( mac_addr == NULL ) {
        ESP_LOGE(TAG, "send cb error: mac_addr is null");
        return;
    }

    master_event_t evt;
    evt.type = MASTER_EVENT_SEND_CB;
    evt.status = status;
    memcpy(&evt.addr, mac_addr,ESP_NOW_ETH_ALEN);

    if (xQueueSend(master_queue, &evt, portMAX_DELAY) != pdTRUE) {
        ESP_LOGW(TAG, "send cb error: send queue fail");
    }
}

static void app_espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len) {

    if (mac_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "receive cb error: bad arguments");
        return;
    }

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
    if (xQueueSend(master_queue, &evt, portMAX_DELAY) != pdTRUE) {
        ESP_LOGW(TAG, "receive cb error: send queue fail");
        free(evt.data);
    }
}

static void app_espnow_task(void *pvParameter) {

    master_event_t evt;

    while (xQueueReceive(master_queue, &evt, portMAX_DELAY) == pdTRUE) {
        printf("------------- <NEW EVENT> ---------------\n");
        if ( evt.type == MASTER_EVENT_SEND_CB ) {
            format_mac_addr(evt.addr);
            printf("event sent to [%s] with status = %d\n", tmp_mac_addr, evt.status);
        }
        else if ( evt.type == MASTER_EVENT_RECV_CB ) {
            format_mac_addr(evt.addr);
            printf("event received from [%s] with %d byte(s)\n", tmp_mac_addr, evt.len);
            ESP_LOG_BUFFER_HEXDUMP(TAG, evt.data, evt.len, ESP_LOG_DEBUG);
            if ( evt.len == sizeof(meteo_event_t)) {
                meteo_event_t* m_evt = (meteo_event_t*)evt.data;
                printf("temperature: %.2f\n", m_evt->temp);
                printf("humidity   : %.2f\n", m_evt->humi);
                printf("pressure   : %.2f\n", m_evt->pres);                
            }
            free(evt.data);
        }
    }
}

static esp_err_t app_add_pear(uint8_t* addr) {
    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL) {
        ESP_LOGE(TAG, "Malloc peer information fail");
        vSemaphoreDelete(master_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, s_broadcast_addr, ESP_NOW_ETH_ALEN);
    esp_err_t ret =  esp_now_add_peer(peer);
    free(peer);
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

    master_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(master_event_t));
    if (master_queue == NULL) {
        ESP_LOGE(TAG, "Create mutex fail");
        return ESP_FAIL;
    }

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(app_espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(app_espnow_recv_cb) );

    ESP_ERROR_CHECK( app_add_pear(s_broadcast_addr) );

    return ESP_OK;
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


    xTaskCreate(app_espnow_task, "app_espnow_task", 2048, NULL, 4, NULL);
}
