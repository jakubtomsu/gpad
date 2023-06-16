# gpad
STB-style single header C/C++/Odin library for game controller input.

All raw device inputs are mapped to the Xbox controller layout.


## Usage
```c
#define GPAD_IMPLEMENTATION
#include "gpad.h"


```

```c
gpad_initialize();

Gpad_Device_Id devices[GPAD_MAX_DEVICES] = {0};
if (gpad_list_devices(&devices[0], GPAD_MAX_DEVICES)) {
  for (int i
    Gpad_Device_State state = {0};
    if (gpad_device_state(
}

gpad_shutdown();
```

## Build controller DB
This library uses [SDL Game Controller DB](https://github.com/gabomdq/SDL_GameControllerDB) to map raw joystick inputs to the Xbox controller layout. The `gamecontrollerdb.txt` contains the original data, and `build_db` is a program which  parses it and generates C code to `gpad_gamecontrollerdb.inl`.

The `build_db` tool is written in [Odin](https://github.com/odin-lang/Odin). To build the database, install Odin and run (from the gpad directory):
```bat
odin run build_db
```
