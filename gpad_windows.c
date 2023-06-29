#if _WIN32
#define GPAD_PLATFORM_WINDOWS
#include "gpad.h"

#include <string.h>
#include <stdio.h>

#define DIRECTINPUT_VERSION 0x0800

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Xinput.h>
#include <dinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "xinput.lib")


typedef uint16_t Gpad_Mapping_Index;
#define GPAD_MAPPING_INDEX_INVALID ((Gpad_Mapping_Index)~0)

typedef uint8_t Gpad_Mapping_Kind;

typedef enum Gpad_Mapping_Kind_ {
    Gpad_Mapping_Kind_Axis,
    Gpad_Mapping_Kind_Button,
    Gpad_Mapping_Kind_Hatbit,
} Gpad_Mapping_Kind_;

typedef struct Gpad_Mapping_Elem {
    Gpad_Mapping_Kind kind;
    uint8_t index;
    int8_t axis_scale;
    int8_t axis_offset;
} Gpad_Mapping_Elem;

// String, last char is 0.
typedef char Gpad_Guid[33];

typedef struct Gpad_Mapping {
    const char* name;
    Gpad_Mapping_Elem buttons[Gpad_Button_COUNT];
    Gpad_Mapping_Elem axes[Gpad_Axis_COUNT];
} Gpad_Mapping;

#include "gpad_gamecontrollerdb.inl"

#define GPAD__NUM_MAPPINGS (sizeof(g_gpad_mappings) / sizeof(g_gpad_mappings[0]))

bool gpad_device_button_pressed(const Gpad_Device_State* state, const Gpad_Button button) {
    if(state && button < Gpad_Button_COUNT) {
        return state->buttons & (1 << button);
    }
    return false;
}

const char* gpad_button_name(Gpad_Button button) {
    switch(button) {
        case Gpad_Button_A: return "A";
        case Gpad_Button_B: return "B";
        case Gpad_Button_X: return "X";
        case Gpad_Button_Y: return "Y";
        case Gpad_Button_Left_Shoulder: return "Left_Shoulder";
        case Gpad_Button_Right_Shoulder: return "Right_Shoulder";
        case Gpad_Button_Back: return "Back";
        case Gpad_Button_Start: return "Start";
        case Gpad_Button_Guide: return "Guide";
        case Gpad_Button_Left_Thumb: return "Left_Thumb";
        case Gpad_Button_Right_Thumb: return "Right_Thumb";
        case Gpad_Button_Dpad_Up: return "Dpad_Up";
        case Gpad_Button_Dpad_Right: return "Dpad_Right";
        case Gpad_Button_Dpad_Down: return "Dpad_Down";
        case Gpad_Button_Dpad_Left: return "Dpad_Left";
    }
    return "<Invalid>";
}

const char* gpad_axis_name(Gpad_Axis axis) {
    switch(axis) {
        case Gpad_Axis_Left_X: return "Left_X";
        case Gpad_Axis_Left_Y: return "Left_Y";
        case Gpad_Axis_Right_X: return "Right_X";
        case Gpad_Axis_Right_Y: return "Right_Y";
        case Gpad_Axis_Left_Trigger: return "Left_Trigger";
        case Gpad_Axis_Right_Trigger: return "Right_Trigger";
    }
    return "<Invalid>";
}

// Returns -1 on failure.
static int gpad__find_mapping(const char* guid) {
    for(int i = 0; i < GPAD__NUM_MAPPINGS; i++) {
        if(strncmp(guid, &g_gpad_mapping_guids[i][0], sizeof(Gpad_Guid)) == 0) {
            return i;
        }
    }
    return -1;
}

static const char* gpad__mapping_kind_name(Gpad_Mapping_Kind kind) {
    switch(kind) {
        case Gpad_Mapping_Kind_Axis: return "Axis";
        case Gpad_Mapping_Kind_Button: return "Button";
        case Gpad_Mapping_Kind_Hatbit: return "Hatbit";
    }
    return "<Invalid>";
}

#define GPAD__MAX_DINPUT_DEVICES 8

typedef uint8_t Gpad_Device_Entry_Kind;

