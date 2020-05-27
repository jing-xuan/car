#include <stdio.h>
#include <cstdint>
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <bits/stdc++.h> 
#include "api.h"
#include <time.h>

using namespace std;

#define AT_COMMAND 0x08
#define AT_COMMAND_RESP 0x88
#define TX_REQUEST 0x10
#define TX_STATUS 0x8B


void serialize (packet *pkt, uint8_t *q, int payloadLen) {
    *q = pkt->offset; q++;
    *q = pkt->len >> 8; q++;
    *q = pkt->len & 0xFF; q++;
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
    pkt->payload = (unsigned char *)malloc(length - 2);
    for (int i = 0; i < length - 2; i++) {
        pkt->payload[i] = data[5 + i];
    }
    pkt->checksum = data[length + 3];
    return pkt;
}

void makePkt (packet *pkt, int frameType, unsigned char payload[], int payloadLen) {
    pkt->frameType = frameType;
    pkt->frameID = 1;
    pkt->len = payloadLen + 2;
    pkt->payload = payload;
    //calc checksum
    uint64_t sum = pkt->frameType + pkt->frameID;
    for (int i = 0; i < payloadLen; i++) {
        sum += int(payload[i]);
    }
    sum &= 0xff;
    pkt->checksum = 0xff - sum;
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
    unsigned char cmd[] = "DB";
    makePkt(testPkt, AT_COMMAND, cmd, 2);
    uint8_t *q = (uint8_t *)malloc(8);
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
    if (strncmp((char *)cmd, (char *)returnPkt->payload, 2) != 0) {
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

void sendMsg (int serial_port, unsigned char addr[], unsigned char msg[], int msgLen) {
    //sending message
    unsigned char stuff[] = {0xFF, 0xFE, 0x00, 0x00};
    packet * txPkt = new packet;
    unsigned char *payload = (unsigned char *)malloc(msgLen + 12);
    memcpy(payload, addr, 8);
    memcpy((payload + 8), stuff, 4);
    memcpy((payload + 12), msg, (msgLen));
    makePkt(txPkt, TX_REQUEST, payload, msgLen + 12);
    uint8_t *q = (uint8_t *)malloc(msgLen + 12 + 4);
    serialize(txPkt, q, msgLen + 12);
    write(serial_port, q, msgLen + 18);
    //reading transmit status
    uint8_t *p = readPacket(serial_port);
    if (!verifyChecksum(p)) {
        cout << "error with transmit status checksum" << endl;
        return;
    }
    packet * txStatus = deserialize(p);
    if (txStatus->frameType != TX_STATUS) {
        cout << "wrong frame type" << endl;
        return;
    }
    cout << "sent message, used " << int(txStatus->payload[2]) << " retries, status is " << int(txStatus->payload[3]) << endl;
    free(txPkt->payload);
    free(txStatus->payload);
    free(txPkt);
    free(txStatus);
    free(payload);
    free(p);
    free(q);
}

void testPayload(int serial_port, unsigned char addr[], uint64_t length) {
    uint64_t bytesSent = 0;
    unsigned char *chunk = (unsigned char *)calloc(1, 80);
    clock_t start = clock();
    while(bytesSent < length) {
        sendMsg(serial_port, addr, chunk, 80);
        bytesSent += 80;
        usleep(10000);
    }
    clock_t end = clock();
    double timeSpent = double(end - start) / CLOCKS_PER_SEC;
    cout << "Time spent: " << timeSpent << endl;
    free(chunk);
}

void sendLargeMsg(int serial_port, unsigned char addr[], unsigned char msg[], long msgLen) {
    int bytesSent = 0;
    unsigned char *chunk = (unsigned char *)malloc(80);
    int len = 0;
    while (bytesSent < msgLen) {
        if (msgLen - bytesSent>= 80) {
            memcpy(chunk, msg + bytesSent, 80);
            bytesSent+= 80;
            len = 80;
        } else {
            memcpy(chunk, msg + bytesSent, msgLen - bytesSent);
            len = msgLen - bytesSent;
            bytesSent = msgLen;
        }
        sendMsg(serial_port, addr, chunk, len);
    }
    free(chunk);
}

uint8_t * readPacket (int serial_port) {
    uint8_t *buf = (uint8_t *)malloc(3);;
    int len = 0;
    uint16_t length; 
    while (len < 3) {
        int n = read(serial_port, buf + len, 1);
        len += n;
    }
    length = uint16_t((buf[1] << 8)) + uint16_t(buf[2]);
    buf = (uint8_t *)realloc(buf, length + 4);
    while (len < length + 4) {
        int n = read(serial_port, buf + len, 1);
        len += n;
    }
    return buf;
}

packet * waitforPacket(int serial_port) {
    uint8_t * p = readPacket(serial_port);
    packet *rcvPkt = new packet;
    if (!verifyChecksum(p)) {
        cout << "error with packet checksum" << endl;
        return rcvPkt;
    }
    rcvPkt = deserialize(p);
    return rcvPkt;  
}
    