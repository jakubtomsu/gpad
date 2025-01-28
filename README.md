# gpad
Simple C/C++/Odin library for game controller input.

> [!WARNING]
> This library currently works only on windows, with support for XInput and DirectInput remapped using SDL controller DB. It's also missing support for device connected window messages.
>
> I am not planning to add more features. I didn't want to release this lib because it's unfinished, but now since some people asked me about it I decided to make it public anyway. That said any PRs are appreciated.
> To learn more about this topic I strongly recommend reading [Joystick Input Examples by MysteriousJ](https://github.com/MysteriousJ/Joystick-Input-Examples)
>
> Use at your own risk.

All raw device inputs are mapped to the Xbox controller layout.


## Usage
In _only one_ C file, define `GPAD_IMPLEMENTATION`. Then you can just `#include "gpad.h"` anywhere else.
```c
#define GPAD_IMPLEMENTATION
#include "gpad.h"
```

Init and shutdown context:
```cpp
gpad_initialize();

while (true) {
    // Tick ...
}

gpad_shutdown();
```

Iterate all devices and poll their input:
```cpp
for (int i = 0; i < GPAD_MAX_DEVICES; i++) {
    Gpad_Device_State state = {0};
    // gpad_device_state will return false if the device on that ID is not connected.
    if (gpad_device_state(i, &state)) {
        for(int button = 0; button < Gpad_Button_COUNT; button++) {
            printf("%s: %s", gpad_button_name(button), gpad_device_button_pressed(&state, button) ? "down" : "up");
        }
    }
}
```

Or alternatively list all available devices using `gpad_list_devices` helper function.
```cpp
Gpad_Device_Id devices[GPAD_MAX_DEVICES] = {0};
int device_count = gpad_list_devices(&devices[0], GPAD_MAX_DEVICES);
for (int i = 0; i < device_count; i++) {
    // use ID from device[i] ...
}
```

## Examples

The examples are written in odinlang for simplicity, but should be easy enough to understand for a C programmer.

Run with `odin run examples/some_example`.

The `visual` example is a simple raylib app which visualizes all the input states.

## Build controller DB
This library uses [SDL Game Controller DB](https://github.com/gabomdq/SDL_GameControllerDB) to map raw joystick inputs to the Xbox controller layout. The `gamecontrollerdb.txt` contains the original data, and `build_db` is a program which  parses it and generates C code to `gpad_gamecontrollerdb.inl`.

The `build_db` tool is written in [Odin](https://github.com/odin-lang/Odin). To build the database, install Odin and run (from the gpad directory):
```bat
odin run build_db
```
