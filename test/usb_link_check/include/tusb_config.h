/*
 *  tusb_config.h -- TinyUSB configuration for the bare-CMSIS link check.
 *
 *  TinyUSB requires the APPLICATION to supply this header; the stack will not compile without
 *  one on the include path. That is why it lives with the check target rather than in the
 *  light_usb modules: a real application's mix of classes and buffer sizes is its own business,
 *  and shipping one from a library would quietly override it.
 *
 *  Host MIDI is enabled here because light_usbhost_midi is what this target links, and it is
 *  the deepest path through the stack -- enabling it compiles the host controller driver, the
 *  enumeration machinery, the hub driver and the MIDI class together.
 */
#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

// CFG_TUSB_MCU is NOT set here. light_usb's CMakeLists derives it from LIGHT_CHIP and passes it
// as a compile definition, so this header stays chip-agnostic and the two cannot disagree.

#define CFG_TUSB_OS               OPT_OS_NONE
#define CFG_TUSB_DEBUG            0

// the root-hub port. Both STM32 families this check covers expose their USB core as port 0
#define BOARD_TUH_RHPORT          0
#define BOARD_TUH_MAX_SPEED       OPT_MODE_DEFAULT_SPEED

#define CFG_TUH_ENABLED           1
#define CFG_TUH_MAX_SPEED         BOARD_TUH_MAX_SPEED

// the device side is deliberately off: light_usbhost_midi is a host class, and leaving the
// device stack enabled would compile a second controller driver this target never enters
#define CFG_TUD_ENABLED           0

#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN        __attribute__ ((aligned(4)))

#define CFG_TUH_ENUMERATION_BUFSIZE 256

#define CFG_TUH_HUB               1
#define CFG_TUH_DEVICE_MAX        (CFG_TUH_HUB ? 4 : 1)
#define CFG_TUH_MIDI              CFG_TUH_DEVICE_MAX
#define CFG_TUH_ENDPOINT_MAX      8
#define CFG_TUH_API_EDPT_XFER     1

#define CFG_TUH_CDC               0
#define CFG_TUH_HID               0
#define CFG_TUH_MSC               0
#define CFG_TUH_VENDOR            0

#ifdef __cplusplus
}
#endif

#endif
