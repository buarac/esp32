#ifndef _SENSOR_H_
#define _SENSOR_H_


typedef enum {

    BME280_SENSOR = 0x50

} sensor_type_t;

typedef struct {
    sensor_type_t   type;

    __UINT8_TYPE__ payload[0];
} sensor_info_t;

typedef struct {
    float   temp;
    float   humi;
    float   pres;
} bme280_sensor_data_t;

void sensor_print_info(sensor_info_t* info);

#endif // _SENSOR_H_