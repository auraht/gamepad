/*
 
GamepadEventloop_Darwin.cpp ... Implementation of GamepadEventloop for Darwin
                                (i.e. Mac OS X).

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

#include "Gamepad_Darwin.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <boost/tr1/unordered_map.hpp>
#include <algorithm>

namespace GP {
    static void collect_unique_elements(const void *value, void *context) {
        std::tr1::unordered_map<unsigned, IOHIDElementRef>& queue = *static_cast<std::tr1::unordered_map<unsigned, IOHIDElementRef>*>(context);
        IOHIDElementRef element = static_cast<IOHIDElementRef>(const_cast<void*>(value));
        
        unsigned encoded_usage = IOHIDElementGetUsagePage(element) << 8 | IOHIDElementGetUsage(element);
        queue[encoded_usage] = element;
    }
    
    void Gamepad_Darwin::handle_input_value(void* context, IOReturn, void*, IOHIDValueRef value) {
        Gamepad_Darwin* this_ = static_cast<Gamepad_Darwin*>(context);
        IOHIDElementRef element = IOHIDValueGetElement(value);
        int usage = IOHIDElementGetUsage(element);
        long new_value = IOHIDValueGetIntegerValue(value);
        /// TODO: More sophisticated checking for whether a usage is axis or not.
        if (IOHIDElementGetUsagePage(element) == kHIDPage_GenericDesktop) {
            this_->handle_axis_change(axis_from_usage(usage), new_value);
        } else {
            this_->handle_button_change(usage, new_value);
        }
    }
    
    Gamepad_Darwin::Gamepad_Darwin(IOHIDDeviceRef device) : Gamepad(), _device(device) {
        std::tr1::unordered_map<unsigned, IOHIDElementRef> _unique_elements;
        
        CFArrayRef elements = IOHIDDeviceCopyMatchingElements(device, NULL, kIOHIDOptionsTypeNone);
        CFArrayApplyFunction(elements, CFRangeMake(0, CFArrayGetCount(elements)), collect_unique_elements, &_unique_elements);
        
        CFMutableArrayRef restricted_elements = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
        for (std::tr1::unordered_map<unsigned, IOHIDElementRef>::const_iterator cit = _unique_elements.begin(); cit != _unique_elements.end(); ++ cit) {
            IOHIDElementRef element = cit->second;
            
            CFTypeRef key = CFSTR(kIOHIDElementCookieKey);
            IOHIDElementCookie cookie = IOHIDElementGetCookie(element);
            CFTypeRef value = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &cookie);
            CFDictionaryRef dictionary = CFDictionaryCreate(kCFAllocatorDefault, &key, &value, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
            CFRelease(value);
            
            if (IOHIDElementGetUsagePage(element) == kHIDPage_GenericDesktop) {
                Axis axis = axis_from_usage(IOHIDElementGetUsage(element));
                this->set_bounds_for_axis(axis, IOHIDElementGetLogicalMin(element), IOHIDElementGetLogicalMax(element));
            }
            
            CFArrayAppendValue(restricted_elements, dictionary);
            CFRelease(dictionary);
        }
        IOHIDDeviceSetInputValueMatchingMultiple(device, restricted_elements);
        CFRelease(restricted_elements);

        IOHIDDeviceRegisterInputValueCallback(device, Gamepad_Darwin::handle_input_value, this);
    }
    
    unsigned long Gamepad_Darwin::local_address() const {
        CFNumberRef location = static_cast<CFNumberRef>(IOHIDDeviceGetProperty(_device, CFSTR(kIOHIDLocationIDKey)));
        uintptr_t retval;
        CFNumberGetValue(location, kCFNumberLongType, &retval);
        return retval;
    }
}
