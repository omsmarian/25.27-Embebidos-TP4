#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#define BAUD_RATE 1200
#define SERIAL_PORT "/dev/ttyS0"

#define HEADER 0xAA55C33C
#define SEND_DATA_CMD 0x07
#define SEND_DATA_OK 0x81
#define SEND_DATA_FAIL 0xC1
#define KEEP_ALIVE_CMD 0x02
#define KEEP_ALIVE_OK 0x82

typedef struct {
	uint16_t floor1;
	uint16_t floor2;
	uint16_t floor3;
} Data;

bool send_data(Data data);
void send_response(uint8_t response);
bool receive_command(uint8_t *command, Data *data);
void keep_alive();

//int main() {
//	uint8_t command;
//	Data data;
//
//	while (1) {
//		if (receive_command(&command, &data)) {
//			if (command == SEND_DATA_CMD) {
//				if (send_data(data)) {
//					send_response(SEND_DATA_OK);
//				} else {
//					send_response(SEND_DATA_FAIL);
//				}
//			} else if (command == KEEP_ALIVE_CMD) {
//				keep_alive();
//			}
//		}
//		sleep(1); // Delay to avoid flooding ThingSpeak
//	}
//
//	return 0;
//}

bool send_data(Data data) {
	// Simulate sending data to ThingSpeak
	printf("Sending data to ThingSpeak: Floor1=%d, Floor2=%d, Floor3=%d\n", data.floor1, data.floor2, data.floor3);
	sleep(15); // Simulate ThingSpeak delay
	return true; // Simulate success
}

void send_response(uint8_t response) {
	uint32_t header = HEADER;
	uint8_t length = 1;
	uint8_t buffer[6];

	memcpy(buffer, &header, 4);
	buffer[4] = length;
	buffer[5] = response;

	// Simulate sending response over serial port
	printf("Sending response: 0x%X\n", response);
}

bool receive_command(uint8_t *command, Data *data) {
	// Simulate receiving command over serial port
	// For demonstration purposes, we will simulate a SendData command
	*command = SEND_DATA_CMD;
	data->floor1 = 10;
	data->floor2 = 20;
	data->floor3 = 30;
	return true;
}

void keep_alive() {
	send_response(KEEP_ALIVE_OK);
}
