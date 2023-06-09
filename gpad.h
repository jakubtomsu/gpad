#ifndef GPAD_H_INCLUDED
#define GPAD_H_INCLUDED

#include <stdint.h>

#define GPAD_MAX_DEVICES 4

typedef uint8_t Gpad_Button;
typedef uint8_t Gpad_Axis;
typedef uint8_t Gpad_Id;
typedef int Gpad_Result;

typedef enum Gpad_Button_ {
    Gpad_Button_A               = 0,
    Gpad_Button_B               = 1,
    Gpad_Button_X               = 2,
    Gpad_Button_Y               = 3,
    Gpad_Button_Left_Bumper     = 4,
    Gpad_Button_Right_Bumper    = 5,
    Gpad_Button_Back            = 6,
    Gpad_Button_Start           = 7,
    Gpad_Button_Guide           = 8,
    Gpad_Button_Left_Thumb      = 9,
    Gpad_Button_Right_Thumb     = 10,
    Gpad_Button_Dpad_Up         = 11,
    Gpad_Button_Dpad_Right      = 12,
    Gpad_Button_Dpad_Down       = 13,
    Gpad_Button_Dpad_Left       = 14,
    Gpad_Button_COUNT,
    Gpad_Button_LAST            = Gpad_Button_Dpad_Left,
    Gpad_Button_Cross           = Gpad_Button_A,
    Gpad_Button_Circle          = Gpad_Button_B,
    Gpad_Button_Square          = Gpad_Button_X,
    Gpad_Button_Triangle        = Gpad_Button_Y,
} Gpad_Button_;

typedef enum Gpad_Axis_ {
    Gpad_Axis_Left_X        = 0,
    Gpad_Axis_Left_Y        = 1,
    Gpad_Axis_Right_X       = 2,
    Gpad_Axis_Right_Y       = 3,
    Gpad_Axis_Left_Trigger  = 4,
    Gpad_Axis_Right_Trigger = 5,
    Gpad_Axis_COUNT,
    Gpad_Axis_LAST          = Gpad_Axis_Right_Trigger,
} Gpad_Axis_;

typedef struct Gpad_Controller {
    uint8_t buttons[Gpad_Button_COUNT];
    float axes[Gpad_Axis_COUNT];
} Gpad_Controller;

#if __cplusplus
extern "C" {
#endif

void gpad_initialize(void);
void gpad_shutdown(void);

const char* gpad_name(Gpad_Id id);
bool gpad_controller(Gpad_Id id, Gpad_Controller* result);

#if __cplusplus
} // extern "C"
#endif

#endif // GPAD_H_INCLUDED
#if _WIN32
#define DIRECTINPUT_VERSION 0x0800

#include <Windows.h>
#include <dinput.h>
#include <Xinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

typedef struct Gpad_Context {
    bool initialized;
    HINSTANCE instance;
    IDirectInput8* dinput;
    IDirectInputDevice8* devices[GPAD_MAX_DEVICES];
    int num_devices;
} Gpad_Context;

static Gpad_Context g_gpad_context = {};

BOOL CALLBACK gpad_dinput_enum_devices_callback(LPCDIDEVICEINSTANCE instance, LPVOID userData) {
	return DIENUM_CONTINUE;
}

#include <stdio.h>

void gpad_initialize(void) {
    g_gpad_context.instance = GetModuleHandle(0);

    if (DirectInput8Create(
        g_gpad_context.instance,
        DIRECTINPUT_VERSION,
        IID_IDirectInput8,
        (void**)&g_gpad_context.dinput,
        0) != DI_OK) return;

    printf("Created\n");

    if (g_gpad_context.dinput->EnumDevices(
        DI8DEVCLASS_GAMECTRL,
        gpad_dinput_enum_devices_callback,
        0,
        DIEDFL_ALLDEVICES) != DI_OK) return;

    printf("Enumed\n");

    printf("Num devices: %i\n", g_gpad_context.num_devices);

    for (unsigned int joystickIndex=0; joystickIndex < g_gpad_context.num_devices; ++joystickIndex)
		{
			DIJOYSTATE state;
			if (g_gpad_context.devices[joystickIndex]->GetDeviceState(sizeof(state), &state) == DI_OK)
			{
				DIDEVCAPS caps = {sizeof(DIDEVCAPS)};
				g_gpad_context.devices[joystickIndex]->GetCapabilities(&caps);

				printf("Joystick %d ", joystickIndex);
				printf("X:%5d ", state.lX);
				printf("Y:%5d ", state.lY);
				printf("Z:%5d ", state.lZ);
				printf("Rx:%5d ", state.lRx);
				printf("Ry:%5d ", state.lRy);
				printf("Rz:%5d ", state.lRz);
				printf("Slider0:%5d ", state.rglSlider[0]);
				printf("Slider1:%5d ", state.rglSlider[1]);

				printf("Hat:%5d ", state.rgdwPOV[0]);

				printf("Buttons: ");
				for (unsigned int buttonIndex = 0; buttonIndex < caps.dwButtons; ++buttonIndex) {
					if (state.rgbButtons[buttonIndex]) {
						printf("%d ", buttonIndex);
					}
				}

				printf("\n");
			}
		}

    for (int i = 0; i < XUSER_MAX_COUNT; i++) {
        XINPUT_STATE state = {0};
        if (XInputGetState(i, &state) == ERROR_SUCCESS) {
            printf("Player %d ", i);
				printf("LX:%6d ", state.Gamepad.sThumbLX);
				printf("LY:%6d ", state.Gamepad.sThumbLY);
				printf("RX:%6d ", state.Gamepad.sThumbRX);
				printf("RY:%6d ", state.Gamepad.sThumbRY);
				printf("LT:%3u ", state.Gamepad.bLeftTrigger);
				printf("RT:%3u ", state.Gamepad.bRightTrigger);
				printf("Buttons: ");
				if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)        printf("up ");
				if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)      printf("down ");
				if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)      printf("left ");
				if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)     printf("right ");
				if (state.Gamepad.wButtons & XINPUT_GAMEPAD_START)          printf("start ");
				if (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)           printf("back ");
				if (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)     printf("LS ");
				if (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)    printf("RS ");
				if (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)  printf("LB ");
				if (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) printf("RB ");
				if (state.Gamepad.wButtons & XINPUT_GAMEPAD_A)              printf("A ");
				if (state.Gamepad.wButtons & XINPUT_GAMEPAD_B)              printf("B ");
				if (state.Gamepad.wButtons & XINPUT_GAMEPAD_X)              printf("X ");
				if (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y)              printf("Y ");
				printf("\n");
        } else {

        }
    }
}

void gpad_shutdown(void) {

}
#endif

#ifdef GPAD_IMPLEMENTATION
#endif // GPAD_IMPLEMENTATION