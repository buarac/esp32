#include "esp_log.h"
#include "bme280.h"
#include <string.h>

static const char *TAG = "bme280";

static inline esp_err_t bme280_wait_measure_done(bme280_t* bme) {
    ESP_LOGD(TAG, "waiting for measure done");

    uint8_t measure_done = 0;
    esp_err_t ret;
    bme280_status_t     status;
    bme280_ctrl_temp_t  ctrl;
    uint8_t data[2];
    uint8_t reg = BME280_REG_STATUS;

    while ( !measure_done ) {

        ret = i2c_device_read (&bme->device, &reg, 1, data, 2);
        if ( ret != ESP_OK ) {
            ESP_LOGE(TAG, "failed to read status & ctrl_temp registers");
            return ret;
        }
        status.data = data[0];
        ctrl.data = data[1];

        ESP_LOGD(TAG, "mode = %d", ctrl.bits.mode);
        ESP_LOGD(TAG, "measuring = %d", status.bits.measuring);

        if ( ctrl.bits.mode == BME280_SLEEP_MODE && status.bits.measuring == 0 ) {
            measure_done = 1;
        }
    }
    ESP_LOGD(TAG, "measure done");
    return ESP_OK;
}

static inline esp_err_t bme280_wait_nvm_copied(bme280_t* bme) {
    ESP_LOGD(TAG, "waiting for nvm data copied");
    uint8_t nvm_copied = 0;
    esp_err_t ret;
    uint16_t attempts = 0;

    while ( !nvm_copied ) {
        ret = i2c_device_read_reg_uint8 (&bme->device, BME280_REG_STATUS, &bme->status.data);
        if ( ret != ESP_OK ) {
            ESP_LOGE(TAG, "failed to read status register");
            return ret;
        }    
        if ( bme->status.bits.im_update == 0 ) {
            nvm_copied = 1;
        }
        ++attempts;
    }
    ESP_LOGD(TAG, "nvm data copied (%d attempt(s))", attempts);
    return ESP_OK;
}

esp_err_t bme280_init_default(bme280_t* bme) {
    ESP_LOGV(TAG, "bme280_init()");

    esp_err_t ret = bme280_init(bme, BME280_I2C_PORT, BME280_I2C_ADDR, BME280_I2C_SDA, BME280_I2C_SCL);
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "init failed");
        return ret;
    }

    ESP_LOGI(TAG, "bme 280 chip id = 0x%02x", bme->chip_id);

    bme280_params_t params;
    bme280_params_default(bme, &params);

    ret = bme280_init_params(bme, &params);
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "bme280 failed to set params");
        return ret;
    }

    return ret;
}

esp_err_t bme280_init(bme280_t *bme, i2c_port_t port, uint8_t addr, uint8_t sda, uint8_t scl)
{
    ESP_LOGV(TAG, "bme280_init()");

    esp_err_t ret = ESP_OK;

    if (bme == NULL)
    {
        ESP_LOGE(TAG, "bme280 object is null");
        return ESP_FAIL;
    }

    memset(bme, 0, sizeof(bme280_t));
    ret = i2c_device_init(&bme->device, port, addr, sda, scl);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "i2c device init failed");
        return ret;
    }


    ret = bme280_soft_reset(bme);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "soft reset failed");
        return ret;
    }

    bme280_wait_nvm_copied(bme);

    // check if bme280 device
    bme280_chip_id_get(bme);
    if (bme->chip_id != BME280_CHIP_ID)
    {
        ESP_LOGE(TAG, "Not a bme280 device. expected chip id = %d, found %d", BME280_CHIP_ID, bme->chip_id);
        return ESP_ERR_INVALID_ARG;
    }

    // load calibrate registers
    ret = bme280_read_calib_data(bme);

    return ret;
}

esp_err_t bme280_done(bme280_t *bme)
{
    ESP_LOGV(TAG, "bme280_done()");

    return ESP_OK;
}

