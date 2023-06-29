@echo off

cl /c gpad_windows.c /I c\include /Fogpad.obj /Oi /MT /Zi /D_DEBUG /DEBUG
lib /OUT:gpad_windows_x64_debug.lib gpad.obj

cl /c gpad_windows.c /I c\include /Fogpad.obj /Oi /O2
lib /OUT:gpad_windows_x64_release.lib gpad.obj

del gpad.obj
