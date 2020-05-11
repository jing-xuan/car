#include <stdio.h>
#include <cstdint>
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <bits/stdc++.h> 
#include "api.h"

using namespace std;

#define AT_COMMAND 0x08

struct packet {
    uint8_t offset = 0x7E;
    uint16_t len;
    uint8_t frameType;
    uint8_t frameID;
    char* payload;
    uint8_t checksum;
};

void serialize(packet *pkt, uint8_t *q, int payloadLen) {
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

void makePkt( packet *pkt, int frameType, char payload[], int payloadLen) {
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

void getRSSI(int serial_port) {
    packet* testPkt = new packet;
    char cmd[] = "DB";
    makePkt(testPkt, AT_COMMAND, cmd, 2);
    int length = 2;
    uint8_t *q = (uint8_t *)malloc(8*8);
    serialize(testPkt, q, 2);
    write(serial_port, q, 64);
    readPacket(serial_port);
    free(q);
    free(testPkt);
}

void sendMsg(uint64_t addr, char payload[]) {
    //printf("sending message to %c", addr);
}

void readPacket(int serial_port) {
    uint8_t *data = (uint8_t *)malloc(24);
    read(serial_port, data, 24);
    uint16_t length = uint16_t((data[1] << 8)) + uint16_t(data[2]);
    cout << length << endl;
    data = (uint8_t *)realloc(data, 3+ length * 8);
    data += 3;
    read(serial_port, data, 8 * (length + 1));
    data -= 3;
    for (int i = 0; i < length + 3; i++) {
        cout << hex << int(data[i]) << endl;
    }
}