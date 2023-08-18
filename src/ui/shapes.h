/**
 * --------------------------------------
 * 
 * CEaShell Source Code - shapes.h
 * By RoccoLox Programs and TIny_Hacker
 * Copyright 2022 - 2023
 * License: GPL-3.0
 * 
 * --------------------------------------
**/
// TKB Studios asked for permission to use this code snippet and was allowed by RoccoLox Programs.

#ifndef SHAPES_H
#define SHAPES_H

#include <stdint.h>
#include <stdbool.h>
#include <graphx.h>

#ifdef __cplusplus
extern "C" {
#endif

void shapes_RoundRectangleFill(const uint8_t color, const uint8_t radius, const int width, const uint8_t height, const int x, const uint8_t y);

#ifdef __cplusplus
}
#endif
#endif