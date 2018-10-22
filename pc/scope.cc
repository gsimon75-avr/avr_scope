#include "common.h"
#include "screen.h"
#include "device.h"
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

const int sample_times[] = { 1, 2, 5, 10, 20, 50, 100 }; // usec
const int voltages[] = { 5, 20 };
const uint16_t voltage_factors[] = { 216, 250 }; // Vref / desired_unit * 256

int sample_rate = 3;
int voltage_ref = 1;
int zero_level = 1; // 0:DC, 1:AC
trig_type_t trig_type = TRIG_FALLING;
uint8_t trig_level = 0x40;

uint8_t pwm_prescaler = 6;
uint8_t pwm_total = 0xff;
uint8_t pwm_high = 0xfe;


static void
dump_pwm_freq() {
    float freq = 16e6;

    switch (pwm_prescaler) {
        case 0: break; // timer stopped
        case 1: freq /=    1; break;
        case 2: freq /=    8; break;
        case 3: freq /=   32; break;
        case 4: freq /=   64; break;
        case 5: freq /=  128; break;
        case 6: freq /=  256; break;
        case 7: freq /= 1024; break;
    }
    freq /= pwm_total;
    printf("pwm=%.1fHz %.1f%%\n", freq, (100.0 * pwm_high / pwm_total));
}


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
    init_screen();
    init_device("/dev/ttyUSB1");

    set_sample_rate(sample_rate);
    set_voltage_ref(voltage_ref);
    set_zero_level(zero_level);
    set_trig_type(trig_type);
    set_trig_level(trig_level);
    set_pwm_prescaler(pwm_prescaler);
    set_pwm_total(pwm_total);
    set_pwm_high(pwm_high);

    redraw_time_scale(sample_times[sample_rate]);
    redraw_voltage_scale(voltages[voltage_ref]);
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
                if (sample_rate < (sizeof(sample_times) / sizeof(sample_times[0]) - 1)) {
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

                case SDLK_z:
                zero_level = (zero_level + 1) & 1;
                set_zero_level(zero_level);
                //redraw_voltage_scale(voltages[voltage_ref]);
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

                case SDLK_PAGEUP:
                if (e.key.keysym.mod & KMOD_CTRL) {
                    if (pwm_prescaler < 7) {
                        ++pwm_prescaler;
                        set_pwm_prescaler(pwm_prescaler);
                        dump_pwm_freq();
                    }
                }
                else if (e.key.keysym.mod & KMOD_SHIFT) {
                    if (pwm_high < (pwm_total - 1)) {
                        ++pwm_high;
                        set_pwm_high(pwm_high);
                        dump_pwm_freq();
                    }
                }
                else {
                    if (pwm_total < 0xff) {
                        ++pwm_total;
                        set_pwm_total(pwm_total);
                        dump_pwm_freq();
                    }
                }
                break;

                case SDLK_PAGEDOWN:
                if (e.key.keysym.mod & KMOD_CTRL) {
                    if (pwm_prescaler > 0) {
                        --pwm_prescaler;
                        set_pwm_prescaler(pwm_prescaler);
                        dump_pwm_freq();
                    }
                }
                else if (e.key.keysym.mod & KMOD_SHIFT) {
                    if (pwm_high > 1) {
                        --pwm_high;
                        set_pwm_high(pwm_high);
                        dump_pwm_freq();
                    }
                }
                else {
                    if (pwm_total > 2) {
                        --pwm_total;
                        if (pwm_high >= pwm_total) {
                            pwm_high = pwm_total - 1;
                            set_pwm_high(pwm_high);
                        }
                        set_pwm_total(pwm_total);
                        dump_pwm_freq();
                    }
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
    shutdown_device();
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
