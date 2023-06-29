package gpad_sample

import "core:time"
import "core:fmt"
import gpad ".."

main :: proc() {
    gpad.initialize()
    defer gpad.shutdown()

    for {
        for id: gpad.Device_Id = 0; id < gpad.MAX_DEVICES; id += 1 {
            if state, ok := gpad.poll(id); ok {
                for axis in gpad.Axis {
                    fmt.printf("%s: %f, ", gpad.axis_name(axis), state.axes[axis])
                }

                for button in gpad.Button {
                    if button in state.buttons {
                        fmt.printf("%s  ", gpad.button_name(button))
                    }
                }

                gpad.rumble_device(
                    id,
                    state.axes[.Left_Trigger] * 0.5 + 0.5,
                    state.axes[.Right_Trigger] * 0.5 + 0.5,
                )

                fmt.printf("\n")
            }
        }

        time.sleep(time.Millisecond * 100)
    }
}
