#ifndef _LIGHT_USBHOST_MIDI_H
#define _LIGHT_USBHOST_MIDI_H

#include <light_usb_midi.h>
#include <light_usbhost.h>

//   _HAVE_TINYUSB means "the TinyUSB headers are reachable and its API may be called". Consumers
// gate their whole USB implementation on it -- crossfire does, throughout -- so a platform
// missing from this test does not fail to build: it builds with no USB in it at all, silently.
// That was the state of every CMSIS build until the stack was made available there.
#if(LIGHT_SYSTEM == SYSTEM_PICO_SDK && LIGHT_PLATFORM == PLATFORM_TARGET)
#define _HAVE_TINYUSB
// the Pico SDK ships TinyUSB's BSP, which is where board_init() comes from
#include "bsp/board.h"
#include "tusb.h"
#elif(LIGHT_SYSTEM == SYSTEM_CMSIS && LIGHT_PLATFORM == PLATFORM_TARGET)
#define _HAVE_TINYUSB
//   NO bsp/board.h: bare CMSIS has no BSP and that header does not exist. Its one service here,
// board_init(), is replaced by light_usb_init() -- which is what a consumer should call on
// either platform anyway, and now does.
#include "tusb.h"
#endif

extern void light_usbhost_midi_init();

#endif