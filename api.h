#include <cstdint>

void getRSSI(int serial_port);

void sendMsg(uint64_t addr, char payload[]);

uint8_t * readPacket(int serial_port);