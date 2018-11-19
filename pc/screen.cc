#include "common.h"
#include "screen.h"
#include "device.h"
#include "font.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <SDL.h>

int curX = 0;
uint32_t analog_screen[ANALOG_SCREEN_HEIGHT*ANALOG_SCREEN_WIDTH];
SDL_Texture *txt_analog_screen;
SDL_Rect dst_analog_screen { ANALOG_SCREEN_X, ANALOG_SCREEN_Y, ANALOG_SCREEN_WIDTH, ANALOG_SCREEN_HEIGHT };
uint32_t* lastOfs[ANALOG_SCREEN_WIDTH];

uint32_t density[DENSITY_HEIGHT*DENSITY_WIDTH], count_density[DENSITY_HEIGHT*DENSITY_WIDTH];
SDL_Texture *txt_density;
SDL_Rect dst_density { DENSITY_X, DENSITY_Y, DENSITY_WIDTH, DENSITY_HEIGHT };

uint32_t digital_screen[DIGITAL_SCREEN_HEIGHT*DIGITAL_SCREEN_WIDTH];
SDL_Texture *txt_digital_screen;
SDL_Rect dst_digital_screen { DIGITAL_SCREEN_X, DIGITAL_SCREEN_Y, DIGITAL_SCREEN_WIDTH, DIGITAL_SCREEN_HEIGHT };

void
request_redraw() {
    SDL_Event event;
    SDL_zero(event);
    event.type = user_event_type_base;
    event.user.code = UEVENT_DATA_READY;
    event.user.data1 = nullptr;
    event.user.data2 = nullptr;
    SDL_PushEvent(&event);
}

void
end_of_sweep() {
    if (curX != 0) {
        request_redraw();
        curX = 0;
    }
}

void
add_sample(uint8_t y, uint8_t digital) {
    static const uint8_t digital_mask[] = { 0x80, 0x40, 0x20, 0x10 }; // masks for digital lines to display (add 0x08 for to show the pwm output as well)
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

    if (curX < ANALOG_SCREEN_WIDTH) {
        reinterpret_cast<uint8_t*>(lastOfs[curX])[0] = 0;
        reinterpret_cast<uint8_t*>(lastOfs[curX])[1] = 0;
        lastOfs[curX] = &analog_screen[(ANALOG_SCREEN_WIDTH * (unsigned int)(255 - y)) + curX];
        if (digital & 0x08) {
            reinterpret_cast<uint8_t*>(lastOfs[curX])[0] = 0xff;
            reinterpret_cast<uint8_t*>(lastOfs[curX])[1] = 0x7f;
        }
        else {
            reinterpret_cast<uint8_t*>(lastOfs[curX])[1] = 0xff;
        }


        for (int j = 0; j < sizeof(digital_mask); ++j) {
            int y = j * 3;
            digital_screen[DIGITAL_SCREEN_WIDTH*y + curX] &= 0xff0000;
            if (digital & digital_mask[j])
                digital_screen[DIGITAL_SCREEN_WIDTH*y + curX] |= 0xff00;
            else
                digital_screen[DIGITAL_SCREEN_WIDTH*y + curX] |= 0x3f00;
        }

        ++curX;
        if (curX == ANALOG_SCREEN_WIDTH) {
            need_redraw = true;
            curX = 0;
        }
    }
    
    if (need_redraw)
        request_redraw();
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
    for (int i = 0; i < ANALOG_SCREEN_WIDTH; i += 10) {
        int fill = dash_width(i / 10);
        for (int j = 0; j < ANALOG_SCREEN_HEIGHT; j += 10)
            for (int k = -fill; k <= fill; ++k) {
                int y = j + k;
                if ((0 <= y) && (y < ANALOG_SCREEN_HEIGHT))
                    analog_screen[ANALOG_SCREEN_WIDTH*(ANALOG_SCREEN_HEIGHT-1-y) + i] = 0x7f0000;
            }
        for (int j = 0; j < DIGITAL_SCREEN_HEIGHT; j += 2)
            digital_screen[DIGITAL_SCREEN_WIDTH*(DIGITAL_SCREEN_HEIGHT-1-j) + i] = 0x7f0000;
    }
    for (int j = 0; j < ANALOG_SCREEN_HEIGHT; j += 10) {
        int fill = dash_width(j / 10);
        for (int i = 0; i < ANALOG_SCREEN_WIDTH; i += 10)
            for (int k = -fill; k <= fill; ++k) {
                int x = i + k;
                if ((0 <= x) && (x < ANALOG_SCREEN_WIDTH))
                    analog_screen[ANALOG_SCREEN_WIDTH*(ANALOG_SCREEN_HEIGHT-1-j) + i+k] = 0x7f0000;
            }
    }
}

void
init_screen() {
    for (int i = 0; i < ANALOG_SCREEN_WIDTH; ++i)
        lastOfs[i] = &analog_screen[0];

    memset(density, 0, sizeof(density));

    txt_analog_screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, ANALOG_SCREEN_WIDTH, ANALOG_SCREEN_HEIGHT); // 0xrrggbb
    txt_digital_screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, DIGITAL_SCREEN_WIDTH, DIGITAL_SCREEN_HEIGHT); // 0xrrggbb
    txt_density = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, DENSITY_WIDTH, DENSITY_HEIGHT); // 0xrrggbb

    draw_grid();
    redraw_screen();
}

void
redraw_screen() {
    SDL_UpdateTexture(txt_analog_screen, nullptr, &analog_screen, 4*ANALOG_SCREEN_WIDTH);
    SDL_RenderCopy(renderer, txt_analog_screen, nullptr, &dst_analog_screen);
    SDL_UpdateTexture(txt_digital_screen, nullptr, &digital_screen, 4*DIGITAL_SCREEN_WIDTH);
    SDL_RenderCopy(renderer, txt_digital_screen, nullptr, &dst_digital_screen);
    SDL_UpdateTexture(txt_density, nullptr, &density, 4*DENSITY_WIDTH);
    SDL_RenderCopy(renderer, txt_density, nullptr, &dst_density);
    SDL_RenderPresent(renderer);
}

void
shutdown_screen() {
    SDL_DestroyTexture(txt_analog_screen);
    SDL_DestroyTexture(txt_digital_screen);
    SDL_DestroyTexture(txt_density);
}