void bme280_params_default(bme280_t *bme, bme280_params_t *params)
{
    ESP_LOGV(TAG, "bme280_params_default");

    bme280_params_t p;

    p.filter = BME280_FILTER_OFF;
    p.mode = BME280_SLEEP_MODE;
    p.over_samp_temp = BME280_OVERSAMPLING_1;
    p.over_samp_humi = BME280_OVERSAMPLING_1;
    p.over_samp_pres = BME280_OVERSAMPLING_1;
    p.standby = BME280_STANDBY_1000;

    memset(params, 0, sizeof(bme280_params_t));
    memcpy(params, &p, sizeof(bme280_params_t));
}

esp_err_t bme280_init_params(bme280_t *bme, bme280_params_t *params)
{
    ESP_LOGV(TAG, "bme280_init_params");

    if (bme == NULL || params == NULL)
    {
        ESP_LOGE(TAG, "bme or params null");
        return ESP_ERR_INVALID_ARG;
    }

    bme280_config_t config;
    bme280_ctrl_temp_t ctrl_temp;
    bme280_ctrl_humi_t ctrl_humi;
    u_int8_t reg;
    esp_err_t ret;

    config.bits.standby = params->standby;
    config.bits.filter = params->filter;
    ctrl_humi.bits.over_samp_humi = params->over_samp_humi;
    ctrl_temp.bits.over_samp_temp = params->over_samp_temp;
    ctrl_temp.bits.over_samp_pres = params->over_samp_pres;
    ctrl_temp.bits.mode = params->mode;

    // config
    reg = BME280_REG_CONFIG;
    ret = i2c_device_write(&bme->device, &reg, 1, &config.data, 1);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to write (%d) to config register", config.data);
        return ret;
    }
    // ctrl humi
    reg = BME280_REG_CTRL_HUMI;
    ret = i2c_device_write(&bme->device, &reg, 1, &ctrl_humi.data, 1);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to write (%d) to ctrl humi register", ctrl_humi.data);
        return ret;
    }

    // ctrl temp
    reg = BME280_REG_CTRL_TEMP;
    ret = i2c_device_write(&bme->device, &reg, 1, &ctrl_temp.data, 1);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to write (%d) to ctrl temp register", ctrl_temp.data);
        return ret;
    }

    return ret;
}

esp_err_t bme280_status_get(bme280_t* bme) {
    ESP_LOGV(TAG, "bme280_status_get");

    return i2c_device_read_reg_uint8(&bme->device, BME280_REG_STATUS, &bme->status.data);
}

esp_err_t bme280_chip_id_get(bme280_t *bme) {
    ESP_LOGV(TAG, "bme280_chip_id_get()");

    return i2c_device_read_reg_uint8(&bme->device, BME280_REG_CHIP_ID, &bme->chip_id);
}

esp_err_t bme280_soft_reset(bme280_t *bme)
{
    ESP_LOGV(TAG, "bme280_soft_reset");

    uint8_t reg = BME280_REG_RESET;
    uint8_t data = BME280_RESET_VALUE;
    return i2c_device_write(&bme->device, &reg, 1, &data, 1);
}

