#ifndef SCREEN_H
#define SCREEN_H 1

#include <stdint.h>

void init_screen(const char *devname);
void set_sample_rate(uint8_t n);
void set_voltage_ref(uint8_t n);
void set_zero_level(uint8_t n);
void set_trig_level(uint8_t n);
void set_trig_type(trig_type_t type);
void redraw_screen();
void shutdown_screen();

#endif // SCREEN_H
