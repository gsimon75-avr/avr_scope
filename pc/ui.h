#ifndef UI_H
#define UI_H 1

#include <stdint.h>
#include <SDL.h>

void init_ui();
void redraw_time_scale(int n);
void redraw_voltage_scale(int n);
void redraw_trig_marker(uint8_t n, trig_type_t type);
void shutdown_ui();

#endif // UI_H
