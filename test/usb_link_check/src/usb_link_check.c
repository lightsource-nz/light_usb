/*
 *  usb_link_check.c
 *  the only executable a bare-CMSIS build of light_usb produces.
 *
 *  WHY IT EXISTS: every module in this project is an INTERFACE library, so a "successful build"
 *  for a CMSIS target compiles nothing whatsoever -- CMake is happy, no source is touched, and a
 *  broken port configuration is indistinguishable from a working one. The Pico SDK path is
 *  covered incidentally, because crossfire links these modules; the CMSIS path had no consumer
 *  at all when it was written.
 *
 *  So this target's job is to be linked. It compiles the TinyUSB core, the class drivers and the
 *  chip's host controller driver, and then requires every symbol they reference to resolve --
 *  which is precisely what a wrong CFG_TUSB_MCU or a missing portable driver fails to do, and
 *  what no amount of `cmake --build` on the libraries alone would ever reveal.
 *
 *  It is NOT a hardware test. It never runs; the periodic main is present because the framework
 *  wants one, and because taking the address of the USB entry points is what stops the linker
 *  from garbage-collecting the thing being checked.
 */
#include <light.h>
#include <light_usb.h>
#include <light_usbhost.h>
#include <light_usbhost_midi.h>

#include <module/mod_usbhost_midi.h>

static void usb_link_check_event(const struct light_module *mod, uint8_t event, void *arg);
static uint8_t usb_link_check_main(struct light_application *app);

Light_Application_Define(usb_link_check, usb_link_check_event, usb_link_check_main,
                                                        &light_usbhost_midi);

static void usb_link_check_event(const struct light_module *mod, uint8_t event, void *arg)
{
        switch(event) {
        case LF_EVENT_MODULE_LOAD:
                //   the whole point: these calls drag the USB stack into the link. Reaching them
                // at runtime is not required and never happens -- resolving them at link time is
                // the check.
                light_usb_init();
                light_usbhost_midi_init();
                break;
        default:
                break;
        }
}

static uint8_t usb_link_check_main(struct light_application *app)
{
        return LF_STATUS_RUN;
}

//   a bare-CMSIS application supplies its own entry point -- the framework does not provide one,
// and the startup code's Reset_Handler branches straight to main(). Same shape as the
// framework's own demo_blackpill and demo_mini_stm32h7.
int main(void)
{
        light_framework_init();
        light_framework_run(0, NULL);
        return 0;
}
