#include "queue.h"

static const char* TAG = "Queue";

void Init(Queue* Q)
{
    Q->Front = NULL;
    Q->Rear = NULL;
}

int Isempty(Queue Q)
{
    return Q.Front == NULL;
}

static package DataNull = {
        .start_bit = 0x02,
        .device_ID = 0x00,
        .package_version = 0x01,
        .timestamp = 0,
        .data_length = 0,
        .data[0] = 0,
        .data[1] = 0,
        .data[2] = 0,
        .data[3] = 0,
        .stop_bit = 0x03
    };

void enqueue(Queue* Q, package x)
{
    //Tao mot node moi
    Node* P = (Node*)malloc(sizeof(Node));
    P->Data = x;
    P->Next = NULL;

    //Neu Queue rong
    if (Isempty(*Q))
    {
        Q->Front = P;
        Q->Rear = P;
    }
    else
    {
        Q->Rear->Next = P;
        Q->Rear = P;
    }
}

raw_package dequeue(Queue* Q) {
    if (Isempty(*Q)) {
        return DataNull;
    }

    Node* temp = Q->Front;
    package data = temp->Data;
    Q->Front = Q->Front->Next;

    if (Q->Front == NULL) {
        Q->Rear = NULL;
    }

    free(temp);
    return data;
}

void errorHandler(int response)
{
	switch(response) {

		case 0 :
			ESP_LOGE( TAG, "Queue Empty\n" );
			break;

		case 4:
			ESP_LOGE( TAG, "Device 2" );
			break;

		case 16:
			ESP_LOGE( TAG, "Device 1" );
			break;

		default :
			ESP_LOGE( TAG, "Unknown error\n" );
	}
}
