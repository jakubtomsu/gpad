package gpad

import "core:c"

// These definitions must be the same as in gpad.h!

MAX_DEVICES :: 8
ID_INVALID :: ~Device_Id(0)
AXIS_MAX :: 1.0
AXIS_MIN :: -1.0

Device_Id :: distinct u8

// PS4 layout:
//      Cross:      A
//      Circle:     B
//      Square:     X
//      Triangle:   Y
Button :: enum u8 {
    a              = 0,
    b              = 1,
    x              = 2,
    y              = 3,
    left_shoulder  = 4,
    right_shoulder = 5,
    back           = 6,
    start          = 7,
    guide          = 8,
    left_thumb     = 9,
    right_thumb    = 10,
    dpad_up        = 11,
    dpad_right     = 12,
    dpad_down      = 13,
    dpad_left      = 14,
}

Axis :: enum u8 {
    left_x        = 0,
    left_y        = 1,
    right_x       = 2,
    right_y       = 3,
    left_trigger  = 4,
    right_trigger = 5,
}

Device_State :: struct {
    buttons: bit_set[Button;u16],
    axes:    [Axis]f32,
}

when ODIN_OS == .Windows {
    when ODIN_DEBUG {
        foreign import lib "gpad_windows_x64_debug.lib"
    } else {
        foreign import lib "gpad_windows_x64_release.lib"
    }
}

@(default_calling_convention = "c", link_prefix = "gpad_")
foreign lib {
    initialize :: proc() ---
    shutdown :: proc() ---
    is_initialized :: proc() -> bool ---
    refresh_connected_devices :: proc() ---
    poll_device :: proc(device: Device_Id, out_state: ^Device_State) -> bool ---
    rumble_device :: proc(device: Device_Id, low_frequency: f32, high_frequency: f32) -> bool ---

    // Utilities

    list_devices :: proc(devices: [^]Device_Id, max_devices: c.int) -> c.int ---
    button_name :: proc(button: Button) -> cstring ---
    axis_name :: proc(axis: Axis) -> cstring ---
    device_button_pressed :: proc(#by_ptr state: Device_State, button: Button) -> bool ---
}

poll :: proc(device: Device_Id) -> (result: Device_State, ok: bool) #optional_ok {
    ok = poll_device(device, &result)
    return
}

list_devices_slice :: proc(allocator := context.temp_allocator) -> []Device_Id {
    buf := new([MAX_DEVICES]Device_Id, allocator)
    num := list_devices(&buf[0], MAX_DEVICES)
    if num < 0 do num = 0
    return buf[:num]
}
