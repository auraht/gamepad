/*
 
GamepadEventloop_Darwin.hpp ... Implementation of GamepadEventloop for Darwin
                                (i.e. Mac OS X).

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

#ifndef GAMEPAD_DARWIN_HPP_3pw9suqkr442t9
#define GAMEPAD_DARWIN_HPP_3pw9suqkr442t9 1

#include "../Gamepad.hpp"
#include <IOKit/hid/IOHIDManager.h>
#include <unordered_map>

namespace GP {
    class Gamepad_Darwin : public Gamepad {
    private:
        IOHIDDeviceRef _device;
        std::unordered_map<int, IOHIDElementRef> _valid_output_elements, _valid_feature_elements;
        uint64_t _last_report_time;
        
        static void collect_axis_bounds(const void* element, void* self);
        
        static void handle_input_value(void* context, IOReturn result, void* sender, IOHIDValueRef value);
        static void handle_report(void* context, IOReturn, void*, IOHIDReportType, uint32_t, uint8_t*, CFIndex);
        
        bool commit_transaction(const Transaction& transaction);
        bool get_features(Transaction& transaction);
        
    public:
        Gamepad_Darwin(IOHIDDeviceRef device);
    };
}

#endif
