#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>

int
main(int argc, char **argv) {
    int res;
    struct termios tio;

    int fd_serial = open("/dev/ttyU1", O_RDWR | O_NONBLOCK | O_NOCTTY);
    memset(&tio, 0, sizeof(struct termios));
    tio.c_iflag = INPCK | IGNPAR;
    tio.c_oflag = 0;
    tio.c_cflag = CREAD | CS8;
    tio.c_lflag = 0;
    memset(tio.c_cc, 0, sizeof(tio.c_cc));
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;

    if (argc > 1)
        tio.c_ispeed = tio.c_ospeed = strtoul(argv[1], NULL, 0);
    else
        tio.c_ispeed = tio.c_ospeed = 9600;

    res = tcsetattr(fd_serial, TCSANOW, &tio);
    printf("tcsetattr: %d, err=%s, spd=0x%08x=%u\n", res, strerror(errno), tio.c_ospeed, tio.c_ospeed);
    memset(&tio, 0, sizeof(struct termios));
    res = tcgetattr(fd_serial, &tio);
    printf("tcgetattr: %d, ispd=%u, ospd=%u\n", res, tio.c_ispeed, tio.c_ospeed);

    uint8_t values[1<<20];
    //memset(values, 0x55, sizeof(values)); // baudrate = freq * 5/10 = freq / 2
    memset(values, 0xf0, sizeof(values)); // baudrate = freq * 1/10
    while (1) {
        write(fd_serial, values, sizeof(values));
    }

    close(fd_serial);
    return 0;
}
