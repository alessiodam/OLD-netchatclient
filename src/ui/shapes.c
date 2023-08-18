/**
 * --------------------------------------
 * 
 * CEaShell Source Code - shapes.c
 * By RoccoLox Programs and TIny_Hacker
 * Copyright 2022 - 2023
 * License: GPL-3.0
 * 
 * --------------------------------------
**/
// TKB Studios asked for permission to use this code snippet and was allowed by RoccoLox Programs.
// changes were made to this code

#include "shapes.h"

#include <graphx.h>

void shapes_RoundRectangleFill(const uint8_t color, const uint8_t radius, const int width, const uint8_t height, const int x, const uint8_t y, const bool outline_needed, const uint8_t outline_color) {
    if (outline_needed)
    {
        gfx_SetColor(outline_color);
        gfx_Circle(x + radius, y + radius, radius);
    }
    gfx_SetColor(color);
    gfx_FillCircle(x + radius, y + radius, radius);
    gfx_FillCircle(x + radius, y + height - radius - 1, radius);
    gfx_FillCircle(x + width - radius - 1, y + radius, radius);
    gfx_FillCircle(x + width - radius - 1, y + height - radius - 1, radius);
    gfx_FillRectangle(x, y + radius, width, height - radius * 2);
    gfx_FillRectangle(x + radius, y, width - radius * 2, height);
}