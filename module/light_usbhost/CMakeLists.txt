
add_library(light_usbhost INTERFACE)

target_include_directories(
        light_usbhost INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        light_usbhost INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/module.c
        ${CMAKE_CURRENT_LIST_DIR}/src/usbhost.c
)

target_link_libraries(
        light_usbhost INTERFACE
        light_usb_common
)

if(LIGHT_SYSTEM STREQUAL PICO_SDK AND LIGHT_PLATFORM STREQUAL TARGET)
        target_link_libraries(
                light_usbhost INTERFACE
                tinyusb_host
        )
endif()

#   BARE CMSIS: the host controller driver, and only that. Everything else TinyUSB needs -- the
# common and class sources, the include path, CFG_TUSB_MCU and the DEVICE controller -- arrives
# through light_usb_common, which this links.
#
#   the split is the same one the Pico SDK makes between tinyusb_board and tinyusb_host, and it
# is what keeps a device-only application from compiling the host stack it never enters.
if(LIGHT_SYSTEM STREQUAL CMSIS AND LIGHT_PLATFORM STREQUAL TARGET)
        foreach(_f IN LISTS LIGHT_USB_TU_HCD)
                target_sources(light_usbhost INTERFACE ${LIGHT_USB_TU_PORT}/${_f})
        endforeach()
endif()
