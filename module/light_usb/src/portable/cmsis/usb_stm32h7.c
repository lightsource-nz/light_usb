/*
 *  usb_stm32h7.c -- OTG_FS bring-up for the STM32H7, in host mode.
 *
 *  WHAT THIS REPLACES: on a Pico target TinyUSB's BSP does this, and the SDK supplies the BSP.
 *  Bare CMSIS has no BSP at all, so the clocks, pins, transceiver supply and interrupt that the
 *  DWC2 driver assumes have already been set up are set up here instead.
 *
 *  MODELLED ON TinyUSB's own hw/bsp/stm32h7/family.c, which is the authoritative sequence --
 *  but written against the registers rather than ST's HAL, because this framework vendors only
 *  the CMSIS device headers and no HAL drivers.
 *
 *  NAMING, because it is genuinely confusing: on the H743 the FS-capable core on PA11/PA12 is
 *  the peripheral ST calls USB2_OTG_FS, its GPIO alternate function is named AF10 "OTG2_HS",
 *  and TinyUSB addresses it as rhport 0. All three names refer to one controller.
 *
 *  HOST MODE, per the decision this was written for: the board does not switch VBUS, so 5V for
 *  the attached device comes from outside. Nothing here drives a VBUS enable, and VBUS sensing
 *  is left off -- with no sense pin wired, enabling it would make the core wait forever for a
 *  session it cannot observe.
 *
 *  NOT HARDWARE-VERIFIED. This compiles and links; no device has enumerated through it.
 */
#include <light.h>

#include <stm32h7xx.h>

#include "tusb.h"

//   PA11 = DM, PA12 = DP. PA10 (ID) and PA9 (VBUS) are deliberately left alone: ID selects the
// role on an OTG cable and this port is fixed in host mode, and VBUS sense is off (see above).
#define USB_PIN_DM      11u
#define USB_PIN_DP      12u
#define USB_PIN_AF      10u

static void _gpio_set_af(GPIO_TypeDef *port, uint32_t pin, uint32_t af)
{
        //   MODER to alternate-function (0b10), push-pull, no pull, highest slew. "Very high
        // speed" is not optional decoration on a full-speed differential pair -- at 12 Mbit/s a
        // slower slew setting rounds the edges enough to fail eye-diagram timing, and the
        // symptom is intermittent enumeration rather than an obvious failure.
        port->MODER &= ~(3u << (pin * 2u));
        port->MODER |= (2u << (pin * 2u));
        port->OTYPER &= ~(1u << pin);
        port->OSPEEDR |= (3u << (pin * 2u));
        port->PUPDR &= ~(3u << (pin * 2u));

        //   AFR[0] covers pins 0-7 and AFR[1] pins 8-15, four bits each
        volatile uint32_t *afr = &port->AFR[pin >> 3u];
        *afr &= ~(0xFu << ((pin & 7u) * 4u));
        *afr |= (af << ((pin & 7u) * 4u));
}

//   spin on a hardware ready flag, but never forever.
//
//   AN UNBOUNDED WAIT HERE IS THE WRONG FAILURE. The first version of this file polled
// USB33RDY with a bare `while(!ready) {}`, and on the WeAct MiniSTM32H7xx that bit never
// asserts -- so the board did not "fail to bring up USB", it hung inside init before enabling
// the peripheral clock, and every OTG register read back as zero. From the outside that is
// indistinguishable from a dead board, and it took halting the CPU over SWD to see that the PC
// was sitting six bytes inside this function.
//   returns whether the flag arrived, so the caller can say so and carry on.
static bool _wait_ready(volatile uint32_t *reg, uint32_t mask, uint32_t spins)
{
        while(spins--) {
                if(*reg & mask)
                        return true;
        }
        return false;
}

//   a loop count rather than milliseconds: this runs before the USB clock is up and is called
// from init, where leaning on a timebase is more assumption than it is worth. Order-of-magnitude
// is all that is wanted -- long enough for a supply to settle, short enough not to look hung.
#define USB_READY_SPINS         1000000u

void light_usb_platform_init(void)
{
        //   1. the transceiver supply.
        //
        //   THE VOLTAGE DETECTOR, NOT THE REGULATOR. USBREGEN enables the internal 3.3V USB
        // regulator, which is for boards that feed VDD33USB from VDD50USB through the chip. This
        // board supplies VDD33USB externally, so that regulator has nothing to do and USB33RDY
        // never asserts -- measured on hardware: PWR_CR3 read back 0x02000042, USBREGEN set and
        // USB33RDY clear, permanently.
        //   USB33DEN enables the level detector instead, which is what TinyUSB's own
        // hw/bsp/stm32h7 does -- and its comment notes board init works even without it. So a
        // detector that never reports ready is a warning, not a reason to stop.
        PWR->CR3 |= PWR_CR3_USB33DEN;
        if(!_wait_ready(&PWR->CR3, PWR_CR3_USB33RDY, USB_READY_SPINS)) {
                light_warn("usb: VDD33USB not reported ready (PWR_CR3=0x%x); continuing",
                                                (unsigned) PWR->CR3);
        }

        //   2. a 48MHz reference. HSI48 rather than PLL3Q: it needs no PLL configuration, and it
        // leaves the board's own clock tree alone -- this file has no business reconfiguring
        // PLLs that light_core's chip port already set up for the CPU and peripherals.
        RCC->CR |= RCC_CR_HSI48ON;
        if(!_wait_ready(&RCC->CR, RCC_CR_HSI48RDY, USB_READY_SPINS)) {
                //   fatal in a way the supply is not: with no 48MHz reference the core cannot
                // clock the bus at all, so continuing would present as a silent absence of
                // traffic rather than as this
                light_error("usb: HSI48 did not start (RCC_CR=0x%x); USB will not function",
                                                (unsigned) RCC->CR);
                return;
        }
        RCC->D2CCIP2R &= ~RCC_D2CCIP2R_USBSEL;
        RCC->D2CCIP2R |= (3u << RCC_D2CCIP2R_USBSEL_Pos);   // 3 = HSI48

        //   3. the pins, before the peripheral clock: the core samples the line state as it comes
        // out of reset, so pins still in their default analog mode at that moment give it a
        // reading it will act on.
        RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;
        (void) RCC->AHB4ENR;   // ST erratum: a peripheral clock enable needs a read-back before use
        _gpio_set_af(GPIOA, USB_PIN_DM, USB_PIN_AF);
        _gpio_set_af(GPIOA, USB_PIN_DP, USB_PIN_AF);

        //   4. the controller itself
        RCC->AHB1ENR |= RCC_AHB1ENR_USB2OTGFSEN;
        (void) RCC->AHB1ENR;

        //   5. the interrupt. TinyUSB's host stack is driven by tuh_int_handler() from the ISR;
        // tuh_task() alone will not see a transfer complete. Enabled last, so nothing fires
        // before the clocks and pins above are in place.
        //
        //   the vector is OTG_FS_IRQn (101). Note that the CMSIS header's COMMENTS for this
        // block are shifted by one and describe it as an HS2 wakeup -- the enum name and the
        // startup file's vector table agree with each other, and they are what matter.
        NVIC_SetPriority(OTG_FS_IRQn, 2);
        NVIC_EnableIRQ(OTG_FS_IRQn);
}

//   named by the vector table in startup_stm32h743xx.s, which declares it weak and aliases it to
// Default_Handler; defining it here overrides that alias
void OTG_FS_IRQHandler(void)
{
        tuh_int_handler(0, true);
}
