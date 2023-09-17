#include <stdio.h>
#include "uart_lib.h"

void encodeDataToHex( int id, int pbgt, int lent, float ec, float od, float ph, float teamp, int year ,int mon ,int mday ,int hour ,int minz ,int sec , char *hexString) {
    sprintf(hexString, "%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x", id, pbgt, lent, year, mon, mday, hour, minz, sec, *(unsigned int *)&ph,*(unsigned int *)&od, *(unsigned int *)&ec, *(unsigned int *)&teamp);
}


void decodeHexToData(const char *hexString, size_t hexStringLen, SensorData *data) {
    if (hexStringLen != 97) {
        printf("Invalid hex string length.\n");
        return;
    }
    sscanf(hexString, "%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x", &data->id, &data->pbgt, &data->lenth, &data->year, &data->mon, &data->mday, &data->hour, &data->minz, &data->sec, (unsigned int *)&data->ph,(unsigned int *)&data->od, (unsigned int *)&data->ec, (unsigned int *)&data->teamp);
}
