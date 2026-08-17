#   The only executable a bare-CMSIS build of this project produces, and therefore the only thing
# that compiles the TinyUSB stack for CMSIS at all -- every module here is an INTERFACE library.
#
#   Not registered with add_test(): it is a cross-compiled firmware image the host cannot run.
# What it verifies is that the sources compile and the link resolves, which is exactly the class
# of failure a wrong CFG_TUSB_MCU or a missing portable driver produces.

add_executable(usb_link_check)

target_sources(usb_link_check PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src/usb_link_check.c)

#   TinyUSB requires the application to supply tusb_config.h, and finds it on the include path.
# It lives here rather than in the modules because a real application's choice of classes and
# buffer sizes is its own; a library shipping one would silently override it.
target_include_directories(usb_link_check PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

#   light_usbhost_midi pulls the whole chain -- light_usb_common for the core and the device
# controller, light_usbhost for the host controller, light_usb_midi for the class -- so linking
# this one target compiles every line of the CMSIS path
target_link_libraries(
        usb_link_check PUBLIC
        light_usbhost_midi
)

# .bin/.hex and the size breakdown, through the property hook cmake/cmsis.cmake installs
light_target_set_property(usb_link_check application_target 1)
