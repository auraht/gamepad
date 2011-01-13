/*
 
Gamepad_Windows.hpp ... Implementation of Gamepad for Windows.

Copyright (c) 2011  KennyTM~ <kennytm@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, 
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.
* Neither the name of the KennyTM~ nor the names of its contributors may be
  used to endorse or promote products derived from this software without
  specific prior written permission.

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

#ifndef GAMEPAD_WINDOWS_HPP_hq0p8ilfgiv34n29
#define GAMEPAD_WINDOWS_HPP_hq0p8ilfgiv34n29 1

#include "../Gamepad.hpp"
#include <Windows.h>
#include <vector>
#include <unordered_set>
#include "hidpi.h"

namespace GP {
    class Gamepad_Windows : public Gamepad {
    private:
        HANDLE _hdevice;
        std::vector<char> _preparsed_buf;
        PHIDP_PREPARSED_DATA _preparsed;

        ULONG _buttons_count;
        std::unordered_set<USAGE> _previous_active_buttons;
        std::vector<Axis> _axes;
        ULONG _previous_axis_values[kMaxAxisIndex];
        
    public:
        Gamepad_Windows(HANDLE hdevice);

        void handle_raw_input(const RAWHID& hid);
    };
}

#endif
