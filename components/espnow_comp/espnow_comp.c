#include "espnow_comp.h"

uint8_t NULL_MAC_ADDR[ESP_NOW_ETH_ALEN] = { 0, 0, 0, 0, 0, 0 };
uint8_t BROADCAST_MAC_ADDR[ESP_NOW_ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

static const char *TAG = "espnow_comp";


inline uint8_t espnow_is_null_addr(uint8_t* addr) {
    return (memcmp(addr, NULL_MAC_ADDR, ESP_NOW_ETH_ALEN) == 0);
}

inline uint8_t espnow_is_broadcast_addr(uint8_t* addr) {
    return (memcmp(addr, BROADCAST_MAC_ADDR, ESP_NOW_ETH_ALEN) == 0);   
}

esp_err_t espnow_init(esp_now_send_cb_t send_cb, esp_now_recv_cb_t recv_cb, uint8_t* addr) {
    ESP_LOGV(TAG, "espnow_init");

    if ( send_cb == NULL && recv_cb == NULL ) {
        ESP_LOGE(TAG, "both cb function are nulls");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start());

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    if ( send_cb != NULL ) ESP_ERROR_CHECK( esp_now_register_send_cb(send_cb) );
    if ( recv_cb != NULL ) ESP_ERROR_CHECK( esp_now_register_recv_cb(recv_cb) );

    ESP_ERROR_CHECK( espnow_add_peer(BROADCAST_MAC_ADDR) );
    if ( addr != NULL ) ESP_ERROR_CHECK( espnow_add_peer(addr));
    return ESP_OK;
}

esp_err_t espnow_done() {
    ESP_LOGV(TAG, "espnow_done");
    return ESP_OK;
}

esp_err_t espnow_add_peer(uint8_t* addr) {
    ESP_LOGV(TAG, "espnow_add_peer");

    esp_err_t ret = ESP_OK;
    if ( esp_now_is_peer_exist(addr)  == false ) {
        esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
        if (peer == NULL) {
            ESP_LOGE(TAG, "malloc peer information fail");
            return ESP_FAIL;
        }
        memset(peer, 0, sizeof(esp_now_peer_info_t));
        peer->channel = CONFIG_ESPNOW_CHANNEL;
        peer->ifidx = ESP_IF_WIFI_STA;
        peer->encrypt = false;
        memcpy(peer->peer_addr, addr, ESP_NOW_ETH_ALEN);
        ret =  esp_now_add_peer(peer);
        free(peer);
        ESP_LOGD(TAG, "peer added");
    }
    else {
        ESP_LOGD(TAG, "peer already added");
    }
    return ret;
}

