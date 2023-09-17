#include <stdio.h>
#include "driver/gpio.h"
#include "uart_lib.h"

void encodeDataToHex( int id, int pbgt,float ec, float od, float ph, float teamp, int year ,int mon ,int mday ,int hour ,int min ,int sec , char *hexString) {
    sprintf(hexString, "%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x", id, pbgt, year, mon, mday, hour, min, sec, *(unsigned int *)&ph,*(unsigned int *)&od, *(unsigned int *)&ec, *(unsigned int *)&teamp);
}


void decodeHexToData(const char *hexString, size_t hexStringLen, SensorData *data) {
    if (hexStringLen != 97) {
        printf("Invalid hex string length.\n");
        return;
    }
    sscanf(hexString, "%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x", &data->id, &data->pbgt,&data->year, &data->mon, &data->mday, &data->hour, &data->min, &data->sec, (unsigned int *)&data->ph,(unsigned int *)&data->od, (unsigned int *)&data->ec, (unsigned int *)&data->teamp);
}

void GPIO_SET_UART(){
    gpio_set_direction(M0, GPIO_MODE_OUTPUT);
    gpio_set_direction(M1, GPIO_MODE_OUTPUT);
    gpio_set_level(M0, 0);
    gpio_set_level(M1, 0);
}