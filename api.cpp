// C library headers
#include <stdio.h>
#include <string.h>
#include <iostream>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include <chrono>
#include <math.h>
#include "api.h"

using namespace std;

#define AT_COMMAND 0x08
#define AT_COMMAND_RESP 0x88
#define TX_REQUEST 0x10
#define TX_STATUS 0x8B

int serial_port;

void initialize(char * port) {
serial_port = open(port, O_RDWR);
// Create new termios struc, we call it 'tty' for convention
struct termios tty;
memset(&tty, 0, sizeof tty);

// Read in existing settings, and handle any error
if(tcgetattr(serial_port, &tty) != 0) {
    printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
}

tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
tty.c_cflag |= CS8; // 8 bits per byte (most common)
tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

tty.c_lflag &= ~ICANON;
tty.c_lflag &= ~ECHO; // Disable echo
tty.c_lflag &= ~ECHOE; // Disable erasure
tty.c_lflag &= ~ECHONL; // Disable new-line echo
tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
// tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
// tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
tty.c_cc[VMIN] = 0;

// Set in/out baud rate to be 9600
cfsetispeed(&tty, B115200);
cfsetospeed(&tty, B115200);

// Save tty settings, also checking for error
if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
    printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
}
}


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

void getRSSI () {
    //sending AT Command
    packet* testPkt = new packet;
    unsigned char cmd[] = "DB";
    makePkt(testPkt, AT_COMMAND, cmd, 2);
    uint8_t *q = (uint8_t *)malloc(8);
    serialize(testPkt, q, 2);
    write(serial_port, q, 64);

    //receiving result
    uint8_t *data = readPacket();
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

void sendMsg (unsigned char addr[], unsigned char msg[], int msgLen) {
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
    uint8_t *p = readPacket();
    if (!verifyChecksum(p)) {
        cout << "error with transmit status checksum" << endl;
        return;
    }
    packet * txStatus = deserialize(p);
    if (txStatus->frameType != TX_STATUS) {
        cout << "wrong frame type" << endl;
        return;
    }
    // cout << "sent message, used " << int(txStatus->payload[2]) << " retries, status is " << int(txStatus->payload[3]) << endl;
    free(txPkt->payload);
    free(txStatus->payload);
    free(txPkt);
    free(txStatus);
    free(p);
    free(q);
}

void testPayload(unsigned char addr[], int length) {
    int bytesSent = 0;
    unsigned char *chunk = (unsigned char *)calloc(1, 256);
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    while(bytesSent < length) {
        sendMsg(addr, chunk, 256);
        bytesSent += 256;
        usleep(10000);
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "[s]" << std::endl;
    free(chunk);
}

void sendLargeMsg(unsigned char addr[], unsigned char msg[], int msgLen) {
    int bytesSent = 0;
    unsigned char *chunk = (unsigned char *)malloc(256);
    int len = 0;

    int n = ceil((float)msgLen / 256);
    unsigned char *packetNum = (unsigned char *)malloc(10);
    packetNum[0] = 0; packetNum[1] = 0; packetNum[2] = 0;
    memcpy((packetNum + 3), &n, 4);
    sendMsg(addr, packetNum, 10);

    while (bytesSent < msgLen) {
        if (msgLen - bytesSent>= 256) {
            memcpy(chunk, msg + bytesSent, 256);
            bytesSent+= 256;
            len = 256;
        } else {
            memcpy(chunk, msg + bytesSent, msgLen - bytesSent);
            len = msgLen - bytesSent;
            bytesSent = msgLen;
        }
        sendMsg(addr, chunk, len);
    }
}

uint8_t * readPacket () {
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

packet * waitforPacket() {
    uint8_t * p = readPacket();
    packet *rcvPkt = new packet;
    if (!verifyChecksum(p)) {
        cout << "error with packet checksum" << endl;
        return rcvPkt;
    }
    rcvPkt = deserialize(p);
    return rcvPkt;  
}

void rcvLargeMsg(int numPkts) {
    unsigned char * msg = (unsigned char *)malloc(numPkts * 256);
    int p = 0;
    for (int i = 0; i < numPkts; i++) {
        packet * currPkt = waitforPacket();
        memcpy((msg + p), (currPkt->payload + 10), (currPkt->len - 12));
        p += currPkt->len - 12;
    }
    cout << "Full msg is ";
    for (int i = 0; i < p; i++) {
        cout << char(msg[i]);
    }
    cout << endl;
}


void handleRcvPkt(packet *pkt) {
    uint8_t addr[8];
    addr[0] = pkt->frameID;
    for (int i = 0; i < 7; i++) {
        addr[i + 1] = pkt->payload[i];
    }
    if ((int)pkt->payload[10] == 0 && (int)pkt->payload[11] == 0 && (int)pkt->payload[12] == 0) {
        cout << "Start of large message" << endl;
        int numPackets;
        memcpy(&numPackets, (pkt->payload + 13), sizeof(int));
        cout << dec << numPackets << " packets to come" << endl;
        rcvLargeMsg(numPackets);
        return;
    }
    int msgLen = pkt->len - 12; //minus 2 for frameID and frameType, 7 for addr, 3 for 16bit addr and receive type
    cout << "Received dataframe from ";
    for (int i = 0; i < 8; i++) {
        cout << hex << (int)addr[i]; 
    }
    cout << " with message ";
    for (int i = 0; i < msgLen; i++) {
        cout << char(pkt->payload[i + 10]);
    }
    cout << endl;
    free(pkt->payload);
    free(pkt);
}

void handlePacket(packet * pkt) {
    switch (pkt->frameType)
    {
    case 0x90:
        cout << "Received tx message" << endl;
        handleRcvPkt(pkt);
        break;
    
    default:
    cout << "unknown packet type" << endl;
        break;
    }
}
    