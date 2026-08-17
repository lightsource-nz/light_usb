/*
 *  tusb_time.c -- the clock TinyUSB asks the port for, on bare CMSIS.
 *
 *  TinyUSB declares these two in common/tusb_common.h and defines neither: the port is expected
 *  to supply them. On a Pico target the SDK's TinyUSB integration does it, which is why nothing
 *  here was needed for years -- crossfire only ever built against the SDK.
 *
 *  Without them a CMSIS build compiles cleanly and fails at link with a wall of
 *  `undefined reference to 'tusb_time_millis_api'` from tusb.c and usbh.c, naming a symbol that
 *  appears nowhere in this project's source and giving no hint that the answer is a
 *  four-line file.
 *
 *  Both are answered from light_platform, so USB timing and framework timing read the same
 *  clock. That matters more than it looks: TinyUSB times enumeration and control transfers off
 *  this, so a clock that disagrees with the one the application schedules against produces
 *  timeouts that appear to be device faults.
 */
//   light.h is the whole framework surface: it pulls in light_platform.h, which is where
// light_platform_get_time_since_init() and light_platform_sleep_ms() are declared. Including
// that header directly is not the way in -- it depends on types light.h establishes first.
#include <light.h>

#include <stdint.h>

uint32_t tusb_time_millis_api(void)
{
        //   time since init rather than the absolute counter: TinyUSB only ever differences
        // these values, and the since-init form starts near zero, so the wrap it inherits is
        // ~49.7 days after boot rather than at an arbitrary point in the first run.
        return light_platform_get_time_since_init();
}

void tusb_time_delay_ms_api(uint32_t ms)
{
        light_platform_sleep_ms(ms);
}
