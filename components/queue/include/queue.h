/*

	DHT22 temperature sensor driver

*/
#pragma once
#ifndef QUEUE_H_
#define QUEUE_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"

typedef struct MQTTData {
    uint8_t start_bit;
    uint8_t device_ID;
    uint8_t package_version;
    uint32_t timestamp;
    uint8_t data_length;
    float data[4];
    uint8_t stop_bit;
} mqtt_package;

typedef struct loraRawData {
    uint8_t data_raw[128];
    int data_len;
} raw_package;

typedef struct Node
{
    raw_package Data;
    struct Node* Next;
} Node;

typedef struct Queue
{
    struct Node* Front, * Rear;
} Queue_;


void Init(Queue_* Q);
int Isempty(Queue_ Q);
void enqueue(Queue_* Q, raw_package x);
raw_package dequeue(Queue_* Q);
void errorHandler(int response);

#endif
