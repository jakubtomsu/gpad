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
    A              = 0,
    B              = 1,
    X              = 2,
    Y              = 3,
    Left_Shoulder  = 4,
    Right_Shoulder = 5,
    Back           = 6,
    Start          = 7,
    Guide          = 8,
    Left_Thumb     = 9,
    Right_Thumb    = 10,
    Dpad_Up        = 11,
    Dpad_Right     = 12,
    Dpad_Down      = 13,
    Dpad_Left      = 14,
}

Axis :: enum u8 {
    Left_X        = 0,
    Left_Y        = 1,
    Right_X       = 2,
    Right_Y       = 3,
    Left_Trigger  = 4,
    Right_Trigger = 5,
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
