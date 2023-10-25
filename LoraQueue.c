#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "LoraQueue.h"

int main() {

	loraPacket* packet = {
	.start_bit = '\x03',
	.device_id = '\x01',
	.package_version = '\x01',
	.timestamp = 412345454,
	.data_length = 16,
	};

	Queue_init(1000, packet);
	return 0;
}