esp_err_t bme280_read_calib_data(bme280_t *bme)
{
    ESP_LOGV(TAG, "bme280_read_calib_data");

    esp_err_t ret = ESP_OK;
    i2c_device_t *dev = &bme->device;
    bme280_calib_data_t calib;
    // read data
    uint8_t reg = 0x88;
    uint8_t val[24];

    ret = i2c_device_read(dev, &reg, 1, val, 24);
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "failed to read calib data");
        return ret;
    }
    // temperature & pressure
    calib.dig_t1 = (uint16_t)(val[1] << 8 | val[0]);
    calib.dig_t2 = (int16_t)(val[3] << 8 | val[2]);
    calib.dig_t3 = (int16_t)(val[5] << 8 | val[4]);
    calib.dig_p1 = (uint16_t)(val[7] << 8 | val[6]);
    calib.dig_p2 = (int16_t)(val[9] << 8 | val[8]);
    calib.dig_p3 = (int16_t)(val[11] << 8 | val[10]);
    calib.dig_p4 = (int16_t)(val[13] << 8 | val[12]);
    calib.dig_p5 = (int16_t)(val[15] << 8 | val[14]);
    calib.dig_p6 = (int16_t)(val[17] << 8 | val[16]);
    calib.dig_p7 = (int16_t)(val[19] << 8 | val[18]);
    calib.dig_p8 = (int16_t)(val[21] << 8 | val[20]);
    calib.dig_p9 = (int16_t)(val[23] << 8 | val[22]);
    // humidity
    ret = i2c_device_read_reg_uint8(dev, 0xa1, &calib.dig_h1);
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "failed to read calib data");
        return ret;
    }
    reg = 0xe1;
    memset(val, 0, 7);
    ret = i2c_device_read(dev, &reg, 1, val, 7);
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "failed to read calib data");
        return ret;
    }
    calib.dig_h2 = (int16_t)(val[1] << 8 | val[0]);
    calib.dig_h3 = (uint8_t)(val[2]);
    calib.dig_h4 = (int16_t)(val[3] << 4 | (val[4] & 0x0f));
    calib.dig_h5 = (int16_t)(((val[4] & 0xf0) >> 4) | val[5] << 4 );
    calib.dig_h6 = (int8_t)(val[6]);

    memcpy(&bme->calib, &calib, sizeof(bme280_calib_data_t));

    return ret;
}

