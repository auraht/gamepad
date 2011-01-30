/*
 
GamepadChangedObserver_Darwin.cpp ... Implementation of GamepadChangedObserver
                                      for Darwin (i.e. Mac OS X).

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

#include "GamepadChangedObserver_Darwin.hpp"
#include "Gamepad_Darwin.hpp"
#include "../Exception.hpp"

#include <algorithm>

namespace GP {
    // the function of this function is to create the matcher dictionary given
    // the usage page and usage.
    static CFDictionaryRef create_usage_matcher(int usage_page, int usage) {
        CFTypeRef keys[] = {
            CFSTR(kIOHIDDeviceUsagePageKey), 
            CFSTR(kIOHIDDeviceUsageKey)
        };
        CFTypeRef values[] = {
            CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage_page), 
            CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage)
        };
        const int values_count = sizeof(values)/sizeof(*values);
        
        CFDictionaryRef retval = CFDictionaryCreate(kCFAllocatorDefault, keys, values, values_count, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        std::for_each(values, values+values_count, CFRelease);
            
        return retval;
    }
    
    // this method is called whenever a device is attached.
    void GamepadChangedObserver_Darwin::matched_device_cb(void* context, IOReturn, void*, IOHIDDeviceRef device) {
        auto this_ = static_cast<GamepadChangedObserver_Darwin*>(context);
        
        auto gamepad = new Gamepad_Darwin(device);
        
        this_->_active_devices.insert({device, std::shared_ptr<Gamepad_Darwin>(gamepad)});
        this_->handle_event(gamepad, GamepadState::attached);
    }
    
    // this method is called whenever a device is removed.
    void GamepadChangedObserver_Darwin::removing_device_cb(void* context, IOReturn, void*, IOHIDDeviceRef device) {
        auto this_ = static_cast<GamepadChangedObserver_Darwin*>(context);
        auto it = this_->_active_devices.find(device);
        Gamepad_Darwin* gamepad = it->second.get();
        this_->handle_event(gamepad, GamepadState::detaching);
        this_->_active_devices.erase(it);
    }
    
    
    void GamepadChangedObserver_Darwin::observe_impl() {
        _manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
        
        // we only care about joysticks, gamepads and multi-axis controllers,
        // just like SDL.
        CFTypeRef values[] = {
            create_usage_matcher(kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick),
            create_usage_matcher(kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad),
            create_usage_matcher(kHIDPage_GenericDesktop, kHIDUsage_GD_MultiAxisController)
        };
        const int values_count = sizeof(values)/sizeof(*values);
        
        // tell the HID manager to observe those 3 kinds of devices.
        CFArrayRef alternatives = CFArrayCreate(kCFAllocatorDefault, values, values_count, &kCFTypeArrayCallBacks);
        std::for_each(values, values+values_count, CFRelease);
        IOHIDManagerSetDeviceMatchingMultiple(_manager, alternatives);
        CFRelease(alternatives);
        
        // tell the HID manager to watch for attachment and removal.
        IOHIDManagerRegisterDeviceMatchingCallback(_manager, GamepadChangedObserver_Darwin::matched_device_cb, this);
        IOHIDManagerRegisterDeviceRemovalCallback(_manager, GamepadChangedObserver_Darwin::removing_device_cb, this);

        // begin observing.
        IOHIDManagerScheduleWithRunLoop(_manager, _runloop, kCFRunLoopDefaultMode);
        IOHIDManagerOpen(_manager, kIOHIDOptionsTypeNone);
    }
    
    void GamepadChangedObserver_Darwin::unobserve_impl() {
        IOHIDManagerUnscheduleFromRunLoop(_manager, _runloop, kCFRunLoopDefaultMode);
        IOHIDManagerRegisterDeviceMatchingCallback(_manager, NULL, NULL);
        IOHIDManagerRegisterDeviceRemovalCallback(_manager, NULL, NULL);
        IOHIDManagerClose(_manager, kIOHIDOptionsTypeNone);
        CFRelease(_manager);
        _manager = NULL;
        _active_devices.clear();
    }
    
    GamepadChangedObserver* GamepadChangedObserver::create_impl(void* self, Callback callback, void* eventloop) {
        return new GamepadChangedObserver_Darwin(self, callback, eventloop);
    }
}

