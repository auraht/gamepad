/*
 
SimulatedGamepad_Windows.hpp ... Simulating a Gamepad using keyboard and mouse
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

#ifndef SIMULATED_GAMEPAD_WINDOWS_HPP_k3z8ak4qjrp2pgb9
#define SIMULATED_GAMEPAD_WINDOWS_HPP_k3z8ak4qjrp2pgb9 1

#include "../Gamepad.hpp"
#include <Windows.h>

namespace GP {
    class SimulatedGamepad_Windows : public Gamepad {
    private:
        HWND _hwnd;
        UINT_PTR _timer;
        POINT _last_point;
//      LARGE_INTEGER _perf_freq;
//      LARGE_INTEGER _last_perf_count;

        static void CALLBACK mouse_stop_timer(HWND hwnd, UINT msg, UINT_PTR id_event, DWORD timestamp);

        void destroy();
        
    public:
        ~SimulatedGamepad_Windows() { this->destroy(); }
        SimulatedGamepad_Windows(HWND hwnd);

        void handle_key_event(UINT keycode, bool is_pressed);
        void handle_mousemove_event(int x, int y);
    };
}

#endif