static inline uint32_t bme280_compensate_humidity(bme280_t* bme, int32_t humi, int32_t fine_temp) {
    ESP_LOGV(TAG, "bme280_compensate_humidity");
    
    int32_t v_x1_u32r;
    
    v_x1_u32r = (fine_temp - ((int32_t)76800));

    v_x1_u32r = (((((humi << 14) - (((int32_t)bme->calib.dig_h4) << 20) - (((int32_t)bme->calib.dig_h5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)bme->calib.dig_h6)) >> 10) * (((v_x1_u32r * ((int32_t)bme->calib.dig_h3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)bme->calib.dig_h2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)bme->calib.dig_h1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    v_x1_u32r = ( v_x1_u32r >> 12 );

    ESP_LOGD(TAG, "compensate humidity output");
    ESP_LOGD(TAG, "fine_temp = %d", fine_temp);
    ESP_LOGD(TAG, "raw humi  = %d", humi);
    ESP_LOGD(TAG, "comp humi = %d", v_x1_u32r);

    return v_x1_u32r;

}

static inline uint32_t bme280_compensate_pressure(bme280_t* bme, int32_t pres, int32_t fine_temp) {
    ESP_LOGV(TAG, "bme280_compensate_pressure");

    int64_t var1, var2, p;

    var1 = ((int64_t)fine_temp) - 128000;
    var2 = var1 * var1 * (int64_t)bme->calib.dig_p6;
    var2 = var2 + ((var1*(int64_t)bme->calib.dig_p5)<<17);
    var2 = var2 + (((int64_t)bme->calib.dig_p4)<<35);
    var1 = ((var1 * var1 * (int64_t)bme->calib.dig_p3)>>8);
    var1 = (((((int64_t)1)<<47)+var1))*((int64_t)bme->calib.dig_p1)>>33; 
    if (var1 == 0) {
        p = 0;
    }
    else {
        p = 1048576-pres;
        p = (((p<<31)-var2)*3125)/var1;
        var1 = (((int64_t)bme->calib.dig_p9) * (p>>13) * (p>>13)) >> 25; 
        var2 = (((int64_t)bme->calib.dig_p8) * p) >> 19;
        p = ((p + var1 + var2) >> 8) + (((int64_t)bme->calib.dig_p7)<<4);
    }

    ESP_LOGD(TAG, "compensate pressure output");
    ESP_LOGD(TAG, "fine_temp = %d", fine_temp);
    ESP_LOGD(TAG, "raw pres  = %d", pres);
    ESP_LOGD(TAG, "comp pres = %llu", p);

    return p;
}

static inline int32_t bme280_compensate_temperature(bme280_t *bme, int32_t temp, int32_t *fine_temp)
{
    ESP_LOGV(TAG, "bme280_compensate_temperature");
    int32_t var1, var2;
    int32_t comp_temp;

    var1 = ((temp >> 3) - (bme->calib.dig_t1 << 1));
    var1 *= (bme->calib.dig_t2 >> 11);

    var2 = ((temp >> 4) - bme->calib.dig_t1);
    var2 = (var2 * var2) >> 12;
    var2*= (bme->calib.dig_t3 >> 14);

    *fine_temp = var1 + var2;
    comp_temp = (*fine_temp * 5 + 128) >> 8;

    ESP_LOGD(TAG, "compensate temperature output");
    ESP_LOGD(TAG, "var1 = %d", var1);
    ESP_LOGD(TAG, "var2 = %d", var2);
    ESP_LOGD(TAG, "fine_temp = %d", *fine_temp);
    ESP_LOGD(TAG, "raw temp  = %d", temp);
    ESP_LOGD(TAG, "comp_temp = %d", comp_temp);


    return comp_temp;
}

esp_err_t bme280_read_forced(bme280_t *bme, bme280_measure_t *measure)
{
    ESP_LOGV(TAG, "bme280_read_forced");

    esp_err_t ret = ESP_OK;
    bme280_raw_data_t raw_data;
    bme280_measure_t m;
    int32_t fine_temp;
    int32_t comp_temp;
    uint32_t comp_humi;
    uint32_t comp_pres;

    ret = bme280_read_raw_forced(bme, &raw_data);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to read raw data");
        return ret;
    }

    comp_temp = bme280_compensate_temperature(bme, raw_data.temp, &fine_temp);
    comp_humi = bme280_compensate_humidity(bme, raw_data.humi, fine_temp);
    comp_pres = bme280_compensate_pressure(bme, raw_data.pres, fine_temp);
    m.temp = (float)comp_temp / 100.0f;
    m.humi = (float)comp_humi / 1024.0f;
    m.pres = (float)comp_pres / 256.0f / 100.0f;

    memset(measure, 0, sizeof(bme280_measure_t));
    memcpy(measure, &m, sizeof(bme280_measure_t));

    return ret;
}

esp_err_t bme280_read_raw_forced(bme280_t *bme, bme280_raw_data_t *raw_data)
{
    ESP_LOGV(TAG, "bme280_read_raw_forced");

    bme280_ctrl_temp_t ctrl;
    uint8_t reg;
    esp_err_t ret = ESP_OK;

    reg = BME280_REG_CTRL_TEMP;
    ret = i2c_device_read(&bme->device, &reg, 1, &ctrl.data, 1);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to read ctrl temp reg");
        return ret;
    }
    ctrl.bits.mode = BME280_FORCED_MODE;
    ret = i2c_device_write(&bme->device, &reg, 1, &ctrl.data, 1);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to write (%d) to ctrl temp reg", ctrl.data);
        return ret;
    }

    ret = bme280_wait_measure_done (bme);

    return bme280_read_raw(bme, raw_data);
}

esp_err_t bme280_read_raw(bme280_t *bme, bme280_raw_data_t *raw_data)
{
    ESP_LOGV(TAG, "bme280_read_raw");

    esp_err_t ret = ESP_OK;

    uint8_t reg = BME280_REG_RAW_DATA;
    uint8_t val[8];

    ret = i2c_device_read(&bme->device, &reg, 1, val, 8);
    if (ret == ESP_OK)
    {
        memset(raw_data, 0, sizeof(bme280_raw_data_t));
        raw_data->pres = (uint32_t)(val[0] << 12 | val[1] << 4 | val[2] >> 4);
        raw_data->temp = (uint32_t)(val[3] << 12 | val[4] << 4 | val[5] >> 4);
        raw_data->humi = (uint32_t)(val[6] << 8 | val[7]);

        ESP_LOGD(TAG, "raw data output");
        ESP_LOGD(TAG, "raw temp = %d, %04x", raw_data->temp, raw_data->temp);
        ESP_LOGD(TAG, "raw pres = %d, %04x", raw_data->pres, raw_data->pres);
        ESP_LOGD(TAG, "raw humi = %d, %04x", raw_data->humi, raw_data->humi);
    }
    return ret;
}
