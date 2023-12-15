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

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

 /*
 Search for a space in a char buffer
 @param string: (const char) string to search the space position in.
 @param start: start at that string position
 */
 unsigned int spaceSearch(const char *string, const unsigned int start);

 /*
 Check if char a starts with char b
 @param a: (const char) string to search the string b position in.
 @param start: start at that string position.
 @returns bool value.
 */
 bool StartsWith(const char *a, const char *b);

#ifdef __cplusplus
}
#endif
#endif