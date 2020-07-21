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

#include "espnow_comp.h"


/* ESPNOW can work in both station and softap mode. It is configured in menuconfig. */

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

uint8_t my_broadcast_macaddr[ESP_NOW_ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

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
            ESP_LOG_BUFFER_HEXDUMP(TAG, evt.data, evt.len, ESP_LOG_WARN);
            if ( evt.len == sizeof(meteo_event_t)) {
                meteo_event_t* m_evt = (meteo_event_t*)evt.data;
                printf("temperature: %.2f\n", m_evt->temp);
                printf("humidity   : %.2f\n", m_evt->humi);
                printf("pressure   : %.2f\n", m_evt->pres);                
            } 
            else if ( evt.len == 2 ) {
                uint16_t code = (uint16_t)(*evt.data | (*(evt.data+1) << 8));
                ESP_LOGI(TAG, "ping event from %s with code %d", tmp_mac_addr, code);
                if ( code == 1973 ) {
                    //ESP_LOGI(TAG, "ping event from %s", tmp_mac_addr);
                    // ping
                    code = 1389;
                    // renvoyer le ping
                    //esp_now_send ( evt.addr, (void*)&code, 2);
                    espnow_add_peer ( evt.addr );
                    esp_now_send ( my_broadcast_macaddr, (void*)&code, 2);
                }
            }
            free(evt.data);
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

    master_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(master_event_t));
    if (master_queue == NULL) {
        ESP_LOGE(TAG, "Create mutex fail");
        return;
    }

    espnow_init(app_espnow_send_cb, app_espnow_recv_cb, NULL);


    xTaskCreate(app_espnow_task, "app_espnow_task", 2048, NULL, 4, NULL);

    set_sensor_state(SENSOR_NOT_CONFIGURED);
}
