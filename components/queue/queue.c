#include "queue.h"

static const char* TAG = "Queue";

void Init(Queue_* Q)
{
    Q->Front = NULL;
    Q->Rear = NULL;
}

int Isempty(Queue_ Q)
{
    return Q.Front == NULL;
}

static raw_package DataNull = {
        .data_len=0
    };

void enqueue(Queue_* Q, raw_package x)
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

raw_package dequeue(Queue_* Q) {
    if (Isempty(*Q)) {
        return DataNull;
    }

    Node* temp = Q->Front;
    raw_package data = temp->Data;
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
