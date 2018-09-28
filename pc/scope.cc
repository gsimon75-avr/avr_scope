#include "common.h"
#include "screen.h"
#include "ui.h"

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <SDL.h>

#include <stdexcept>

bool do_quit = false;
int quit_notify_fds[2] = { -1, -1 };

SDL_Renderer *renderer;
uint32_t user_event_type_base;

const int sample_times[] = { 10, 20, 50, 100 }; // usec
int sample_rate = 3;
const int voltages[] = { 5, 20 };
const uint16_t voltage_factors[] = { 216, 250 }; // Vref / desired_unit * 256
int voltage_ref = 1;

trig_type_t trig_type = TRIG_NONE;
uint8_t trig_level = 0x80;

int
main(int argc, char **argv) {
    int i;

    i = pipe2(quit_notify_fds, O_NONBLOCK); // write to fd[1], read from fd[0]
    if (i < 0) {
        printf("pipe2 failed: %d %d (%s)\n", i, errno, strerror(errno));
        return 1;
    }

    SDL_Init(SDL_INIT_VIDEO);
    user_event_type_base = SDL_RegisterEvents(1);
    SDL_Window *window = SDL_CreateWindow("yadda", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderClear(renderer);

    init_ui();
    init_screen("/dev/ttyUSB1");
    set_sample_rate(sample_rate);
    redraw_time_scale(sample_times[sample_rate]);
    set_voltage_ref(voltage_ref);
    redraw_voltage_scale(voltages[voltage_ref]);
    set_trig_level(trig_level);
    redraw_trig_marker(trig_level, trig_type);

    while (!do_quit) {
        SDL_Event e;
        if (!SDL_WaitEvent(&e))
            continue;
        if (e.type == SDL_QUIT)
            break;

        switch (e.type) {
            case SDL_USEREVENT:
            switch (e.user.code) {
                case UEVENT_DATA_READY:
                redraw_screen();
                break;
            }
            break;

            case SDL_KEYDOWN:
            switch (e.key.keysym.sym) {
                case SDLK_ESCAPE:
                do_quit = true;
                break;

                case SDLK_LEFT:
                if (sample_rate > 0) {
                    --sample_rate;
                    set_sample_rate(sample_rate);
                    redraw_time_scale(sample_times[sample_rate]);
                }
                break;

                case SDLK_RIGHT:
                if (sample_rate < 3) {
                    ++sample_rate;
                    set_sample_rate(sample_rate);
                    redraw_time_scale(sample_times[sample_rate]);
                }
                break;

                case SDLK_v:
                voltage_ref = (voltage_ref + 1) & 1;
                set_voltage_ref(voltage_ref);
                redraw_voltage_scale(voltages[voltage_ref]);
                break;

                case SDLK_UP:
                if (trig_level < 0xff) {
                    ++trig_level;
                    set_trig_level(trig_level);
                    redraw_trig_marker(trig_level, trig_type);
                }
                break;

                case SDLK_DOWN:
                if (trig_level > 0) {
                    --trig_level;
                    set_trig_level(trig_level);
                    redraw_trig_marker(trig_level, trig_type);
                }
                break;

                case SDLK_t:
                trig_type = (trig_type_t)(1 + (int)trig_type);
                if (trig_type >= TRIG_MAX)
                    trig_type = TRIG_NONE;
                set_trig_type(trig_type);
                redraw_trig_marker(trig_level, trig_type);
            }
            break;
        }
    }
    do_quit = true;
    write(quit_notify_fds[1], "x", 1);
    shutdown_screen();
    shutdown_ui();
    printf("bye\n");

    SDL_RenderClear(renderer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    close(quit_notify_fds[0]);
    close(quit_notify_fds[1]);
    return 0;
}
