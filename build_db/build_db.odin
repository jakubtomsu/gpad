package gpad_build_db

import "core:os"
import "core:strings"
import "core:fmt"
import "core:strconv"

main :: proc() {
    context.allocator = context.temp_allocator

    Platform :: enum u8 {
        windows,
        ios,
    }

    Mapping :: struct {
        name:    string,
        guid:    string,
        buttons: [Button]Element,
        axes:    [Axis]Element,
    }

    Kind :: enum u8 {
        axis,
        button,
        hatbit,
    }

    Element :: struct {
        kind:        Kind,
        index:       u8,
        axis_scale:  i8,
        axis_offset: i8,
    }

    Button :: enum u8 {
        a            = 0,
        b            = 1,
        x            = 2,
        y            = 3,
        left_bumper  = 4,
        right_bumper = 5,
        back         = 6,
        start        = 7,
        guide        = 8,
        left_thumb   = 9,
        right_thumb  = 10,
        dpad_up      = 11,
        dpad_right   = 12,
        dpad_down    = 13,
        dpad_left    = 14,
    }

    Axis :: enum u8 {
        left_x        = 0,
        left_y        = 1,
        right_x       = 2,
        right_y       = 3,
        left_trigger  = 4,
        right_trigger = 5,
    }

    if data, data_ok := os.read_entire_file("gamecontrollerdb.txt", context.temp_allocator); data_ok {
        platforms: map[string][dynamic]Mapping

        text := string(data)

        for line in strings.split_lines_iterator(&text) {
            if len(line) == 0 do continue
            if line[0] == '#' do continue

            mapping: Mapping
            platform: string

            items := strings.split(line, ",")
            mapping.name = items[1]
            mapping.guid = items[0]

            for pair in items[2:] {
                p := strings.split(pair, ":")
                if len(p) != 2 do continue
                ident := p[0]
                switch ident[0] {
                case '+', '-':
                    // TODO: implement output modifiers
                    continue
                }

                if ident == "platform" {
                    platform = p[1]
                    break
                }

                elem: Element
                minimum := -1
                maximum := 1

                elem_char: int = 0
                switch p[1][elem_char] {
                case '+':
                    minimum = 0
                    elem_char += 1
                case '-':
                    maximum = 0
                    elem_char += 1
                }

                switch p[1][elem_char] {
                case 'a':
                    elem.kind = .axis
                case 'b':
                    elem.kind = .button
                case 'h':
                    elem.kind = .hatbit
                }

                elem_char += 1

                invert := p[1][len(p[1]) - 1] == '~'

                elem_val := p[1][elem_char:]
                if invert {
                    elem_val = elem_val[:len(elem_val) - 1]
                }

                if elem.kind == .hatbit {
                    split := strings.split(elem_val, ".")
                    hat := strconv.atoi(split[0])
                    bit := strconv.atoi(split[1])
                    elem.index = u8((hat << 4) | bit)
                } else {
                    elem.index = u8(strconv.atoi(elem_val))
                }

                if elem.kind == .axis {
                    elem.axis_scale = i8(2 / (maximum - minimum))
                    elem.axis_offset = -i8(maximum + minimum)
                    if invert {
                        elem.axis_scale *= -1
                        elem.axis_offset *= -1
                    }
                }

                switch ident {
                            // odinfmt: disable
                case "a":                mapping.buttons[.a] = elem
                case "b":                mapping.buttons[.b] = elem
                case "x":                mapping.buttons[.x] = elem
                case "y":                mapping.buttons[.y] = elem
                case "back":             mapping.buttons[.back] = elem
                case "start":            mapping.buttons[.start] = elem
                case "guide":            mapping.buttons[.guide] = elem
                case "leftshoulder":     mapping.buttons[.left_bumper] = elem
                case "rightshoulder":    mapping.buttons[.right_bumper] = elem
                case "leftstick":        mapping.buttons[.left_thumb] = elem
                case "rightstick":       mapping.buttons[.right_thumb] = elem
                case "dpup":             mapping.buttons[.dpad_up] = elem
                case "dpright":          mapping.buttons[.dpad_right] = elem
                case "dpdown":           mapping.buttons[.dpad_down] = elem
                case "dpleft":           mapping.buttons[.dpad_left] = elem
                case "lefttrigger":      mapping.axes[.left_trigger] = elem
                case "righttrigger":     mapping.axes[.right_trigger] = elem
                case "leftx":            mapping.axes[.left_x] = elem
                case "lefty":            mapping.axes[.left_y] = elem
                case "rightx":           mapping.axes[.right_x] = elem
                case "righty":           mapping.axes[.right_y] = elem
                // odinfmt: enable
                }
            }

            assert(platform != "")

            if platform not_in platforms {
                platforms[platform] = nil
            }

            append(&platforms[platform], mapping)
        }

        b: strings.Builder
        strings.builder_init(&b, context.temp_allocator)

        strings.write_string(&b, "// WARNING: Machine generated! Do not edit!\n")

        write_elem :: proc(b: ^strings.Builder, elem: Element) {
            strings.write_string(b, "{")

            strings.write_int(b, int(elem.kind))
            strings.write_string(b, ", ")

            strings.write_int(b, int(elem.index))
            strings.write_string(b, ", ")

            strings.write_int(b, int(elem.axis_scale))
            strings.write_string(b, ", ")

            strings.write_int(b, int(elem.axis_offset))

            strings.write_string(b, "},\n")
        }

        ignore_platform :: proc(name: string) -> bool {
            switch name {
            case "Android", "iOS":
                return true
            }
            return false
        }

        platform_macro_name :: proc(name: string) -> string {
            switch name {
            case "Android":
                return "ANDROID"
            case "iOS":
                return "IOS"
            case "Windows":
                return "WINDOWS"
            case "Mac OS X":
                return "MAC"
            case "Linux":
                return "LINUX"
            }
            return ""
        }

        strings.write_string(&b, "static const Gpad_Guid g_gpad_mapping_guids[] = {\n")
        for platform, mappings in platforms {
            if ignore_platform(platform) do continue

            strings.write_string(&b, "#ifdef GPAD_PLATFORM_")
            strings.write_string(&b, platform_macro_name(platform))
            strings.write_string(&b, "\n")

            for m in mappings {
                strings.write_string(&b, "\"")
                strings.write_string(&b, m.guid)
                strings.write_string(&b, "\",\n")
            }

            strings.write_string(&b, "#endif // GPAD_PLATFORM_")
            strings.write_string(&b, platform_macro_name(platform))
            strings.write_string(&b, "\n\n\n")
        }
        strings.write_string(&b, "};\n\n\n\n\n")

        strings.write_string(&b, "static const Gpad_Mapping g_gpad_mappings[] = {\n")
        for platform, mappings in platforms {
            if ignore_platform(platform) do continue

            strings.write_string(&b, "#ifdef GPAD_PLATFORM_")
            strings.write_string(&b, platform_macro_name(platform))
            strings.write_string(&b, "\n")

            for m in mappings {
                strings.write_string(&b, "{\n")

                strings.write_string(&b, "\"")
                strings.write_string(&b, m.name)
                strings.write_string(&b, "\",\n")

                strings.write_string(&b, "{\n")
                for x in m.buttons {
                    write_elem(&b, x)
                }
                strings.write_string(&b, "},")

                strings.write_string(&b, "{\n")
                for x in m.axes {
                    write_elem(&b, x)
                }
                strings.write_string(&b, "},")

                strings.write_string(&b, "},\n")
            }

            strings.write_string(&b, "#endif // GPAD_PLATFORM_")
            strings.write_string(&b, platform_macro_name(platform))
            strings.write_string(&b, "\n\n\n")
        }
        strings.write_string(&b, "};")

        for p in platforms {
            fmt.println(p)
        }

        fmt.println("FINISHED")

        if !os.write_entire_file("gpad_gamecontrollerdb.inl", b.buf[:]) {
            fmt.println("Couldn't write gpad_gamecontrollerdb.inl")
        }
    } else {
        fmt.println("Couldn't find gamecontrollerdb.txt")
    }
}
