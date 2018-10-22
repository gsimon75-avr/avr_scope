#include "common.h"
#include "screen.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if __LINUX__
#   include <sys/ioctl.h>
#   include <asm/ioctls.h>
#   include <asm/termbits.h>
#elif __FreeBSD__
#   include <termios.h>
#endif // __LINUX__, __FreeBSD__
#include <SDL.h>

SDL_Thread *thr_serial;

int fd_serial = -1;
//const char init_cmd[] = "1R6S80L0T4p0ch31t";
//const char init_cmd[] = "1R6S80L0T";
const char init_cmd[] = "80L";

uint16_t voltage_factor;

int
handle_serial_data(void *userdata) {
    struct pollfd pfd[2];
    pfd[0].fd = quit_notify_fds[0];
    pfd[0].events = POLLIN;
    pfd[0].revents = 0;
    pfd[1].fd = fd_serial;
    pfd[1].events = POLLIN;
    pfd[1].revents = 0;

    fprintf(stderr, "entering thread loop\n");
    while (!do_quit) {
        uint8_t values[1024];

        poll(pfd, 2, -1);
        if (pfd[0].revents & POLLIN)
            break;
        if (pfd[1].revents & POLLIN) {
            ssize_t len = read(fd_serial, values, sizeof(values));
            if (len == 0) {
                do_quit = true;
            }
            else if (len < 0) {
                if ((errno != EINTR) && (errno != EAGAIN))
                    do_quit = true;
            }
            else {
                for (size_t i = 0; i < len; ++i)
                    add_sample(values[i]);
            }
        }
    }
    fprintf(stderr, "exited thread loop\n");
    return 1;
}

void
set_sample_rate(uint8_t n) {
    char buf[32];
    printf("sample_rate=%d\n", n);
    snprintf(buf, sizeof(buf), "%XS", n);
    usleep(1000); write(fd_serial, buf + 0, 1);
    usleep(1000); write(fd_serial, buf + 1, 1);
}

void
set_voltage_ref(uint8_t n) {
    char buf[32];
    printf("voltage_ref=%d\n", n);
    snprintf(buf, sizeof(buf), "%XR", n & 1);
    usleep(1000); write(fd_serial, buf + 0, 1);
    usleep(1000); write(fd_serial, buf + 1, 1);
    voltage_factor = voltage_factors[n];
}

void
set_zero_level(uint8_t n) {
    char buf[32];
    n &= 1;
    printf("zero_level=%s\n", n ? "AC" : "DC");
    snprintf(buf, sizeof(buf), "%XZ", n);
    usleep(1000); write(fd_serial, buf + 0, 1);
    usleep(1000); write(fd_serial, buf + 1, 1);
}

void
set_trig_level(uint8_t n) {
    char buf[32];
    printf("trig_level=0x%02x\n", n);
    snprintf(buf, sizeof(buf), "%02XL", n);
    usleep(1000); write(fd_serial, buf + 0, 1);
    usleep(1000); write(fd_serial, buf + 1, 1);
    usleep(1000); write(fd_serial, buf + 2, 1);
}

void
set_trig_type(trig_type_t type) {
    char buf[32];
    printf("trig_type=%d\n", type);
    snprintf(buf, sizeof(buf), "%XT", type & 3);
    usleep(1000); write(fd_serial, buf + 0, 1);
    usleep(1000); write(fd_serial, buf + 1, 1);
}

void
set_pwm_prescaler(uint8_t n) {
    char buf[32];
    printf("pwm_prescaler=%d\n", n);
    snprintf(buf, sizeof(buf), "%Xp", n & 7);
    usleep(1000); write(fd_serial, buf + 0, 1);
    usleep(1000); write(fd_serial, buf + 1, 1);
}

void
set_pwm_total(uint8_t n) {
    char buf[32];
    printf("pwm_total=%d\n", n);
    snprintf(buf, sizeof(buf), "%02Xt", n);
    usleep(1000); write(fd_serial, buf + 0, 1);
    usleep(1000); write(fd_serial, buf + 1, 1);
    usleep(1000); write(fd_serial, buf + 2, 1);
}

void
set_pwm_high(uint8_t n) {
    char buf[32];
    printf("pwm_high=%d\n", n);
    snprintf(buf, sizeof(buf), "%02Xh", n);
    usleep(1000); write(fd_serial, buf + 0, 1);
    usleep(1000); write(fd_serial, buf + 1, 1);
    usleep(1000); write(fd_serial, buf + 2, 1);
}


void
init_device(const char *devname) {
#if __LINUX__
    fd_serial = open("/dev/ttyUSB1", O_RDWR);
#elif __FreeBSD__
    fd_serial = open("/dev/ttyU1", O_RDWR);
#endif
    struct termios tio;
    tio.c_iflag = INPCK | IGNPAR;
    tio.c_oflag = 0;
    tio.c_cflag = CREAD | CS8 | PARENB;
    tio.c_lflag = 0;
    memset(tio.c_cc, 0, sizeof(tio.c_cc));
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;
#if __LINUX__
    ioctl(fd_serial, TCSETS, &tio);
    {
        struct termios2 tio2;
        ioctl(fd_serial, TCGETS2, &tio2);
        tio2.c_cflag &= ~CBAUD;
        tio2.c_cflag |= BOTHER;
        tio2.c_ispeed = tio2.c_ospeed = 250000;
        ioctl(fd_serial, TCSETS2, &tio2);
    }
#elif __FreeBSD__
    tio.c_ispeed = tio.c_ospeed = 250000;
    tcsetattr(fd_serial, TCSANOW, &tio);
#endif

    thr_serial = SDL_CreateThread(handle_serial_data, "handle_serial", nullptr);
    for (int i = 0; i < sizeof(init_cmd); ++i) {
      usleep(1000);
      write(fd_serial, init_cmd + i, 1);
    }
}

void
shutdown_device() {
    printf("waiting for thread\n");
    SDL_WaitThread(thr_serial, nullptr);
    close(fd_serial);
}
