#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define max_size 1000

typedef struct lora_packet {
	uint8_t start_bit;
	uint8_t device_id;
	uint8_t package_version;
	uint32_t timestamp;
	uint8_t data_length;
	uint8_t data[16];
	uint8_t stop_bit;
} loraPacket;

typedef struct LoraQueue {
	loraPacket *Packet;
	uint16_t front;
	uint16_t rear;
	uint16_t count; // so goi tin trong queue
} Queue;

Queue Queue_init(loraPacket* pac_ket) {
	Queue* queue = malloc(max_size * sizeof(Queue));
	queue->front = queue->rear = -1;
	queue->count = 0;
	queue->Packet = pac_ket;

	return *queue;
}

void enqueue(Queue *queue, loraPacket *pac_ket) {
	if (queue->count == (sizeof(queue->Packet) / sizeof(queue->Packet[0]))) {
		return;
	}

	queue->Packet[queue->rear] = *pac_ket;
	queue->rear = (queue->rear + 1) % (sizeof(queue->Packet) / sizeof(queue->Packet[0]));
	queue->count++;
}


