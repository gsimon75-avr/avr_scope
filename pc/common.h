#ifndef COMMON_H
#define COMMON_H 1

#include <stdint.h> 
#include <SDL.h>

// sizes of elements:
#define ANALOG_SCREEN_WIDTH 800
#define ANALOG_SCREEN_HEIGHT 256

#define DIGITAL_SCREEN_WIDTH ANALOG_SCREEN_WIDTH
#define DIGITAL_SCREEN_HEIGHT 16

#define TIME_SCALE_WIDTH ANALOG_SCREEN_WIDTH
#define TIME_SCALE_HEIGHT 11

#define VOLTAGE_SCALE_WIDTH 32
#define VOLTAGE_SCALE_HEIGHT ANALOG_SCREEN_HEIGHT

#define TRIG_MARKER_WIDTH 8
#define TRIG_MARKER_HEIGHT ANALOG_SCREEN_HEIGHT

#define DENSITY_WIDTH 1
#define DENSITY_HEIGHT ANALOG_SCREEN_HEIGHT

// position of elements:
// voltage scale = top left
// analog screen = right to voltage scale
// digital screen = below analog screen
// timescale = below digital screen

#define VOLTAGE_SCALE_X 0
#define VOLTAGE_SCALE_Y 0

#define TRIG_MARKER_X (VOLTAGE_SCALE_X + VOLTAGE_SCALE_WIDTH + 3)
#define TRIG_MARKER_Y VOLTAGE_SCALE_Y

#define DENSITY_X (TRIG_MARKER_X + TRIG_MARKER_WIDTH + 3)
#define DENSITY_Y TRIG_MARKER_Y

#define ANALOG_SCREEN_X (DENSITY_X + DENSITY_WIDTH + 3)
#define ANALOG_SCREEN_Y DENSITY_Y

#define DIGITAL_SCREEN_X ANALOG_SCREEN_X
#define DIGITAL_SCREEN_Y (ANALOG_SCREEN_Y + ANALOG_SCREEN_HEIGHT + 3)

#define TIME_SCALE_X DIGITAL_SCREEN_X
#define TIME_SCALE_Y (DIGITAL_SCREEN_Y + DIGITAL_SCREEN_HEIGHT + 3)

#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 300

extern bool do_quit;
extern SDL_Renderer *renderer;
extern int quit_notify_fds[2];

extern uint32_t user_event_type_base;
#define UEVENT_DATA_READY 1

extern const uint16_t voltage_factors[];

typedef enum {
    OP_SCOPE,
    OP_ZCD,
    OP_FULLSPEED_SCOPE,
    OP_MAX
} op_mode_t;

typedef enum {
    TRIG_NONE,
    TRIG_RISING_ANALOG,
    TRIG_FALLING_ANALOG,
    TRIG_RISING_DIGITAL,
    TRIG_FALLING_DIGITAL,
    TRIG_MAX
} trig_type_t;

extern trig_type_t trig_type;
extern uint8_t trig_high, trig_low;

#endif // COMMON_H

