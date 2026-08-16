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
