/**
 * --------------------------------------
 * 
 * TINET source code - TEXTUTILS.H
 * By TKB Studios
 * Copyright 2022 - 2023
 * License: GPL-3.0
 * 
 * --------------------------------------
**/
// some utils were taken from other people and will also be mentioned

#ifndef TEXTUTILS_H
#define TEXTUTILS_H

#include <stdint.h>
#include <stdbool.h>
#include <graphx.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
    Search for a space in a char buffer
    @param string: (const char) string to search the space position in.
    @param start: start at that string position
*/
unsigned int spaceSearch(const char *string, const unsigned int start);

#ifdef __cplusplus
}
#endif
#endif