typedef enum Gpad_Device_Entry_Kind_ {
    Gpad_Device_Entry_Kind_DInput8,
    Gpad_Device_Entry_Kind_XInput,
} Gpad_Device_Entry_Kind_;

typedef struct Gpad_Device_Entry {
    Gpad_Mapping_Index mapping_index;
    Gpad_Device_Entry_Kind kind;

    union {
        IDirectInputDevice8* dinput8_device;
        DWORD xinput_user_index;
    };

    Gpad_Guid guid;
} Gpad_Device_Entry;

typedef struct Gpad_Context {
    bool initialized;

    HINSTANCE instance;
    Gpad_Device_Entry id_entries[GPAD_MAX_DEVICES];
    IDirectInput8* dinput8;
} Gpad_Context;

static Gpad_Context gpad__context = {0};

static void gpad__clear_device_entries(void) {
    memset(&gpad__context.id_entries[0], 0, sizeof(gpad__context.id_entries));
    for(int id = 0; id < GPAD_MAX_DEVICES; id++) {
        gpad__context.id_entries[id].mapping_index = GPAD_MAPPING_INDEX_INVALID;
    }
}

static void gpad__remove_device_entry(const Gpad_Device_Id id) {
    if(id <= GPAD_MAX_DEVICES) {
        memset(&gpad__context.id_entries[id], 0, sizeof(Gpad_Device_Entry));
        gpad__context.id_entries[id].mapping_index = GPAD_MAPPING_INDEX_INVALID;
    }
}

static Gpad_Device_Id gpad__find_unused_device_id(void) {
    for(int id = 0; id < GPAD_MAX_DEVICES; id++) {
        if(gpad__context.id_entries[id].mapping_index == GPAD_MAPPING_INDEX_INVALID) {
            return id;
        }
    }

    return GPAD_ID_INVALID;
}

static void gpad__init_state(Gpad_Device_State* state) {
    state->buttons = 0;
    for(int axis = 0; axis < Gpad_Axis_COUNT; axis++) {
        state->axes[axis] = GPAD_AXIS_MIN;
    }
}

static bool gpad__supports_xinput(const GUID* guid) {
    RAWINPUTDEVICELIST ridl[32];
    UINT count = sizeof(ridl) / sizeof(ridl[0]);

    const UINT num_devices = GetRawInputDeviceList(&ridl[0], &count, sizeof(RAWINPUTDEVICELIST));
    if(num_devices == (UINT)-1) {
        return false;
    }

    for(int i = 0; i < num_devices; i++) {
        if(ridl[i].dwType != RIM_TYPEHID) continue;

        RID_DEVICE_INFO rdi = {0};
        rdi.cbSize = sizeof(rdi);
        UINT size = sizeof(rdi);

        if((INT)GetRawInputDeviceInfoA(ridl[i].hDevice, RIDI_DEVICEINFO, &rdi, &size) == -1) {
            continue;
        }

        if(MAKELONG(rdi.hid.dwVendorId, rdi.hid.dwProductId) != (LONG)guid->Data1) continue;

        char name[256] = {0};
        size = sizeof(name);

        if((INT)GetRawInputDeviceInfoA(ridl[i].hDevice, RIDI_DEVICENAME, name, &size) == -1) {
            break;
        }

        name[sizeof(name) - 1] = '\0';
        if(strstr(name, "IG_")) {
            return true;
        }
    }

    return false;
}

