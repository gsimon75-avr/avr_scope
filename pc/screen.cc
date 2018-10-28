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
uint32_t screen[SCREEN_HEIGHT*SCREEN_WIDTH];
uint32_t* lastOfs[SCREEN_WIDTH];

SDL_Texture *txt_screen;
SDL_Rect dst_screen { SCREEN_X, SCREEN_Y, SCREEN_WIDTH, SCREEN_HEIGHT };

uint32_t density[DENSITY_HEIGHT*DENSITY_WIDTH], count_density[DENSITY_HEIGHT*DENSITY_WIDTH];
SDL_Texture *txt_density;
SDL_Rect dst_density { DENSITY_X, DENSITY_Y, DENSITY_WIDTH, DENSITY_HEIGHT };

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
    bool need_redraw = false;
    bool end_of_sweep = (y == 0xff) || (digital == 0xff);

    y = (voltage_factor * y) >> 8;
    
    ++count_density[y];
    if (count_density[y] == 0xff) {
        for (int i = 0; i < 0xff; ++i) {
            density[255 - i] = count_density[i] * 0x010101;
        }
        memset(count_density, 0, sizeof(count_density));
        need_redraw = true;
    }

    if (curX < SCREEN_WIDTH) {
        reinterpret_cast<uint8_t*>(lastOfs[curX])[1] = 0;
        reinterpret_cast<uint8_t*>(lastOfs[curX])[0] = 0;
        lastOfs[curX] = &screen[(SCREEN_WIDTH * (unsigned int)(255 - y)) + curX];
        //reinterpret_cast<uint8_t*>(lastOfs[curX])[(digital & 0x08) ? 1 : 0] = 0xff;
        reinterpret_cast<uint8_t*>(lastOfs[curX])[1] = 0xff;
        if (digital & 0x08)
            reinterpret_cast<uint8_t*>(lastOfs[curX])[0] = 0xff;
        ++curX;
        if (curX == SCREEN_WIDTH) {
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
init_screen() {
    for (int i = 0; i < SCREEN_WIDTH; ++i)
        lastOfs[i] = &screen[0];

    memset(density, 0, sizeof(density));

    txt_screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT); // 0xrrggbb
    txt_density = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, DENSITY_WIDTH, DENSITY_HEIGHT); // 0xrrggbb

    draw_grid();
    redraw_screen();
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
    SDL_DestroyTexture(txt_screen);
    SDL_DestroyTexture(txt_density);
}
