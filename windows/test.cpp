/*
 
test.cpp ... Test functionality of gamepad.dll.

Copyright (c) 2011  aura Human Technology Ltd.  <rnd@auraht.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, 
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.
* Neither the name of "aura Human Technology Ltd." nor the names of its
  contributors may be used to endorse or promote products derived from this
  software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "GamepadChangedObserver.hpp"
#include "Gamepad.hpp"
#include <Windows.h>
#include <tchar.h>
#include <algorithm>

void gamepad_axis_state_changed(GP::Gamepad& gamepad, GP::Axis axis, GP::AxisState state) {
    if (axis != GP::Axis::Z)
        printf("Gamepad %p: Axis %s %s.\n", &gamepad, GP::name<char>(axis), state == GP::AxisState::start_moving ? "start moving" : "stop moving");
}

void gamepad_axis_changed(GP::Gamepad& gamepad, GP::Axis axis, long new_value, unsigned nse) {
    if (axis != GP::Axis::Z)
        printf("Gamepad %p: Axis %s changed to %ld (%uns).\n", &gamepad, GP::name<char>(axis), new_value, nse);
}

void gamepad_axis_group_state_changed(GP::Gamepad& gamepad, GP::AxisGroup ag, GP::AxisState state) {
    if (ag != GP::AxisGroup::rigid_motion)
        printf("Gamepad %p: Axis group %s %s.\n", &gamepad, GP::name<char>(ag), state == GP::AxisState::start_moving ? "start moving" : "stop moving");
}

void gamepad_axis_group_changed(GP::Gamepad& gamepad, GP::AxisGroup ag, long new_values[], unsigned nse) {
    if (ag != GP::AxisGroup::rigid_motion)
        printf("Gamepad %p: Axis group %s changed to <%ld %ld %ld> (%uns).\n", &gamepad, GP::name<char>(ag), new_values[0], new_values[1], new_values[2], nse);
}


void gamepad_button_changed(GP::Gamepad& gamepad, GP::Button button, bool is_pressed) {
    printf("Gamepad %p: Button %d is %s.\n", &gamepad, button, is_pressed ? "down" : "up");
}


void gamepad_state_changed(void* self, const GP::GamepadChangedObserver& observer, GP::Gamepad& gamepad, GP::GamepadState state) {
    if (state == GP::GamepadState::detaching) {
        printf("Gamepad %p is detached.\n", &gamepad);
        
    } else {
        printf("Gamepad %p is attached.\n", &gamepad);
        
        std::for_each(observer.cbegin(), observer.cend(), [](GP::Gamepad& gamepad) {
            printf(" - Gamepad %p\n", &gamepad);
        });
        
        gamepad.set_axis_changed_callback(gamepad_axis_changed);
        gamepad.set_axis_state_changed_callback(gamepad_axis_state_changed);
        gamepad.set_axis_group_changed_callback(gamepad_axis_group_changed);
        gamepad.set_axis_group_state_changed_callback(gamepad_axis_group_state_changed);
        gamepad.set_button_changed_callback(gamepad_button_changed);
    }
}


//-----------------------------------------------------------------------------

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
        break;
    }

    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    struct A {
        FILE* stream;
        A() {
            AllocConsole();
            freopen_s(&stream, "CONOUT$", "wb", stdout);
        }
        ~A() {
            fclose(stream);
            FreeConsole();
        }
    } a;

    const TCHAR window_class[] = _T("test_gamepad");
    const TCHAR window_title[] = _T("Test Gamepad");

    WNDCLASSEX clsdef;
    clsdef.cbSize = sizeof(clsdef);
    clsdef.style = CS_HREDRAW | CS_VREDRAW;
    clsdef.lpfnWndProc = WndProc;
    clsdef.cbClsExtra = 0;
    clsdef.cbWndExtra = 0;
    clsdef.hInstance = hInstance;
    clsdef.hIcon = NULL;
    clsdef.hCursor = NULL;
    clsdef.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    clsdef.lpszMenuName = NULL;
    clsdef.lpszClassName = window_class;
    clsdef.hIconSm = NULL;

    if (!RegisterClassEx(&clsdef)) {
        MessageBox(NULL, _T("Cannot RegisterClassEx"), window_title, NULL);
        return 1;
    }

    HWND hwnd = CreateWindow(
        window_class, window_title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        500, 100, NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        MessageBox(NULL, _T("Cannot CreateWindow failed"), window_title, NULL);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    GP::GamepadChangedObserver observer (NULL, gamepad_state_changed, hwnd);

    MSG message;
    while (GetMessage(&message, NULL, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return message.wParam;
}
