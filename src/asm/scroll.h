#ifndef SCROLL_H
#define SCROLL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void scrollUp(unsigned int x, uint8_t y, unsigned int width, unsigned int height, unsigned int amount);

#ifdef __cplusplus
}
#endif

#endif