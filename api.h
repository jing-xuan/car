#include <cstdint>

struct packet {
    uint8_t offset = 0x7E;
    uint16_t len;
    uint8_t frameType;
    uint8_t frameID;
    unsigned char* payload;
    uint8_t checksum;
};

extern int serial_port;

void initialize(char * port);

int verifyChecksum(uint8_t * data);

packet * deserialize (uint8_t *data);

void getRSSI();

void sendLargeMsg(unsigned char addr[], unsigned char msg[], int msgLen);

void testPayload(unsigned char addr[], int length);

void sendMsg (unsigned char addr[], unsigned char msg[], int msgLen);

uint8_t * readPacket();

packet * waitforPacket();

void handlePacket(packet * pkt);