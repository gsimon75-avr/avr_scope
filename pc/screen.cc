#include "common.h"
#include "screen.h"
#include "font.h"

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
const char init_cmd[] = "1R3S";

static uint16_t voltage_factor;

typedef enum {
    TRIG_WAIT_PRE,
    TRIG_WAIT,
    TRIG_RUN
} trig_state_t;

trig_state_t trig_state = TRIG_RUN;

int curX = 0;
uint32_t screen[SCREEN_HEIGHT*SCREEN_WIDTH];
uint32_t* lastOfs[SCREEN_WIDTH];

SDL_Texture *txt_screen;
SDL_Rect dst_screen { SCREEN_X, SCREEN_Y, SCREEN_WIDTH, SCREEN_HEIGHT };

uint32_t density[DENSITY_HEIGHT*DENSITY_WIDTH], count_density[DENSITY_HEIGHT*DENSITY_WIDTH];
SDL_Texture *txt_density;
SDL_Rect dst_density { DENSITY_X, DENSITY_Y, DENSITY_WIDTH, DENSITY_HEIGHT };

void
add_sample(uint8_t y) {
    bool need_redraw = false;

    y = (voltage_factor * y) >> 8;
    
    ++count_density[y];
    if (count_density[y] == 0xff) {
        for (int i = 0; i < 0xff; ++i) {
            density[255 - i] = count_density[i] * 0x010101;
        }
        memset(count_density, 0, sizeof(count_density));
        need_redraw = true;
    }

    switch (trig_state) {
        case TRIG_WAIT_PRE:
        switch (trig_type) {
            case TRIG_RISING: // first wait for low
            if (y < trig_low)
                trig_state = TRIG_WAIT;
            break;

            case TRIG_FALLING: // first wait for high
            if (y >= trig_high)
                trig_state = TRIG_WAIT;
            break;

            default: // shouldn't happen, fix it
            trig_state = TRIG_RUN;
            break;
        }
        break;

        case TRIG_WAIT:
        switch (trig_type) {
            case TRIG_RISING: // now wait for high
            if (y >= trig_high)
                trig_state = TRIG_RUN;
            break;

            case TRIG_FALLING: // now wait for low
            if (y < trig_low)
                trig_state = TRIG_RUN;
            break;

            default: // shouldn't happen, fix it
            trig_state = TRIG_RUN;
            break;
        }
        break;

        case TRIG_RUN: {
            reinterpret_cast<uint8_t*>(lastOfs[curX])[1] = 0;
            lastOfs[curX] = &screen[(SCREEN_WIDTH * (unsigned int)(255 - y)) + curX];
            reinterpret_cast<uint8_t*>(lastOfs[curX])[1] = 0xff;
            ++curX;
            if (curX >= SCREEN_WIDTH) {
                curX = 0;
                trig_state = (trig_type == TRIG_NONE) ? TRIG_RUN : TRIG_WAIT_PRE;
                need_redraw = true;
            }
        }
        break;
    }
    
    if (need_redraw) {
        SDL_Event event;
        SDL_zero(event);
        event.type = user_event_type_base;
        event.user.code = UEVENT_DATA_READY;
        event.user.data1 = nullptr;
        event.user.data2 = nullptr;
        SDL_PushEvent(&event);
    }
}

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

int
dash_width(int i) {
    if ((i % 10) == 0)
        return 5;
    if ((i % 5) == 0)
        return 3;
    return 1;
}

void
draw_grid(void) {
    // draw grid
    for (int i = 0; i < SCREEN_WIDTH; i += 10) {
        int fill = dash_width(i / 10);
        for (int j = 0; j < SCREEN_HEIGHT; j += 10)
            for (int k = -fill; k <= fill; ++k) {
                int y = j + k;
                if ((0 <= y) && (y < SCREEN_HEIGHT))
                    screen[SCREEN_WIDTH*(SCREEN_HEIGHT-1-y) + i] = 0x7f0000;
            }
    }
    for (int j = 0; j < SCREEN_HEIGHT; j += 10) {
        int fill = dash_width(j / 10);
        for (int i = 0; i < SCREEN_WIDTH; i += 10)
            for (int k = -fill; k <= fill; ++k) {
                int x = i + k;
                if ((0 <= x) && (x < SCREEN_WIDTH))
                    screen[SCREEN_WIDTH*(SCREEN_HEIGHT-1-j) + i+k] = 0x7f0000;
            }
    }
}

void
init_screen(const char *devname) {
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
        tio2.c_ispeed = tio2.c_ospeed = 2000000;
        ioctl(fd_serial, TCSETS2, &tio2);
    }
#elif __FreeBSD__
    tio.c_ispeed = tio.c_ospeed = 2000000;
    tcsetattr(fd_serial, TCSANOW, &tio);
#endif

    for (int i = 0; i < SCREEN_WIDTH; ++i)
        lastOfs[i] = &screen[0];

    memset(density, 0, sizeof(density));

    txt_screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT); // 0xrrggbb
    txt_density = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, DENSITY_WIDTH, DENSITY_HEIGHT); // 0xrrggbb

    draw_grid();
    redraw_screen();

    thr_serial = SDL_CreateThread(handle_serial_data, "handle_serial", nullptr);
    for (int i = 0; i < sizeof(init_cmd); ++i) {
      usleep(1000);
      write(fd_serial, init_cmd + i, 1);
    }
}

void
set_sample_rate(uint8_t n) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%XS", n & 3);
    usleep(1000); write(fd_serial, buf + 0, 1);
    usleep(1000); write(fd_serial, buf + 1, 1);
}

void
set_voltage_ref(uint8_t n) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%XR", n & 3);
    usleep(1000); write(fd_serial, buf + 0, 1);
    usleep(1000); write(fd_serial, buf + 1, 1);
    voltage_factor = voltage_factors[n];
}

void
redraw_screen() {
    SDL_UpdateTexture(txt_screen, nullptr, &screen, 4*SCREEN_WIDTH);
    SDL_RenderCopy(renderer, txt_screen, nullptr, &dst_screen);
    SDL_UpdateTexture(txt_density, nullptr, &density, 4*DENSITY_WIDTH);
    SDL_RenderCopy(renderer, txt_density, nullptr, &dst_density);
    SDL_RenderPresent(renderer);
}

void
shutdown_screen() {
    printf("waiting for thread\n");
    SDL_WaitThread(thr_serial, nullptr);
    SDL_DestroyTexture(txt_screen);
    SDL_DestroyTexture(txt_density);
    close(fd_serial);
}
