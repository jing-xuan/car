#include <cstdint>

struct packet {
    uint8_t offset = 0x7E;
    uint16_t len;
    uint8_t frameType;
    uint8_t frameID;
    unsigned char* payload;
    uint8_t checksum;
};

int verifyChecksum(uint8_t * data);

packet * deserialize (uint8_t *data);

void getRSSI(int serial_port);

void sendLargeMsg(int serial_port, unsigned char addr[], unsigned char msg[], int msgLen);

void sendMsg (int serial_port, unsigned char addr[], unsigned char msg[], int msgLen);

uint8_t * readPacket(int serial_port);

packet * waitforPacket(int serial_port);