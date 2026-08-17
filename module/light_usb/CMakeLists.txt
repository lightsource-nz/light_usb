
add_library(light_usb_common INTERFACE)

target_include_directories(
        light_usb_common INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        light_usb_common INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/module.c
        ${CMAKE_CURRENT_LIST_DIR}/src/usb.c
)

target_link_libraries(
        light_usb_common INTERFACE
        light_core
)

if(LIGHT_SYSTEM STREQUAL PICO_SDK)
        target_link_libraries(
                light_usb_common INTERFACE
                pico_stdlib
        )
endif()
#   NO tinyusb_pico_pio_usb. This project uses the RP2's hardware USB controller only --
# crossfire_main/include/tusb_config.h sets CFG_TUH_RPI_PIO_USB to 0 and BOARD_TUH_RHPORT to
# the native port -- so the PIO-emulated host controller was linked in and never called.
#
#   it was not merely wasted: Pico-PIO-USB is not a submodule of anything. tinyusb GITIGNORES
# hw/mcu/raspberry_pi/ and its own tools/get_deps.py clones the library in from a third-party
# repository, so the sources existed on one developer's disk, tracked by no repository at all.
# Every rp2040 and rp2350 build of this project therefore depended on a directory that no clone
# reproduces, and CI found it immediately: `cannot find -ltinyusb_pico_pio_usb`.
#
#   dropping the link removes the dependency rather than papering over it with a fetch step for
# code nothing here executes. If PIO-USB is ever wanted (a second USB port on spare GPIOs), it
# comes back with CFG_TUH_RPI_PIO_USB=1 and a get_deps.py step, together.
if(LIGHT_SYSTEM STREQUAL PICO_SDK AND LIGHT_PLATFORM STREQUAL TARGET)
        target_link_libraries(
                light_usb_common INTERFACE
                tinyusb_board
        )
        #   tells usb.c that light_usb_init() brings the stack up by calling TinyUSB's BSP.
        # Replaces __HAVE_TINYUSB, which no build ever defined
        target_compile_definitions(light_usb_common INTERFACE LIGHT_USB_PORT_PICO)
endif()

#   BARE CMSIS: no SDK to hand us the stack, so it is compiled here from the checkout the
# container resolved. The container also chose the OPT_MCU and the portable driver for this chip
# -- see the comment there for why that mapping is not the framework's business.
#
#   tinyusb_sources_get() rather than a list of our own: TinyUSB maintains that list, and a copy
# here would rot silently as classes are added. It deliberately excludes the DCD and HCD drivers,
# which is why the port files are named separately -- the device controller is added here and the
# host controller by light_usbhost, mirroring how the Pico SDK splits tinyusb_board from
# tinyusb_host.
#
#   the include() is confined to THIS directory's scope deliberately: TinyUSB's src/CMakeLists.txt
# opens with cmake_minimum_required(VERSION 3.20), which raises policy settings wherever it lands.
# Including it from the container would apply that to every module below it.
if(LIGHT_SYSTEM STREQUAL CMSIS AND LIGHT_PLATFORM STREQUAL TARGET)
        include(${TINYUSB_PATH}/src/CMakeLists.txt)
        tinyusb_sources_get(_tu_src)

        foreach(_f IN LISTS LIGHT_USB_TU_DCD)
                list(APPEND _tu_src ${LIGHT_USB_TU_PORT}/${_f})
        endforeach()

        #   plus the clock TinyUSB asks the port for. It declares tusb_time_millis_api() and
        # tusb_time_delay_ms_api() and defines neither; the Pico SDK's integration answers them
        # on that path, so nothing here needed to until CMSIS did.
        list(APPEND _tu_src ${CMAKE_CURRENT_LIST_DIR}/src/portable/cmsis/tusb_time.c)

        #   and this chip's bring-up -- clocks, pins, transceiver supply, interrupt. The container
        # selected it and refuses to configure a chip that has none, so it is always set here.
        list(APPEND _tu_src ${CMAKE_CURRENT_LIST_DIR}/${LIGHT_USB_PLATFORM_SRC})

        target_sources(light_usb_common INTERFACE ${_tu_src})

        #   tells usb.c that light_usb_init() must do the bring-up itself. The Pico path has no
        # equivalent flag on purpose: there, TinyUSB's BSP does it and the application calls it
        target_compile_definitions(light_usb_common INTERFACE LIGHT_USB_PORT_CMSIS)

        #   src/ for <tusb.h> and friends. The CMSIS device header the port driver needs
        # (stm32h7xx.h, and the part define that header insists on) already arrives through
        # light_core, which links the selected chip port -- so none of the vendor header wiring
        # is repeated here.
        target_include_directories(light_usb_common INTERFACE ${TINYUSB_PATH}/src)

        #   CFG_TUSB_MCU is what selects the port inside TinyUSB's own headers. Undefined, it
        # fails with a message naming itself; set to the WRONG value it compiles and fails at
        # link on missing dcd_* symbols, naming nothing that leads back to the chip.
        target_compile_definitions(light_usb_common INTERFACE CFG_TUSB_MCU=${LIGHT_USB_TU_MCU})
endif()
