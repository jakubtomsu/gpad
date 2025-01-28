package gpad_example_visual

import "core:time"
import "core:fmt"
import gpad "../.."
import rl "vendor:raylib"

// Warning: raylib uses +Y as *down*

main :: proc() {
    gpad.initialize()
    defer gpad.shutdown()

    rl.SetConfigFlags({.VSYNC_HINT, .MSAA_4X_HINT})
    rl.InitWindow(800, 400, "gpad")
    defer rl.CloseWindow()

    for !rl.WindowShouldClose() {
        rl.BeginDrawing()
        defer rl.EndDrawing()
        rl.ClearBackground({20, 20, 30, 255})
        
        devices := gpad.list_devices_slice()
        
        state := gpad.poll(devices[0])

        rl.DrawCircleV({100, 200}, 35, rl.DARKGRAY)
        rl.DrawCircleV({100, 200} + {state.axes[.Left_X], -state.axes[.Left_Y]} * 30, .Left_Thumb in state.buttons ? 15 : 30, rl.WHITE)
        rl.DrawText(fmt.ctprintf("L %.3f %.3f", state.axes[.Left_X], state.axes[.Left_Y]), 100, 270, 10, rl.SKYBLUE)
        
        rl.DrawCircleV({700, 200}, 35, rl.DARKGRAY)
        rl.DrawCircleV({700, 200} + {state.axes[.Right_X], -state.axes[.Right_Y]} * 30, .Right_Thumb in state.buttons ? 15 : 30, rl.WHITE)
        rl.DrawText(fmt.ctprintf("R %.3f %.3f", state.axes[.Right_X], state.axes[.Right_Y]), 700, 270, 10, rl.SKYBLUE)
        
        rl.DrawCircleV({200, 50}, 25, rl.DARKGRAY)
        rl.DrawCircleV({200, 50} + {0, state.axes[.Left_Trigger]} * 20, 20, rl.WHITE)
        rl.DrawText(fmt.ctprintf("LT %.3f", state.axes[.Left_Trigger]), 200, 100, 10, rl.SKYBLUE)
        
        rl.DrawCircleV({600, 50}, 25, rl.DARKGRAY)
        rl.DrawCircleV({600, 50} + {0, state.axes[.Right_Trigger]} * 20, 20, rl.WHITE)
        rl.DrawText(fmt.ctprintf("RT %.3f", state.axes[.Right_Trigger]), 600, 100, 10, rl.SKYBLUE)
        
        rl.DrawEllipse(100, 100, 25, 15, .Left_Shoulder in state.buttons ? rl.WHITE : rl.GRAY)
        rl.DrawEllipse(700, 100, 25, 15, .Right_Shoulder in state.buttons ? rl.WHITE : rl.GRAY)
        
        rl.DrawCircleV({400, 200}, 8, .Back  in state.buttons ? rl.WHITE : rl.GRAY); rl.DrawText("Back",  400, 200, 10, rl.SKYBLUE)
        rl.DrawCircleV({450, 200}, 8, .Start in state.buttons ? rl.WHITE : rl.GRAY); rl.DrawText("Start", 450, 200, 10, rl.SKYBLUE)
        rl.DrawCircleV({350, 200}, 8, .Guide in state.buttons ? rl.WHITE : rl.GRAY); rl.DrawText("Guide", 350, 200, 10, rl.SKYBLUE)
        
        rl.DrawCircleV({300, 300}, 30, rl.DARKGRAY)
        rl.DrawCircleV({300 - 30, 300 +  0}, 15, .Dpad_Left  in state.buttons ? rl.WHITE : rl.GRAY)
        rl.DrawCircleV({300 + 30, 300 +  0}, 15, .Dpad_Right in state.buttons ? rl.WHITE : rl.GRAY)
        rl.DrawCircleV({300 +  0, 300 + 30}, 15, .Dpad_Down  in state.buttons ? rl.WHITE : rl.GRAY)
        rl.DrawCircleV({300 +  0, 300 - 30}, 15, .Dpad_Up    in state.buttons ? rl.WHITE : rl.GRAY)
        
        rl.DrawCircleV({500, 300}, 30, rl.DARKGRAY)
        rl.DrawCircleV({500 - 30, 300 +  0}, 15, .X in state.buttons ? rl.BLUE : rl.GRAY)
        rl.DrawCircleV({500 + 30, 300 +  0}, 15, .B in state.buttons ? rl.RED : rl.GRAY)
        rl.DrawCircleV({500 +  0, 300 - 30}, 15, .Y in state.buttons ? rl.YELLOW : rl.GRAY)
        rl.DrawCircleV({500 +  0, 300 + 30}, 15, .A in state.buttons ? rl.GREEN : rl.GRAY)
        
        gpad.rumble_device(
            devices[0],
            state.axes[.Left_Trigger] * 0.5 + 0.5,
            state.axes[.Right_Trigger] * 0.5 + 0.5,
        )
    }
}
