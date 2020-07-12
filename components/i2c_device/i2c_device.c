#include "esp_log.h"
#include "i2c_device.h"
#include <string.h>

static const char* TAG = "i2c_device";

esp_err_t i2c_device_init(i2c_device_t* device, i2c_port_t port, uint8_t addr, uint8_t sda, uint8_t scl) {
    ESP_LOGV(TAG, "i2c_device_init(%d, %02x, %d, %d)", port, addr, sda, scl);

    if ( (device == NULL) || (port < 0) || (port >= I2C_NUM_MAX) ) {
        ESP_LOGE(TAG, "bad args");
        return ESP_ERR_INVALID_ARG;
    }

    memset(device, 0, sizeof(i2c_device_t));
    device->addr = addr;
    device->port = port;
    device->conf.mode = I2C_MODE_MASTER;
    device->conf.sda_io_num = sda;
    device->conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    device->conf.scl_io_num = scl;
    device->conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    device->conf.master.clk_speed = 100000;
    i2c_param_config(device->port, &device->conf);
    return i2c_driver_install(device->port, device->conf.mode,0, 0, 0);
}

esp_err_t i2c_device_read(i2c_device_t* device, void* out_data, size_t out_size, void* in_data, size_t in_size) {
    ESP_LOGV(TAG, "i2c_device_read");

    if ( (device==NULL) || (in_data==NULL) || (in_size <= 0) ) {
        ESP_LOGE(TAG, "i2c device read invalid args");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ESP_OK;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    if ( (out_data != NULL ) && ( out_size > 0 ) ) {
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, ( device->addr << 1 ) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        i2c_master_write(cmd, out_data, out_size, ACK_CHECK_EN);
    }
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ( device->addr << 1 ) | I2C_MASTER_READ, ACK_CHECK_EN);
    i2c_master_read(cmd, in_data, in_size, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(device->port, cmd, 1000/portTICK_RATE_MS);
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "Fail to read decice ( port = %d, addr = %02x )", device->port, device->addr);
    }
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t i2c_device_write(i2c_device_t* device, void* reg_data, size_t reg_size, void* out_data, size_t out_size) {
    ESP_LOGV(TAG, "i2c_device_write");
    
    if ( (device==NULL) || (out_data==NULL) || (out_size <= 0) ) {
        ESP_LOGE(TAG, "i2c device write invalid args");
        return ESP_ERR_INVALID_ARG;
    }    

    esp_err_t ret = ESP_OK;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ( device->addr << 1 ) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    if ( reg_data != NULL && reg_size > 0 ) {
        i2c_master_write(cmd, reg_data, reg_size, ACK_CHECK_EN);
    }
    i2c_master_write(cmd, out_data, out_size, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(device->port, cmd, 1000/portTICK_RATE_MS);
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "Fail to write decice ( port = %d, addr = %02x )", device->port, device->addr);
    }
    i2c_cmd_link_delete(cmd);    

    return ret;
}