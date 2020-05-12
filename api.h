#include <cstdint>

void getRSSI(int serial_port);

void sendMsg (int serial_port, unsigned char addr[], unsigned char msg[], int msgLen);

uint8_t * readPacket(int serial_port);