#pragma once
#include <3ds.h>

void uwr_init(void);
// Draws the overlay on one screen. Returns true if it wrote to the framebuffer.
bool uwr_draw(u32 screenId, u8 *fb, u32 stride, u32 format);
