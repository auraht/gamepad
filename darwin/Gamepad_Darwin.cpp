/*
 
GamepadEventloop_Darwin.cpp ... Implementation of GamepadEventloop for Darwin
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

#include "Gamepad_Darwin.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <tr1/unordered_map>
#include <algorithm>

namespace GP {
    void Gamepad_Darwin::collect_axis_bounds(const void* element_, void* self) {
        Gamepad_Darwin* gamepad = static_cast<Gamepad_Darwin*>(self);
        IOHIDElementRef element = static_cast<IOHIDElementRef>(const_cast<void*>(element_));
        
        int usage_page = IOHIDElementGetUsagePage(element);
        int usage = IOHIDElementGetUsage(element);
        int compiled_usage = compile_axis_usage(usage_page, usage);
        Axis axis = axis_from_usage(compiled_usage);
        
        if (axis != kAxisInvalid) {
            gamepad->set_bounds_for_axis(axis, IOHIDElementGetLogicalMin(element), IOHIDElementGetLogicalMax(element));
        }
    }
    
    void Gamepad_Darwin::handle_input_value(void* context, IOReturn, void*, IOHIDValueRef value) {
        Gamepad_Darwin* this_ = static_cast<Gamepad_Darwin*>(context);
        IOHIDElementRef element = IOHIDValueGetElement(value);
        long new_value = IOHIDValueGetIntegerValue(value);
        
        int usage = IOHIDElementGetUsage(element);
        int usage_page = IOHIDElementGetUsagePage(element);
        int compiled_usage = compile_axis_usage(usage_page, usage);
        Axis axis = axis_from_usage(compiled_usage);
        
        
        if (axis != kAxisInvalid) {
            this_->handle_axis_change(axis, new_value);
        } else {
            compiled_usage = compile_button_usage(usage_page, usage);
            this_->handle_button_change(compiled_usage, new_value);
        }
    }
    
    Gamepad_Darwin::Gamepad_Darwin(IOHIDDeviceRef device) : Gamepad(), _device(device) {
        std::tr1::unordered_map<unsigned, IOHIDElementRef> _unique_elements;
        
        CFArrayRef elements = IOHIDDeviceCopyMatchingElements(device, NULL, kIOHIDOptionsTypeNone);
        CFArrayApplyFunction(elements, CFRangeMake(0, CFArrayGetCount(elements)), collect_axis_bounds, this);
        CFRelease(elements);
        
        IOHIDDeviceRegisterInputValueCallback(device, Gamepad_Darwin::handle_input_value, this);
    }
}
