#include "esp_log.h"
#include "bme280.h"
#include <string.h>

static const char *TAG = "bme280";

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

    // check if bme280 device
    bme280_chip_id_get(bme);
    if (bme->chip_id != BME280_CHIP_ID)
    {
        ESP_LOGE(TAG, "Not a bme280 device. expected chip id = %d, found %d", BME280_CHIP_ID, bme->chip_id);
        return ESP_ERR_INVALID_ARG;
    }

    ret = bme280_soft_reset(bme);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "soft reset failed");
        return ret;
    }
    vTaskDelay(1/portTICK_RATE_MS);

    // load calibrate registers
    ret = bme280_cal_data_get(bme);
    ret = bme280_cal_humi_data_get(bme);

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

esp_err_t bme280_cal_data_get(bme280_t *bme)
{
    ESP_LOGV(TAG, "bme280_cal_data_get");

    esp_err_t ret = ESP_OK;
    i2c_device_t *dev = &bme->device;
    // read data
    uint8_t reg = 0x88;
    uint8_t val[24];

    i2c_device_read(dev, &reg, 1, val, 24);

    bme->dig_t1 = (uint16_t)(val[1] << 8 | val[0]);
    bme->dig_t2 = (int16_t)(val[3] << 8 | val[2]);
    bme->dig_t3 = (int16_t)(val[5] << 8 | val[4]);
    bme->dig_p1 = (uint16_t)(val[7] << 8 | val[6]);
    bme->dig_p2 = (int16_t)(val[9] << 8 | val[8]);
    bme->dig_p3 = (int16_t)(val[11] << 8 | val[10]);
    bme->dig_p4 = (int16_t)(val[13] << 8 | val[12]);
    bme->dig_p5 = (int16_t)(val[15] << 8 | val[14]);
    bme->dig_p6 = (int16_t)(val[17] << 8 | val[16]);
    bme->dig_p7 = (int16_t)(val[19] << 8 | val[18]);
    bme->dig_p8 = (int16_t)(val[21] << 8 | val[20]);
    bme->dig_p9 = (int16_t)(val[23] << 8 | val[22]);

    /*
    ESP_LOGD(TAG, "calibration data output");
    ESP_LOGD(TAG, "dig_t1 = %d, %04x", bme->dig_t1, bme->dig_t1);
    ESP_LOGD(TAG, "dig_t2 = %d, %04x", bme->dig_t2, bme->dig_t2);
    ESP_LOGD(TAG, "dig_t3 = %d, %04x", bme->dig_t3, bme->dig_t3);
    ESP_LOGD(TAG, "dig_p1 = %d, %04x", bme->dig_p1, bme->dig_p1);
    ESP_LOGD(TAG, "dig_p2 = %d, %04x", bme->dig_p2, bme->dig_p2);
    ESP_LOGD(TAG, "dig_p3 = %d, %04x", bme->dig_p3, bme->dig_p3);
    ESP_LOGD(TAG, "dig_p4 = %d, %04x", bme->dig_p4, bme->dig_p4);
    ESP_LOGD(TAG, "dig_p5 = %d, %04x", bme->dig_p5, bme->dig_p5);
    ESP_LOGD(TAG, "dig_p6 = %d, %04x", bme->dig_p6, bme->dig_p6);
    ESP_LOGD(TAG, "dig_p7 = %d, %04x", bme->dig_p7, bme->dig_p7);
    ESP_LOGD(TAG, "dig_p8 = %d, %04x", bme->dig_p8, bme->dig_p8);
    ESP_LOGD(TAG, "dig_p9 = %d, %04x", bme->dig_p9, bme->dig_p9);
    */
    return ret;
}

esp_err_t bme280_cal_humi_data_get(bme280_t *bme)
{
    ESP_LOGV(TAG, "bme280_cal_humi_data_get");

    esp_err_t ret = ESP_OK;
    i2c_device_t *dev = &bme->device;
    uint8_t raw[7];
    uint8_t reg = 0xe1;

    ret = i2c_device_read_reg_uint8(dev, 0xa1, &bme->dig_h1);
    ret = i2c_device_read(dev, &reg, 1, raw, 7);

    bme->dig_h2 = (int16_t)(raw[1] << 8 | raw[0]);
    bme->dig_h3 = (uint8_t)(raw[2]);
    bme->dig_h4 = (int16_t)(raw[3] << 4 | (raw[4] & 0x0f));
    bme->dig_h5 = (int16_t)(((raw[4] & 0xf0) >> 4) | raw[5] << 4 );
    bme->dig_h6 = (int8_t)(raw[6]);

    ESP_LOGD(TAG, "humidity calibration data output");
    ESP_LOGD(TAG, "dig_h1 = %d, %04x", bme->dig_h1, bme->dig_h1);
    ESP_LOGD(TAG, "dig_h2 = %d, %04x", bme->dig_h2, bme->dig_h2);
    ESP_LOGD(TAG, "dig_h3 = %d, %04x", bme->dig_h3, bme->dig_h3);
    ESP_LOGD(TAG, "dig_h4 = %d, %04x", bme->dig_h4, bme->dig_h4);
    ESP_LOGD(TAG, "dig_h5 = %d, %04x", bme->dig_h5, bme->dig_h5);
    ESP_LOGD(TAG, "dig_h6 = %d, %04x", bme->dig_h6, bme->dig_h6);

    return ret;
}

static inline int32_t bme280_compensate_temperature(bme280_t *bme, int32_t temp, int32_t *fine_temp)
{
    ESP_LOGV(TAG, "bme280_compensate_temperature");
    int32_t var1, var2;
    int32_t comp_temp;

    var1 = ((temp >> 3) - (bme->dig_t1 << 1));
    var1 *= (bme->dig_t2 >> 11);

    var2 = ((temp >> 4) - bme->dig_t1);
    var2 *= (((temp >> 4) - bme->dig_t1));
    var2 = var2 >> 12;
    var2 *= (bme->dig_t3);
    var2 = var2 >> 14;

    *fine_temp = var1 + var2;
    comp_temp = (*fine_temp * 5 + 128) >> 8;

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

    ret = bme280_read_raw_forced(bme, &raw_data);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to read raw data");
        return ret;
    }

    comp_temp = bme280_compensate_temperature(bme, raw_data.temp, &fine_temp);
    m.temp = (float)comp_temp / 100.0f;

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

    // wait mode back to sleep_mode
    uint8_t measure_finished = 0;
    while (measure_finished == 0)
    {
        ret = i2c_device_read(&bme->device, &reg, 1, &ctrl.data, 1);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "failed waiting device back to sleep mode");
            return ret;
        }
        if (ctrl.bits.mode == BME280_SLEEP_MODE)
        {
            measure_finished = 1;
        }
        vTaskDelay(1 / portTICK_RATE_MS);
    }
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
