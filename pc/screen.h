#ifndef SCREEN_H
#define SCREEN_H 1

#include <stdint.h>

void init_screen();
void add_sample(uint8_t y, uint8_t digital);
void end_of_sweep();
void redraw_screen();
void shutdown_screen();

#endif // SCREEN_H