static BOOL CALLBACK gpad__dinput_enum_devices_callback(LPCDIDEVICEINSTANCE device_instance, LPVOID userData) {
    IDirectInputDevice8* device = 0;
    IDirectInput8_CreateDevice(gpad__context.dinput8, &device_instance->guidInstance, &device, NULL);

    IDirectInputDevice8_SetCooperativeLevel(device, GetActiveWindow(), DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
    IDirectInputDevice8_SetDataFormat(device, &c_dfDIJoystick);
    IDirectInputDevice8_Acquire(device);

    if(gpad__supports_xinput(&device_instance->guidProduct)) {
        return DIENUM_CONTINUE;
    }

    char name[256] = "";

    if(!WideCharToMultiByte(
           CP_UTF8, 0, (LPCWCH)&device_instance->tszInstanceName[0], -1, name, sizeof(name), NULL, NULL)) {
        // Failed to convert joystick name to UTF-8
        IDirectInputDevice8_Release(device);
        return DIENUM_STOP;
    }

    Gpad_Guid guid = "";

    // Generate a joystick GUID that matches the SDL 2.0.5+ one
    if(memcmp(&device_instance->guidProduct.Data4[2], "PIDVID", 6) == 0) {
        sprintf(
            guid,
            "03000000%02x%02x0000%02x%02x000000000000",
            (uint8_t)(device_instance->guidProduct.Data1 >> 0),
            (uint8_t)(device_instance->guidProduct.Data1 >> 8),
            (uint8_t)(device_instance->guidProduct.Data1 >> 16),
            (uint8_t)(device_instance->guidProduct.Data1 >> 24));
    } else {
        sprintf(
            guid,
            "05000000%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x00",
            name[0],
            name[1],
            name[2],
            name[3],
            name[4],
            name[5],
            name[6],
            name[7],
            name[8],
            name[9],
            name[10]);
    }

    Gpad_Device_Id id = GPAD_ID_INVALID;

    for(int i = 0; i < GPAD_MAX_DEVICES; i++) {
        if(strncmp(guid, &gpad__context.id_entries[i].guid[0], sizeof(Gpad_Guid)) == 0) {
            id = i;
        }
    }

    if(id == GPAD_ID_INVALID) {
        id = gpad__find_unused_device_id();
    }

    if(id < GPAD_MAX_DEVICES) {
        Gpad_Device_Entry entry = {0};
        entry.mapping_index = gpad__find_mapping(&guid[0]);
        entry.kind = Gpad_Device_Entry_Kind_DInput8;
        entry.dinput8_device = device;

        // {
        //     Gpad_Mapping mapping = g_gpad_mappings[entry.mapping_index];
        //     printf("MAPPINGS:\n");
        //     printf("\tname: %s\n", mapping.name);
        //     printf("\tbuttons:\n");
        //     for(int i = 0; i < Gpad_Button_COUNT; i++) {
        //         Gpad_Mapping_Elem elem = mapping.buttons[i];
        //         printf(
        //             "\t\traw %i (%s) =>\t index %i (%s), kind: %s, scale: %i, offs: %i\n",
        //             i,
        //             gpad_button_name(i),
        //             elem.index,
        //             gpad_button_name(elem.index),
        //             gpad__mapping_kind_name(elem.kind),
        //             elem.axis_scale,
        //             elem.axis_offset);
        //     }

        //     printf("\taxes:\n");
        //     for(int i = 0; i < Gpad_Axis_COUNT; i++) {
        //         Gpad_Mapping_Elem elem = mapping.axes[i];
        //         printf(
        //             "\t\traw %i (%s) =>\t index %i (%s), kind: %s, scale: %i, offs: %i\n",
        //             i,
        //             gpad_axis_name(i),
        //             elem.index,
        //             gpad_axis_name(elem.index),
        //             gpad__mapping_kind_name(elem.kind),
        //             elem.axis_scale,
        //             elem.axis_offset);
        //     }
        // }

        gpad__context.id_entries[id] = entry;

        return DIENUM_CONTINUE;
    }

    return DIENUM_STOP;
}

bool gpad_initialize(void) {
    if(gpad__context.initialized) return false;

    gpad__context.instance = GetModuleHandle(0);

    gpad__clear_device_entries();

    if(DirectInput8Create(
           gpad__context.instance,
           DIRECTINPUT_VERSION,
#ifdef __cplusplus
           IID_IDirectInput8,
#else
           &IID_IDirectInput8,
#endif
           (void**)&gpad__context.dinput8,
           0) != DI_OK) {
        return false;
    }

    gpad_refresh_connected_devices();

    gpad__context.initialized = true;
    return true;
}

void gpad_shutdown(void) {
    if(!gpad__context.initialized)
        ;

    gpad__context.initialized = false;
}

bool gpad_is_initialized(void) {
    return gpad__context.initialized;
}

bool gpad_device_valid(Gpad_Device_Id id) {
    if(!gpad__context.initialized) return false;
    Gpad_Device_Entry entry = gpad__context.id_entries[id];
    return entry.mapping_index < GPAD__NUM_MAPPINGS;
}

void gpad_refresh_connected_devices(void) {
    gpad__clear_device_entries();

    // Ignore failure
    IDirectInput8_EnumDevices(
        gpad__context.dinput8, DI8DEVCLASS_GAMECTRL, gpad__dinput_enum_devices_callback, 0, DIEDFL_ALLDEVICES);

    for(DWORD i = 0; i < XUSER_MAX_COUNT; i++) {
        XINPUT_STATE state = {0};
        if(XInputGetState(i, &state) == ERROR_SUCCESS) {
            // TODO: util func?
            Gpad_Device_Id id = gpad__find_unused_device_id();
            if(id < GPAD_MAX_DEVICES) {
                Gpad_Device_Entry entry = {0};
                // Zero is a valid mapping, but the mapping is never used with XInput.
                entry.mapping_index = 0;
                entry.kind = Gpad_Device_Entry_Kind_XInput;
                entry.xinput_user_index = i;
                gpad__context.id_entries[id] = entry;
            }
        }
    }
}

int gpad_list_devices(Gpad_Device_Id* ids, const int ids_max) {
    if(!gpad__context.initialized) return 0;
    int num = 0;
    for(int id = 0; id < GPAD_MAX_DEVICES; id++) {
        if(gpad__context.id_entries[id].mapping_index < GPAD__NUM_MAPPINGS) {
            if(num >= ids_max) {
                break;
            }
            ids[num] = id;
            num++;
        }
    }
    return num;
}

bool gpad_poll_device(const Gpad_Device_Id id, Gpad_Device_State* out_state) {
    if(!gpad__context.initialized) return false;
    if(out_state == 0) return false;

    Gpad_Device_Entry entry = gpad__context.id_entries[id];

    if(entry.mapping_index >= GPAD__NUM_MAPPINGS) return false;

    switch(entry.kind) {
        case Gpad_Device_Entry_Kind_DInput8: {
            IDirectInputDevice8* device = entry.dinput8_device;
            if(device) {
                Gpad_Mapping mapping = g_gpad_mappings[entry.mapping_index];
                DIJOYSTATE state;

                IDirectInputDevice8_Poll(device);

                HRESULT error_code = IDirectInputDevice8_GetDeviceState(device, sizeof(state), &state);

                if(error_code == DIERR_NOTACQUIRED || error_code == DIERR_INPUTLOST) {
                    IDirectInputDevice8_Poll(device);
                    IDirectInputDevice8_Acquire(device);
                    error_code = IDirectInputDevice8_GetDeviceState(device, sizeof(state), &state);
                }

                if(error_code != DI_OK) {
                    // Failed
                    gpad__remove_device_entry(id);
                    return false;
                }

                DIDEVCAPS caps = {sizeof(DIDEVCAPS)};
                if(IDirectInputDevice8_GetCapabilities(device, &caps) == DI_OK) {
                    // printf("X:%5d ", state.lX);
                    // printf("Y:%5d ", state.lY);
                    // printf("Z:%5d ", state.lZ);
                    // printf("Rx:%5d ", state.lRx);
                    // printf("Ry:%5d ", state.lRy);
                    // printf("Rz:%5d ", state.lRz);
                    // printf("Slider0:%5d ", state.rglSlider[0]);
                    // printf("Slider1:%5d ", state.rglSlider[1]);
                    // printf("Buttons: ");
                    // for(unsigned int buttonIndex = 0; buttonIndex < caps.dwButtons; ++buttonIndex) {
                    //     if(state.rgbButtons[buttonIndex]) {
                    //         printf("%d ", buttonIndex);
                    //     }
                    // }
                    // printf("\n");

                    Gpad_Device_State result;
                    gpad__init_state(&result);

                    const LONG axes[Gpad_Axis_COUNT] = {
                        state.lX,  // Gpad_Axis_Left_X
                        state.lY,  // Gpad_Axis_Left_Y
                        state.lRx, // Gpad_Axis_Right_X
                        state.lRy, // Gpad_Axis_Right_Y
                        state.lZ,  // Gpad_Axis_Left_Trigger
                        state.lRz, // Gpad_Axis_Right_Trigger
                    };

                    for(int i = 0; i < Gpad_Axis_COUNT; i++) {
                        // printf("%i ", (int)axes[i]);
                    }

                    uint8_t hats[4] = {0};

                    for(int i = 0; i < caps.dwPOVs; i++) {
                        uint32_t pov_index = state.rgdwPOV[i] / 4500;
                        if(pov_index < 8) {
                            static const uint8_t states[8] = {
                                1,     // Up
                                2 | 1, // Right Up
                                2,     // Right
                                2 | 4, // Right Down
                                4,     // Down
                                8 | 4, // Left Down
                                8,     // Left
                                8 | 1, // Left Up
                            };
                            hats[i] = states[pov_index];
                        }
                    }

                    for(int i = 0; i < Gpad_Button_COUNT; i++) {
                        Gpad_Mapping_Elem elem = mapping.buttons[i];
                        switch(elem.kind) {
                            case Gpad_Mapping_Kind_Axis: {
                                if(elem.axis_offset == 0 && elem.axis_scale == 0) continue;
                                if(elem.index >= Gpad_Axis_COUNT) continue;

                                const float fvalue = (((float)axes[elem.index] - 32768.0f) + 0.5f) / 32767.5f;

                                // HACK: This should be baked into the value transform
                                if(elem.axis_offset < 0 || (elem.axis_offset == 0 && elem.axis_scale > 0)) {
                                    if(fvalue >= 0.0f) {
                                        result.buttons |= 1 << i;
                                    }
                                } else {
                                    if(fvalue <= 0.0f) {
                                        result.buttons |= 1 << i;
                                    }
                                }
                            } break;

                            case Gpad_Mapping_Kind_Hatbit: {
                                const unsigned int hat = elem.index >> 4;
                                const unsigned int bit = elem.index & 0xf;
                                if(hat < sizeof(hats) / sizeof(hats[0])) {
                                    if(hats[hat] & bit) {
                                        result.buttons |= 1 << i;
                                    }
                                }
                            } break;

                            case Gpad_Mapping_Kind_Button: {
                                if(state.rgbButtons[elem.index] & 0x80) {
                                    result.buttons |= 1 << i;
                                }
                            } break;
                        }
                    }

                    for(int i = 0; i < Gpad_Axis_COUNT; i++) {
                        Gpad_Mapping_Elem elem = mapping.axes[i];
                        switch(elem.kind) {
                            case Gpad_Mapping_Kind_Axis: {
                                if(elem.axis_offset == 0 && elem.axis_scale == 0) continue;
                                if(elem.index >= Gpad_Axis_COUNT) continue;

                                result.axes[i] = (((float)axes[elem.index] - 32768.0f) + 0.5f) / 32767.5f;
                                if(i == Gpad_Axis_Left_Y || i == Gpad_Axis_Right_Y) {
                                    result.axes[i] *= -1.0f;
                                }
                            } break;

                            case Gpad_Mapping_Kind_Hatbit: {
                                const unsigned int hat = elem.index >> 4;
                                const unsigned int bit = elem.index & 0xf;
                                float val = -1.0f;
                                if(hat < sizeof(hats) / sizeof(hats[0])) {
                                    if(hats[hat] & bit) {
                                        val = 1.0f;
                                    }
                                }
                                result.axes[i] = val;
                            } break;

                            case Gpad_Mapping_Kind_Button: {
                                result.axes[i] = state.rgbButtons[elem.index] & 0x80 ? GPAD_AXIS_MAX : GPAD_AXIS_MIN;
                            } break;
                        }
                    }

                    *out_state = result;
                    return true;
                }
            }
        } break;



        case Gpad_Device_Entry_Kind_XInput: {
            if(entry.xinput_user_index < XUSER_MAX_COUNT) {
                XINPUT_STATE state = {0};
                DWORD error_code = XInputGetState(entry.xinput_user_index, &state);

                if(error_code != ERROR_SUCCESS) {
                    if(error_code == ERROR_DEVICE_NOT_CONNECTED) {
                        gpad__remove_device_entry(id);
                    }
                    return false;
                }

                Gpad_Device_State result;
                gpad__init_state(&result);

                if(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) {
                    result.buttons |= 1 << Gpad_Button_Dpad_Up;
                }
                if(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) {
                    result.buttons |= 1 << Gpad_Button_Dpad_Down;
                }
                if(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) {
                    result.buttons |= 1 << Gpad_Button_Dpad_Left;
                }
                if(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) {
                    result.buttons |= 1 << Gpad_Button_Dpad_Right;
                }
                if(state.Gamepad.wButtons & XINPUT_GAMEPAD_START) {
                    result.buttons |= 1 << Gpad_Button_Start;
                }
                if(state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) {
                    result.buttons |= 1 << Gpad_Button_Back;
                }
                if(state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) {
                    result.buttons |= 1 << Gpad_Button_Left_Thumb;
                }
                if(state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) {
                    result.buttons |= 1 << Gpad_Button_Right_Thumb;
                }
                if(state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) {
                    result.buttons |= 1 << Gpad_Button_Left_Shoulder;
                }
                if(state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) {
                    result.buttons |= 1 << Gpad_Button_Right_Shoulder;
                }
                if(state.Gamepad.wButtons & XINPUT_GAMEPAD_A) {
                    result.buttons |= 1 << Gpad_Button_A;
                }
                if(state.Gamepad.wButtons & XINPUT_GAMEPAD_B) {
                    result.buttons |= 1 << Gpad_Button_B;
                }
                if(state.Gamepad.wButtons & XINPUT_GAMEPAD_X) {
                    result.buttons |= 1 << Gpad_Button_X;
                }
                if(state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) {
                    result.buttons |= 1 << Gpad_Button_Y;
                }

                result.axes[Gpad_Axis_Left_X] = (state.Gamepad.sThumbLX + 0.5f) / 32767.5f;
                result.axes[Gpad_Axis_Left_Y] = (state.Gamepad.sThumbLY + 0.5f) / 32767.5f;
                result.axes[Gpad_Axis_Right_X] = (state.Gamepad.sThumbRX + 0.5f) / 32767.5f;
                result.axes[Gpad_Axis_Right_Y] = (state.Gamepad.sThumbRY + 0.5f) / 32767.5f;
                result.axes[Gpad_Axis_Left_Trigger] = state.Gamepad.bLeftTrigger / 127.5f - 1.0f;
                result.axes[Gpad_Axis_Right_Trigger] = state.Gamepad.bRightTrigger / 127.5f - 1.0f;

                *out_state = result;
                return true;
            }
        } break;
    }

    return false;
}

bool gpad_rumble_device(const Gpad_Device_Id id, float low_frequency, float high_frequency) {
    if(!gpad__context.initialized) return false;

    Gpad_Device_Entry entry = gpad__context.id_entries[id];

    if(entry.mapping_index >= GPAD__NUM_MAPPINGS) return false;

    // Clamp the inputs
    if(low_frequency < 0.0f) low_frequency = 0.0f;
    if(low_frequency > 1.0f) low_frequency = 1.0f;
    if(high_frequency < 0.0f) high_frequency = 0.0f;
    if(high_frequency > 1.0f) high_frequency = 1.0f;

    switch(entry.kind) {
        case Gpad_Device_Entry_Kind_DInput8: {
            // TODO
        } break;

        case Gpad_Device_Entry_Kind_XInput: {
            if(entry.xinput_user_index >= 4) {
                break;
            }
            XINPUT_VIBRATION state = {0};
            state.wLeftMotorSpeed = (WORD)(low_frequency * 65535.0f);
            state.wRightMotorSpeed = (WORD)(high_frequency * 65535.0f);
            if(XInputSetState(entry.xinput_user_index, &state) == ERROR_SUCCESS) {
                return true;
            }
        } break;
    }

    return false;
}
#endif // __WIN32