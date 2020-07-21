#ifndef _ESPNOW_COMP_H
#define _ESPNOW_COMP_H
#include <stdlib.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h" 
#include "esp_now.h"

#include "esp_netif.h"
#include "esp_wifi.h"




uint8_t espnow_is_null_addr(uint8_t* addr);
uint8_t espnow_is_broadcast_addr(uint8_t* addr);

esp_err_t espnow_init(esp_now_send_cb_t send_sb, esp_now_recv_cb_t recv_cb, uint8_t* addr);
esp_err_t espnow_done();
esp_err_t espnow_add_peer(uint8_t* addr);

#endif // _ESPNOW_COMP_H