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
#include <getopt.h>
#include <SDL.h>

#include <stdexcept>

bool do_quit = false;
int quit_notify_fds[2] = { -1, -1 };

SDL_Renderer *renderer;
uint32_t user_event_type_base;

const int sample_times[] = { 1, 2, 5, 10, 20, 50, 100 }; // usec
const int voltages[] = { 5, 20 };
const uint16_t voltage_factors[] = { 216, 250 }; // Vref / desired_unit * 256

op_mode_t op_mode = OP_SCOPE;

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

void
help(const char *myname) {
    printf("Usage: %s [options]\n", myname);
    printf("  -h | --help\n");
    printf("  -m | --mode n             Operation Mode, 0=scope, 1=zero-crossing detector\n");
    printf("  -r | --rate n             Sample Rate, 0..7\n");
    printf("  -v | --vref n             Voltage Reference, 1 unit is: 0=5mV, 1=20mV\n");
    printf("  -z | --zero-level n       Zero Level, 0=DC, 1=AC\n");
    printf("  -t | --trig n             Trig Type, 0=off, 1=rising, 2=falling\n");
    printf("  -l | --trig-level n       Trig Level, 0..255\n");
    printf("  -s | --pwm-prescaler n    PWM Prescaler, 0..7\n");
    printf("  -P | --pwm-total n        PWM Total clks, 0..255\n");
    printf("  -p | --pwm-high n         PWM High clks, 0..255\n");
}


int
main(int argc, char **argv) {
    int i;

    static struct option longopts[] = {
        { "help",           no_argument,            NULL,           'h' },
        { "mode",           required_argument,      NULL,           'm' },
        { "rate",           required_argument,      NULL,           'r' },
        { "vref",           required_argument,      NULL,           'v' },
        { "zero-level",     required_argument,      NULL,           'z' },
        { "trig",           required_argument,      NULL,           't' },
        { "trig-level",     required_argument,      NULL,           'l' },
        { "pwm-prescaler",  required_argument,      NULL,           's' },
        { "pwm-total",      required_argument,      NULL,           'P' },
        { "pwm-high",       required_argument,      NULL,           'p' },
        { NULL,             0,                      NULL,           0 }
    };

    while ((i = getopt_long(argc, argv, "hm:r:v:z:t:l:s:P:p:", longopts, NULL)) != -1) {
        switch (i) {
            case 'h': help(argv[0]); return 0;
            case 'm': op_mode = (op_mode_t)strtol(optarg, NULL, 0); break;
            case 'r': sample_rate = strtol(optarg, NULL, 0); break;
            case 'v': voltage_ref = strtol(optarg, NULL, 0); break;
            case 'z': zero_level = strtol(optarg, NULL, 0); break;
            case 't': trig_type = (trig_type_t)strtol(optarg, NULL, 0); break;
            case 'l': trig_level = strtol(optarg, NULL, 0); break;
            case 's': pwm_prescaler = strtol(optarg, NULL, 0); break;
            case 'P': pwm_total = strtol(optarg, NULL, 0); break;
            case 'p': pwm_high = strtol(optarg, NULL, 0); break;
            default: printf("Unknown option '%c'\n", i); return 1;
        }
    }
    argc -= optind;
    argv += optind;

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

    set_op_mode(op_mode);
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
                        double duty = ((double)pwm_high) / pwm_total;
                        ++pwm_total;
                        pwm_high = (int)(0.5 + duty * pwm_total);
                        set_pwm_total(pwm_total);
                        set_pwm_high(pwm_high);
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
                        double duty = ((double)pwm_high) / pwm_total;
                        --pwm_total;
                        pwm_high = (int)(0.5 + duty * pwm_total);
                        set_pwm_total(pwm_total);
                        set_pwm_high(pwm_high);
                        dump_pwm_freq();
                    }
                }
                break;

                case SDLK_m:
                op_mode = (op_mode_t)(1 + (int)op_mode);
                if (op_mode >= OP_MAX)
                    op_mode = (op_mode_t)0;
                set_op_mode(op_mode);
                break;

                case SDLK_t:
                trig_type = (trig_type_t)(1 + (int)trig_type);
                if (trig_type >= TRIG_MAX)
                    trig_type = (trig_type_t)0;
                set_trig_type(trig_type);
                redraw_trig_marker(trig_level, trig_type);
                break;

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
