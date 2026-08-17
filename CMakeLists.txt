cmake_minimum_required(VERSION 3.18)

#   This file is added two different ways, and it has to work both times -- the same shape
# light_display uses.
#
#   NORMALLY a consuming project has already added the framework, so light_core exists and there
# is nothing to resolve; the file reduces to the add_subdirectory() list at the bottom.
#
#   STANDALONE (cmake -S . -B build) it is the top-level project and stands the framework up
# itself, so a build failure in a USB module is attributable to that module rather than to
# whichever application happened to be building it.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        #   LIGHT_PATH by hand, for the obvious reason: light_resolve_project() lives in the
        # framework, so it cannot be used to find the framework. Everything after
        # light_preinit.cmake can use it.
        if(DEFINED ENV{LIGHT_PATH} AND (NOT LIGHT_PATH))
                set(LIGHT_PATH "$ENV{LIGHT_PATH}" CACHE PATH "path to the light framework project")
        endif()
        if(NOT LIGHT_PATH)
                set(LIGHT_PATH "${CMAKE_CURRENT_LIST_DIR}/../light_framework_mk3" CACHE PATH "path to the light framework project")
        endif()
        get_filename_component(LIGHT_PATH "${LIGHT_PATH}" REALPATH BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
        if(NOT EXISTS "${LIGHT_PATH}/light_init.cmake")
                message(FATAL_ERROR
                        "could not find the light framework at '${LIGHT_PATH}'.\n"
                        "Check it out beside this project as ../light_framework_mk3, or set LIGHT_PATH.")
        endif()

        include(${LIGHT_PATH}/external/light_preinit.cmake)

        #   the project and its core module share this name. Harmless -- project() and
        # add_library() are different namespaces, and the core module's target is
        # light_usb_common rather than light_usb -- but it is why the consuming side passes a
        # MARKER to light_resolve_project()
        project(light_usb C CXX ASM)

        add_subdirectory(${LIGHT_PATH} light_framework)
endif()

#   WHERE THE USB STACK COMES FROM, resolved once here so every module below agrees.
#
#   ON A PICO TARGET the Pico SDK vendors TinyUSB at lib/tinyusb and exposes it as CMake targets
# (tinyusb_board, tinyusb_host). Using anything else there would mean two copies of the stack in
# one binary, so the SDK's copy wins and there is nothing to resolve.
#
#   ON BARE CMSIS there is no SDK to provide it, so the stack is compiled from a checkout of its
# own -- found the same way every other dependency is: an explicit TINYUSB_PATH, the environment,
# then a sibling ../tinyusb. MARKER is src/tusb.h because TinyUSB has no top-level CMakeLists.txt,
# so the default marker would never match even on a correct checkout.
#
#   the chip mapping lives here rather than in the framework: which OPT_MCU and which portable
# driver a part needs is TinyUSB's business, and teaching light_core_chip_* about it would put
# USB knowledge in ports that have no USB code.
if(LIGHT_SYSTEM STREQUAL CMSIS AND LIGHT_PLATFORM STREQUAL TARGET)
        light_resolve_project(TINYUSB tinyusb MARKER src/tusb.h)

        #   ST splits across two unrelated device controllers, so this is a real fork rather than
        # a naming difference: F4 and H7 carry the Synopsys DWC2 core, while F102/F103 have ST's
        # own "fsdev" full-speed device. Picking the wrong one compiles and fails at link with
        # missing dcd_* symbols, naming nothing that leads back to the chip.
#   LIGHT_USB_PLATFORM_SRC is this project's own bring-up for the part -- clocks, pins,
        # transceiver supply, interrupt -- which is separate from choosing TinyUSB's driver.
        # A chip can have a perfectly good TinyUSB port and still have no bring-up written, and
        # that combination builds, links, and does nothing at all on hardware. So a chip without
        # one is refused rather than quietly produced.
        if(LIGHT_CHIP STREQUAL stm32h743)
                set(LIGHT_USB_TU_MCU  OPT_MCU_STM32H7)
                set(LIGHT_USB_TU_PORT ${TINYUSB_PATH}/src/portable/synopsys/dwc2)
                set(LIGHT_USB_TU_DCD  dwc2_common.c dcd_dwc2.c)
                set(LIGHT_USB_TU_HCD  hcd_dwc2.c)
                set(LIGHT_USB_PLATFORM_SRC src/portable/cmsis/usb_stm32h7.c)
        elseif(LIGHT_CHIP STREQUAL stm32f411)
                set(LIGHT_USB_TU_MCU  OPT_MCU_STM32F4)
                set(LIGHT_USB_TU_PORT ${TINYUSB_PATH}/src/portable/synopsys/dwc2)
                set(LIGHT_USB_TU_DCD  dwc2_common.c dcd_dwc2.c)
                set(LIGHT_USB_TU_HCD  hcd_dwc2.c)
                #   the F411 carries the same DWC2 core, so TinyUSB needs nothing new -- but its
                # clock tree and supply differ enough (no dedicated USB regulator, 48MHz from
                # PLL Q rather than HSI48) that the H7 sequence would not bring it up
                set(LIGHT_USB_PLATFORM_SRC "")
        elseif(LIGHT_CHIP STREQUAL stm32f103)
                set(LIGHT_USB_TU_MCU  OPT_MCU_STM32F1)
                set(LIGHT_USB_TU_PORT ${TINYUSB_PATH}/src/portable/st/stm32_fsdev)
                set(LIGHT_USB_TU_DCD  fsdev_common.c dcd_stm32_fsdev.c)
                set(LIGHT_USB_TU_HCD  hcd_stm32_fsdev.c)
                set(LIGHT_USB_PLATFORM_SRC "")
        else()
                message(FATAL_ERROR
                        "light_usb has no TinyUSB port mapping for LIGHT_CHIP='${LIGHT_CHIP}'.\n"
                        "Supported on CMSIS: stm32h743, stm32f411 (Synopsys DWC2), stm32f103 (ST fsdev).\n"
                        "Adding one means naming its CFG_TUSB_MCU and portable driver in "
                        "${CMAKE_CURRENT_LIST_FILE}.")
        endif()

        if(NOT LIGHT_USB_PLATFORM_SRC)
                message(FATAL_ERROR
                        "light_usb has a TinyUSB port for LIGHT_CHIP='${LIGHT_CHIP}' but no bring-up "
                        "for it: nothing would enable the USB clocks, configure the pins, power the "
                        "transceiver or route the interrupt.\n"
                        "That combination compiles and links and does nothing on hardware, which is "
                        "why it is refused here rather than built.\n"
                        "Implemented so far: stm32h743. Adding one means a src/portable/cmsis/ file "
                        "defining light_usb_platform_init(), and naming it in "
                        "${CMAKE_CURRENT_LIST_FILE}.")
        endif()

        light_info("light_usb: TinyUSB from ${TINYUSB_PATH} (${LIGHT_USB_TU_MCU})")
endif()

#   EVERY module, unconditionally. All four are INTERFACE libraries: they contribute no sources
# and compile nothing until a target links them, so a consumer wanting only the MIDI device class
# pays nothing for the host stack. Same argument the framework makes for its chip ports.
#
#   ORDER FOLLOWS THE DEPENDENCY CHAIN -- light_usb defines light_usb_common, which the other
# three sit on; light_usbhost_midi links both light_usb_midi and light_usbhost. CMake resolves
# link targets at generate time and would accept any order, but declaring a dependency after its
# dependent reads as an accident to whoever edits this next.
add_subdirectory(module/light_usb)
add_subdirectory(module/light_usb_midi)
add_subdirectory(module/light_usbhost)
add_subdirectory(module/light_usbhost_midi)

#   the CMSIS link check, and it only exists standalone. A consuming project adds this file as a
# subdirectory and does not want a stray executable in its build; but when this project IS the
# build, an executable is the only way any of the above gets compiled at all.
#
#   guarded on CMSIS/TARGET because that is the path with no other consumer: the Pico SDK path is
# exercised every time crossfire builds, while nothing linked these modules on CMSIS before.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR
                AND LIGHT_SYSTEM STREQUAL CMSIS AND LIGHT_PLATFORM STREQUAL TARGET)
        add_subdirectory(test/usb_link_check)
endif()
