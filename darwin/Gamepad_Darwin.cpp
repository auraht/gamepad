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
#include <mach/mach_time.h>

namespace GP {
    struct Ctx {
        Gamepad_Darwin* gamepad;
        CFMutableArrayRef restricted_elements;
    };
    
    void Gamepad_Darwin::collect_axis_bounds(const void* element_, void* self) {
        Ctx* ctx = static_cast<Ctx*>(self);
        IOHIDElementRef element = static_cast<IOHIDElementRef>(const_cast<void*>(element_));
        
        int usage_page = IOHIDElementGetUsagePage(element);
        int usage = IOHIDElementGetUsage(element);
        Axis axis = axis_from_usage(usage_page, usage);
        auto elem_type = IOHIDElementGetType(element);
        
        if (axis != Axis::invalid) {
            ctx->gamepad->set_bounds_for_axis(axis, IOHIDElementGetLogicalMin(element), IOHIDElementGetLogicalMax(element));
        } else if (elem_type != kIOHIDElementTypeInput_Button) {
            if (elem_type == kIOHIDElementTypeOutput || elem_type == kIOHIDElementTypeFeature) {
                int compiled_usage = usage_page << 16 | usage;
                ctx->gamepad->_valid_output_elements.insert({compiled_usage, element});
            }
            element = NULL;
        }
        
        if (element != NULL) {
            CFTypeRef key = CFSTR(kIOHIDElementCookieKey);
            IOHIDElementCookie cookie = IOHIDElementGetCookie(element);
            CFTypeRef value = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &cookie);
            CFDictionaryRef dictionary = CFDictionaryCreate(kCFAllocatorDefault, &key, &value, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
            CFRelease(value);
            
            CFArrayAppendValue(ctx->restricted_elements, dictionary);
            CFRelease(dictionary);
        }
    }
    
    void Gamepad_Darwin::handle_input_value(void* context, IOReturn, void*, IOHIDValueRef value) {
        Gamepad_Darwin* this_ = static_cast<Gamepad_Darwin*>(context);
        IOHIDElementRef element = IOHIDValueGetElement(value);
        long new_value = IOHIDValueGetIntegerValue(value);
        
        int usage = IOHIDElementGetUsage(element);
        int usage_page = IOHIDElementGetUsagePage(element);
        Axis axis = axis_from_usage(usage_page, usage);
        
        if (axis != Axis::invalid) {
            this_->set_axis_value(axis, new_value);
        } else {
            Button button = button_from_usage(usage_page, usage);
            this_->handle_button_change(button, new_value);
        }
    }
    
    void Gamepad_Darwin::handle_report(void* context, IOReturn, void*, IOHIDReportType, uint32_t, uint8_t*, CFIndex) {
        Gamepad_Darwin* this_ = static_cast<Gamepad_Darwin*>(context);
        this_->handle_axes_change();
    }
    
    void send(int usage_page, int usage, const unsigned char* content, size_t content_size);
    void retrieve(int usage_page, int usage, unsigned char* buffer, size_t buffer_size);

        
    Gamepad_Darwin::Gamepad_Darwin(IOHIDDeviceRef device) : Gamepad(), _device{device} {
        CFArrayRef elements = IOHIDDeviceCopyMatchingElements(device, NULL, kIOHIDOptionsTypeNone);
        CFMutableArrayRef restricted_elements = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
        
        Ctx ctx = {this, restricted_elements};
        CFArrayApplyFunction(elements, CFRangeMake(0, CFArrayGetCount(elements)), collect_axis_bounds, &ctx);
        CFRelease(elements);
        
        IOHIDDeviceSetInputValueMatchingMultiple(device, restricted_elements);
        CFRelease(restricted_elements);
        
        IOHIDDeviceRegisterInputValueCallback(device, Gamepad_Darwin::handle_input_value, this);
        IOHIDDeviceRegisterInputReportCallback(device, NULL, 0, Gamepad_Darwin::handle_report, this);
    }
    
    //## WARNING: THE FOLLOWING TWO METHODS ARE NOT TESTED! 
    //            Testing will be delayed to when I've met a device that the
    //             output format is reliably decoded. Do not use them in 
    //             productive environment.
            
    bool Gamepad_Darwin::send(int usage_page, int usage, const void* content, size_t content_size) {
        int compiled_usage = usage_page << 16 | usage;
        auto it = _valid_output_elements.find(compiled_usage);
        if (it == _valid_output_elements.end())
            return false;
        
        
        auto elem = it->second;
        //## TODO: Or use IOHIDValueCreateWithBytes??
        auto value = IOHIDValueCreateWithBytesNoCopy(kCFAllocatorDefault, elem, mach_absolute_time(), static_cast<const uint8_t*>(content), content_size);
        IOReturn retval = IOHIDDeviceSetValue(_device, elem, value);
        CFRelease(value);
        
        return retval == 0;
    }
    
    bool Gamepad_Darwin::retrieve(int usage_page, int usage, void* buffer, size_t buffer_size) {
        int compiled_usage = usage_page << 16 | usage;
        auto it = _valid_output_elements.find(compiled_usage);
        if (it == _valid_output_elements.end())
            return false;
        
        
        auto elem = it->second;
        IOHIDValueRef value;
        IOReturn retval = IOHIDDeviceGetValue(_device, elem, &value);
        if (retval)
            return false;
        
        size_t length = IOHIDValueGetLength(value);
        const uint8_t* bytePtr = IOHIDValueGetBytePtr(value);
        if (length > buffer_size)
            length = buffer_size;
        memcpy(buffer, bytePtr, length);
        return true;
    }
    
    //## END WARNING
}
