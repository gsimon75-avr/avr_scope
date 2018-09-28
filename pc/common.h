#ifndef COMMON_H
#define COMMON_H 1

#include <stdint.h> 
#include <SDL.h>

// sizes of elements:
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 256

#define TIME_SCALE_WIDTH SCREEN_WIDTH
#define TIME_SCALE_HEIGHT 11

#define VOLTAGE_SCALE_WIDTH 32
#define VOLTAGE_SCALE_HEIGHT SCREEN_HEIGHT

#define TRIG_MARKER_WIDTH 8
#define TRIG_MARKER_HEIGHT SCREEN_HEIGHT

#define DENSITY_WIDTH 1
#define DENSITY_HEIGHT SCREEN_HEIGHT

// position of elements:
// voltage scale = top left
// screen = right to voltage scale
// timescale = below screen

#define VOLTAGE_SCALE_X 0
#define VOLTAGE_SCALE_Y 0

#define TRIG_MARKER_X (VOLTAGE_SCALE_X + VOLTAGE_SCALE_WIDTH + 3)
#define TRIG_MARKER_Y VOLTAGE_SCALE_Y

#define DENSITY_X (TRIG_MARKER_X + TRIG_MARKER_WIDTH + 3)
#define DENSITY_Y TRIG_MARKER_Y

#define SCREEN_X (DENSITY_X + DENSITY_WIDTH + 3)
#define SCREEN_Y DENSITY_Y

#define TIME_SCALE_X SCREEN_X
#define TIME_SCALE_Y (SCREEN_Y + SCREEN_HEIGHT + 3)

#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 300

extern bool do_quit;
extern SDL_Renderer *renderer;
extern int quit_notify_fds[2];

extern uint32_t user_event_type_base;
#define UEVENT_DATA_READY 1

extern const uint16_t voltage_factors[];

typedef enum {
    TRIG_NONE,
    TRIG_RISING,
    TRIG_FALLING,
    TRIG_MAX
} trig_type_t;

extern trig_type_t trig_type;
extern uint8_t trig_high, trig_low;

#endif // COMMON_H

