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

const int display_text_size = 1024;
static TCHAR display_text[display_text_size];

static int update_event_number() {
    static int event_number = 0;
    event_number ++;
    if (event_number >= 100)
        event_number = 0;
    return event_number;
}

void gamepad_axis_changed(void* hwnd, GP::Gamepad* gamepad, GP::Gamepad::Axis axis, long new_value) {
    _sntprintf(display_text, display_text_size, _T("[%02d] Gamepad %p: Axis %s changed to %ld."), update_event_number(), gamepad, GP::Gamepad::axis_name<TCHAR>(axis), new_value);

    RedrawWindow(static_cast<HWND>(hwnd), NULL, NULL, RDW_INVALIDATE|RDW_ERASE);
}

void gamepad_button_changed(void* hwnd, GP::Gamepad* gamepad, int button, bool is_pressed) {
    _sntprintf(display_text, display_text_size, _T("[%02d] Gamepad %p: Button %d is %s."), update_event_number(), gamepad,button, is_pressed ? _T("down") : _T("up"));

    RedrawWindow(static_cast<HWND>(hwnd), NULL, NULL, RDW_INVALIDATE|RDW_ERASE);
}


void gamepad_state_changed(void* hwnd, GP::Gamepad* gamepad, GP::GamepadChangedObserver::State state) {
    if (state == GP::GamepadChangedObserver::kDetaching) {
        _sntprintf(display_text, display_text_size, _T("[%02d] Gamepad %p is detached."), update_event_number(), gamepad);
    } else {
        _sntprintf(display_text, display_text_size, _T("[%02d] Gamepad %p is attached."), update_event_number(), gamepad);
        
        gamepad->set_axis_changed_callback(hwnd, gamepad_axis_changed);
        gamepad->set_button_changed_callback(hwnd, gamepad_button_changed);
    }

    RedrawWindow(static_cast<HWND>(hwnd), NULL, NULL, RDW_INVALIDATE|RDW_ERASE);
}

//-----------------------------------------------------------------------------

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_PAINT: 
        {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        TextOut(hdc, 0, 0, display_text, _tcslen(display_text));
        EndPaint(hwnd, &ps);
        break;
        }
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

    GP::GamepadChangedObserver* observer = GP::GamepadChangedObserver::create(hwnd, gamepad_state_changed, hwnd);

    MSG message;
    while (GetMessage(&message, NULL, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    delete observer;

    return message.wParam;
}
