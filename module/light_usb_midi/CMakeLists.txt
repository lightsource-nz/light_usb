
add_library(light_usb_midi INTERFACE)

target_include_directories(
        light_usb_midi INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        light_usb_midi INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/module.c
        ${CMAKE_CURRENT_LIST_DIR}/src/usb_midi.c
)

#   everything TinyUSB-related arrives through light_usb_common, which is where tinyusb_board is
# linked on a PICO_SDK target build. A PICO_SDK block here used to follow, calling
# target_link_libraries() with an empty library list -- valid CMake that does nothing, and easy
# to mistake for a place something belongs.
target_link_libraries(
        light_usb_midi INTERFACE
        light_usb_common
)
