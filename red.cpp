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
#include "api.h"

#include <opencv2/opencv.hpp>

#define port "/dev/ttyUSB0"
// 0x0013A200419A977D;
//00 13 A2 00 41 9A 97 7D
unsigned char addrRed[] = {0x00, 0x13, 0xA2, 0x00, 0x41, 0x9A, 0x97, 0x7D};
unsigned char addrBlue[] = {0x00, 0x13, 0xA2, 0x00, 0x41, 0x9A, 0x9B, 0x5C};


using namespace std;

int main() {
    initialize(port);
    while(1) {
        sendLargeMsg(addrBlue, (unsigned char *) "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Dapibus ultrices in iaculis nunc sed augue lacus. Quam nulla porttitor massa id neque aliquam. Ultrices mi tempus imperdiet nulla malesuada. Eros in cursus turpis massa tincidu", 300);
        cout << "Sent message" << endl;
        sleep(1);
    }
    close(serial_port);
}