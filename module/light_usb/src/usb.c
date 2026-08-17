#include <light_usb.h>

#ifdef __HAVE_TINYUSB
#include "bsp/board.h"
#include "tusb.h"
#endif

#ifdef __HAVE_RP2_HW
#define LIGHT_USB_NATIVE_PORT_COUNT 1
#define LIGHT_USB_PIO_ENABLED 1
#define LIGHT_USB_PIO_SM_MIN 0
#define LIGHT_USB_PIO_SM_MAX 7
#else
#define LIGHT_USB_NATIVE_PORT_COUNT 0
#endif

static struct light_usb_port _native_port[LIGHT_USB_NATIVE_PORT_COUNT];

//   the bare-CMSIS bring-up: clocks, pins, transceiver supply and the USB interrupt, none of
// which exists without a BSP. Implemented per chip under src/portable/cmsis/ and selected by
// the container's CMakeLists, which refuses to configure a chip it has no port for.
#ifdef LIGHT_USB_PORT_CMSIS
extern void light_usb_platform_init(void);
#endif

void light_usb_init()
{
        //   NOTE __HAVE_TINYUSB IS DEFINED NOWHERE -- not in any CMakeLists, cmake module or
        // header in this project, the framework, or crossfire. So this branch has never been
        // compiled, on any platform, and board_init() has never been called from here.
        //   it is inert rather than broken: crossfire calls board_init() itself during its own
        // startup, so the Pico path has always been initialised, just not through this function.
        // Left exactly as-is deliberately -- defining the macro now would call board_init() a
        // second time on a path that is working and hardware-verified.
#ifdef __HAVE_TINYUSB
        board_init();
#endif

        //   CMSIS has no such fallback: nothing else sets the USB peripheral up, so this call
        // is the only bring-up there is
#ifdef LIGHT_USB_PORT_CMSIS
        light_usb_platform_init();
#endif

        for(uint8_t i = 0; i < LIGHT_USB_NATIVE_PORT_COUNT; i++) {

        }
}

uint8_t light_usb_get_native_port_count()
{
        return LIGHT_USB_NATIVE_PORT_COUNT;
}
struct light_usb_port *light_usb_get_native_port(uint8_t phys_id);
bool light_usb_pio_driver_available();
struct light_usb_port *light_usb_create_pio_port(struct light_usb_pio_port_desc desc);

uint8_t light_usb_get_port_status(struct light_usb_port *port);
bool light_usb_port_has_capability(struct light_usb_port *port, uint8_t capability);
bool light_usb_claim_port(uint8_t port_id, struct light_module *owner);
void light_usb_release_port(struct light_usb_port *port);
bool light_usb_activate_port(struct light_usb_port *port, uint8_t port_mode);
void light_usb_deactivate_port(struct light_usb_port *port)
{

}
