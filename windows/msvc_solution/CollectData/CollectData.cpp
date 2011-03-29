#include <cstdio>
#include <GamepadChangedObserver.hpp>
#include <Gamepad.hpp>
#include <Windows.h>


void AxisGroupChangedCallback(GP::Gamepad& gamepad, GP::AxisGroup axis_group, long new_values[], unsigned nanoseconds_elapsed) {
    if (axis_group == GP::rigid_motion) {
        printf("%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%u\n", new_values[0], new_values[1], new_values[2], new_values[3], new_values[4], new_values[5], nanoseconds_elapsed);
    }
}

void GCOCallback(void* self, const GP::GamepadChangedObserver& observer, GP::Gamepad& gamepad, GP::GamepadState state) {
    if (state == GP::attached) {
        gamepad.set_axis_group_changed_callback(&AxisGroupChangedCallback);
    }
}

int main () {
    printf("X\tY\tZ\tRx\tRy\tRz\tdt\n");

    GP::GamepadChangedObserver gco (NULL, &GCOCallback, NULL);

    MSG message;
    while (GetMessage(&message, NULL, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return message.wParam;
}
