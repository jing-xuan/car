// C library headers
#include <stdio.h>
#include <string.h>
#include <iostream>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

#include "api.h"

#define port "/dev/ttyUSB0"
// 0x0013A200419A977D;
//00 13 A2 00 41 9A 97 7D
unsigned char addrRed[] = {0x00, 0x13, 0xA2, 0x00, 0x41, 0x9A, 0x97, 0x7D};
unsigned char addrBlue[] = {0x00, 0x13, 0xA2, 0x00, 0x41, 0x9A, 0x9B, 0x5C};


using namespace std;

int main() {
int serial_port = open(port, O_RDWR);

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
cfsetispeed(&tty, B9600);
cfsetospeed(&tty, B9600);

// Save tty settings, also checking for error
if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
    printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
}

// Write to serial port
// char msg[] = {0x7E, 0x00, 0x04, 0x08, 0x01, 'D', 'B', 0x70};
// unsigned char msgDec[] = {126, 0, 4, 8, 1, 68, 66, 112};
// write(serial_port, &msg, sizeof(msg));

// getRSSI(serial_port);

sendMsg(serial_port, addrBlue, (unsigned char *) "test", 4);

close(serial_port);
}