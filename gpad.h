#ifndef GPAD_H_INCLUDED
#define GPAD_H_INCLUDED

#include <stdint.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

// TODO:
// Recieve device connected/disconnected events from the OS.
//      windows: WM_DEVICECHANGE, RegisterDeviceNotification

#define GPAD_MAX_DEVICES 8

#define GPAD_AXIS_MAX 1.0f
#define GPAD_AXIS_MIN -1.0f
#define GPAD_ID_INVALID ((Gpad_Device_Id)~0)

typedef uint8_t Gpad_Button;
typedef uint8_t Gpad_Axis;
// Unique identifier for a device. Can be reused after a device is removed.
typedef uint8_t Gpad_Device_Id;

// PS4 mappings:
//      Cross:      A
//      Circle:     B
//      Square:     X
//      Triangle:   Y
typedef enum Gpad_Button_ {
    Gpad_Button_A = 0,
    Gpad_Button_B = 1,
    Gpad_Button_X = 2,
    Gpad_Button_Y = 3,
    Gpad_Button_Left_Shoulder = 4,
    Gpad_Button_Right_Shoulder = 5,
    Gpad_Button_Back = 6,
    Gpad_Button_Start = 7,
    Gpad_Button_Guide = 8,
    Gpad_Button_Left_Thumb = 9,
    Gpad_Button_Right_Thumb = 10,
    Gpad_Button_Dpad_Up = 11,
    Gpad_Button_Dpad_Right = 12,
    Gpad_Button_Dpad_Down = 13,
    Gpad_Button_Dpad_Left = 14,

    Gpad_Button_COUNT,
    Gpad_Button_LAST = Gpad_Button_Dpad_Left,
} Gpad_Button_;

// Left and Right tumbsticks are in range [-1.0, 1.0].
// Positive X is right and positive Y is up.
// Triggers are -1.0 when released.
typedef enum Gpad_Axis_ {
    Gpad_Axis_Left_X = 0,
    Gpad_Axis_Left_Y = 1,
    Gpad_Axis_Right_X = 2,
    Gpad_Axis_Right_Y = 3,
    Gpad_Axis_Left_Trigger = 4,
    Gpad_Axis_Right_Trigger = 5,
    Gpad_Axis_COUNT,
    Gpad_Axis_LAST = Gpad_Axis_Right_Trigger,
} Gpad_Axis_;

typedef struct Gpad_Device_State {
    uint16_t buttons;
    float axes[Gpad_Axis_COUNT];
} Gpad_Device_State;

#ifdef __cplusplus
extern "C" {
#endif

bool gpad_initialize(void);
void gpad_shutdown(void);
// Returns true if the context was correctly initialized.
bool gpad_is_initialized(void);
// Scan for all the connected devices. This is a slow operation, don't call every frame!!!
void gpad_refresh_connected_devices(void);
// Poll the device state. This should probably be called once per frame for each used controller and cached.
bool gpad_poll_device(Gpad_Device_Id id, Gpad_Device_State* out_state);
// Vibrate the controller. On Xbox, the left motor is low frequency and the high motor high frequency.
bool gpad_rumble_device(Gpad_Device_Id id, float low_frequency, float high_frequency);
bool gpad_device_valid(Gpad_Device_Id id);

//
// Utilities
//

const char* gpad_button_name(Gpad_Button button);
const char* gpad_axis_name(Gpad_Axis axis);
// Get a list of all valid device IDs. Returns number of devices written.
int gpad_list_devices(Gpad_Device_Id* ids, int ids_max);
// Utility for checking if a button is pressed.
bool gpad_device_button_pressed(const Gpad_Device_State* state, Gpad_Button button);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // GPAD_H_INCLUDED