/*
 
SimulatedGamepad_Windows.cpp ... Simulating a Gamepad using keyboard and mouse
                                 for Windows.

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

#include "SimulatedGamepad_Windows.hpp"
#include "hidpi.h"
#include <vector>
#include <algorithm>

namespace GP {
    void SimulatedGamepad_Windows::destroy() {
       if (_timer) {
           KillTimer(_hwnd, _timer);
           _timer = 0;
       }
    }

    static const UINT TIMESTEP = 250;

    SimulatedGamepad_Windows::SimulatedGamepad_Windows(HWND hwnd) : _hwnd(hwnd) {
        this->set_bounds_for_axis(Axis::X, -5, 5);
        this->set_bounds_for_axis(Axis::Y, -5, 5);
        this->set_bounds_for_axis(Axis::Z, -1, 1);

        GetCursorPos(&_last_point);
        _perf_freq.QuadPart = 0;
        _last_perf_count.QuadPart = 0;
        QueryPerformanceFrequency(&_perf_freq);

        _timer = reinterpret_cast<UINT_PTR>(this);
    }

    void SimulatedGamepad_Windows::handle_mousemove_event(int x, int y) {
        SetTimer(_hwnd, _timer, TIMESTEP, &SimulatedGamepad_Windows::mouse_stop_timer);
        int delta_x = x - _last_point.x;
        int delta_y = y - _last_point.y;
        _last_point.x = x;
        _last_point.y = y;
        this->set_axis_value(Axis::X, delta_x);
        this->set_axis_value(Axis::Y, delta_y);
        LARGE_INTEGER last_count = _last_perf_count;
        LARGE_INTEGER cur_count;
        QueryPerformanceCounter(&cur_count);
        _last_perf_count = cur_count;
        if (last_count.QuadPart) {
            auto nanoseconds_elapsed = (cur_count.QuadPart - last_count.QuadPart) * 1000000000 / _perf_freq.QuadPart;
            this->handle_axes_change(nanoseconds_elapsed);
        }
    }

    void CALLBACK SimulatedGamepad_Windows::mouse_stop_timer(HWND hwnd, UINT msg, UINT_PTR id_event, DWORD timestamp) {
        auto this_ = reinterpret_cast<SimulatedGamepad_Windows*>(id_event);
        this_->_last_perf_count.QuadPart = 0;
        
        this_->set_axis_value(Axis::X, 0);
        this_->set_axis_value(Axis::Y, 0);
        this_->handle_axes_change(TIMESTEP * 1000000);
        
        KillTimer(hwnd, id_event);
    }

    void SimulatedGamepad_Windows::handle_key_event(UINT keycode, bool is_pressed) {
        Button translated_button;
        switch (keycode) {
            case 'W': translated_button = Button::_1; break;
            case 'E': translated_button = Button::_2; break;
            case 'A': translated_button = Button::_3; break;
            case 'D': translated_button = Button::_4; break;
            case 'Z': translated_button = Button::_5; break;
            case 'X': translated_button = Button::_6; break;
            case 'S': translated_button = Button::_7; break;
            case ' ': translated_button = Button::_8; break;
            default: return;
        }
        this->handle_button_change(translated_button, is_pressed);
    }
}
