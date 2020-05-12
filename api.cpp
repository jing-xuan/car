#include <stdio.h>
#include <cstdint>
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <bits/stdc++.h> 
#include "api.h"

using namespace std;

#define AT_COMMAND 0x08
#define AT_COMMAND_RESP 0x88

struct packet {
    uint8_t offset = 0x7E;
    uint16_t len;
    uint8_t frameType;
    uint8_t frameID;
    char* payload;
    uint8_t checksum;
};

void serialize (packet *pkt, uint8_t *q, int payloadLen) {
    *q = pkt->offset; q++;
    *q = pkt->len >> 8; q++;
    *q = pkt->len & 0x0F; q++;
    *q = pkt->frameType; q++;
    *q = pkt->frameID; q++;
    for (int i = 0; i < payloadLen; i++) {
        *q = pkt->payload[i];
        q++;
    }
    *q = pkt->checksum;
}

packet * deserialize (uint8_t *data) {
    packet* pkt = new packet;
    uint16_t length = uint16_t((data[1] << 8)) + uint16_t(data[2]);
    pkt->len = length;
    pkt->frameType = data[3];
    pkt->frameID = data[4];
    pkt->payload = (char *)malloc(8 * (length - 2));
    for (int i = 0; i < length - 2; i++) {
        pkt->payload[i] = data[5 + i];
    }
    pkt->checksum = data[length + 3];
    return pkt;
}

void makePkt (packet *pkt, int frameType, char payload[], int payloadLen) {
    pkt->frameType = frameType;
    pkt->frameID = 1;
    pkt->len = payloadLen + 2;
    pkt->payload = (char *)malloc(payloadLen * 8);
    strcpy(pkt->payload, payload);
    //calc checksum
    int sum = pkt->frameType + pkt->frameID;
    for (int i = 0; i < payloadLen; i++) {
        sum += int(payload[i]);
    }
    sum &= 0x0F;
    pkt->checksum = 127 - sum;
}

int verifyChecksum(uint8_t * data) {
    uint16_t pktlen = uint16_t((data[1] << 8)) + uint16_t(data[2]);
    uint16_t sum = 0;
    for (int i = 0; i < pktlen + 1; i++) {
        sum += int(data[3 + i]);
    }
    sum &= 0xff;
    sum ^= 0xff;
    return (sum == 0);
}

void getRSSI (int serial_port) {
    //sending AT Command
    packet* testPkt = new packet;
    char cmd[] = "DB";
    makePkt(testPkt, AT_COMMAND, cmd, 2);
    uint8_t *q = (uint8_t *)malloc(8*8);
    serialize(testPkt, q, 2);
    write(serial_port, q, 64);

    //receiving result
    uint8_t *data = readPacket(serial_port);
    if (!verifyChecksum(data)) {
        cout << "Error in checksum" << endl;
        return;
    }
    packet* returnPkt = deserialize(data);
    if (returnPkt->frameType != AT_COMMAND_RESP) {
        cout << "Error in frame type" << endl;
        return;
    }
    if (strncmp(cmd, returnPkt->payload, 2) != 0) {
        cout << "Error in frame command" << endl;
        return;
    }
    if (returnPkt->payload[2] != 0) {
        cout << "Status error" << endl;
        return;
    }
    cout << "RSSI for last packet: " << int(returnPkt->payload[3]) << endl;
    free(q);
    free(testPkt);
    free(returnPkt);
    free(data);
}

void sendMsg (uint64_t addr, char payload[]) {
    //printf("sending message to %c", addr);
}

uint8_t * readPacket (int serial_port) {
    uint8_t *data = (uint8_t *)malloc(24);
    read(serial_port, data, 24);
    uint16_t length = uint16_t((data[1] << 8)) + uint16_t(data[2]);
    data = (uint8_t *)realloc(data, 3+ length * 8);
    data += 3;
    read(serial_port, data, 8 * (length + 1));
    data -= 3;
    return data;
}
    