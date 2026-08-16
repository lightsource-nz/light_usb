# light_usb

USB support for the light framework: the shared device/host core, and the MIDI class drivers
built on it.

Note that this project and its core module share the name `light_usb` -- the group is this
repository, the module is `module/light_usb` inside it. That matters in one place only, and it
is handled: see *Using it* below.

| module | target | what it is |
|---|---|---|
| `light_usb` | `light_usb_common` | the shared USB core: stack init, the periodic task, and the device-side plumbing both roles sit on |
| `light_usb_midi` | `light_usb_midi` | USB-MIDI **device** class -- the board appears as a MIDI instrument |
| `light_usbhost` | `light_usbhost` | USB **host** core, over TinyUSB's host stack |
| `light_usbhost_midi` | `light_usbhost_midi` | USB-MIDI **host** class -- the board enumerates and drives an attached MIDI instrument |

The dependency graph is a chain rather than a web:

```
    light_usb_common
      |        |
      |        +-- light_usb_midi ----+
      |                               +-- light_usbhost_midi
      +-- light_usbhost --------------+
```

## Why these live together

They were four directories inside the crossfire application, which meant the only way to use
USB in a second project was to copy them. Nothing about them is specific to crossfire: the
group depends on `light_core` and, on a Pico target, TinyUSB via the SDK -- there is no display,
no renderer and no application code in it.

Each module's history is preserved. They were extracted with `git subtree split`, so every
original commit is here and `git blame` reaches through the move.

The same wrinkle applies as in `light_display`: a path-filtered `git log -- module/light_usb`
shows only the import, because git's history simplification stops at a merge whose first parent
already contains the result. The commits are all present --

```sh
git log --oneline --full-history -- module/light_usb   # includes the merges
git log --oneline <import-commit>^2                    # that module's own history alone
```

## Using it

Consuming projects resolve this repository by path rather than vendoring it as a submodule:

```cmake
light_resolve_project(LIGHT_USB light_usb MARKER module/light_usb/CMakeLists.txt)
add_subdirectory(${LIGHT_USB_PATH} light_usb_group)
```

**Pass the `MARKER`.** It is the one place the shared name bites. `module/light_usb` is both "an
in-project checkout of the group" and "a checkout of the core module", and the resolver prefers
an in-project `module/<name>` over the sibling. `module/light_usb/CMakeLists.txt` exists only in
the group, so it settles the question. (The CMake *targets* do not collide -- the core module
defines `light_usb_common`, not `light_usb` -- so this is purely about locating the directory.)

Adding the group defines every module, which costs nothing: all four are INTERFACE libraries
that contribute no sources until something links them. Link what you need:

```cmake
target_link_libraries(my_app PRIVATE light_usbhost_midi)   # pulls the rest of the chain
```

## Dependencies

`light_core` from the framework, and on a `PICO_SDK`/`TARGET` build the SDK's `tinyusb_board`
and `tinyusb_host`. Nothing else.

Deliberately **not** `tinyusb_pico_pio_usb`: these modules drive the RP2's hardware USB
controller. The PIO-emulated controller is a separate library that TinyUSB neither vendors nor
submodules, and linking it made builds depend on a directory no clone reproduces.
