/*
	Color utility functions for embedded systems

	Copyright 2023 <>< Charles Lohr, under the MIT-x11 or NewBSD License, you choose!
*/

#ifndef _COLOR_UTILITIES_H
#define _COLOR_UTILITIES_H

#include <stdint.h>

// To stop warnings about unused functions.
uint32_t EHSVtoHEX( uint8_t hue, uint8_t sat, uint8_t val ) __attribute__((used));
uint32_t TweenHexColors( uint32_t hexa, uint32_t hexb, int tween ) __attribute__((used));

#endif

