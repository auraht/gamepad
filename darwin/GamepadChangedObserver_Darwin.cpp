/*
 
GamepadChangedObserver_Darwin.cpp ... Implementation of GamepadChangedObserver
                                      for Darwin (i.e. Mac OS X).

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

#include "GamepadChangedObserver_Darwin.hpp"
#include "Gamepad_Darwin.hpp"
#include "../Eventloop.hpp"
#include "../Exception.hpp"

#include <algorithm>

namespace GP {
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
    
    void GamepadChangedObserver_Darwin::matched_device_cb(void* context, IOReturn, void*, IOHIDDeviceRef device) {
        GamepadChangedObserver_Darwin* this_ = static_cast<GamepadChangedObserver_Darwin*>(context);
        std::tr1::shared_ptr<Gamepad_Darwin> gamepad (new Gamepad_Darwin(device));
        this_->_active_devices.insert(std::make_pair(device, gamepad));
        this_->handle_event(gamepad.get(), kAttached);
    }
    void GamepadChangedObserver_Darwin::removing_device_cb(void* context, IOReturn, void*, IOHIDDeviceRef device) {
        GamepadChangedObserver_Darwin* this_ = static_cast<GamepadChangedObserver_Darwin*>(context);
        std::tr1::unordered_map<IOHIDDeviceRef, std::tr1::shared_ptr<Gamepad_Darwin> >::iterator it = this_->_active_devices.find(device);
        Gamepad_Darwin* gamepad = it->second.get();
        this_->handle_event(gamepad, kDetaching);
        this_->_active_devices.erase(it);
    }
    
    
    void GamepadChangedObserver_Darwin::observe_impl() {
        _manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
        
        CFTypeRef values[] = {
            create_usage_matcher(kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick),
            create_usage_matcher(kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad),
            create_usage_matcher(kHIDPage_GenericDesktop, kHIDUsage_GD_MultiAxisController)
        };
        const int values_count = sizeof(values)/sizeof(*values);
        
        CFArrayRef alternatives = CFArrayCreate(kCFAllocatorDefault, values, values_count, &kCFTypeArrayCallBacks);
        std::for_each(values, values+values_count, CFRelease);
        IOHIDManagerSetDeviceMatchingMultiple(_manager, alternatives);
        CFRelease(alternatives);
        
        IOHIDManagerRegisterDeviceMatchingCallback(_manager, GamepadChangedObserver_Darwin::matched_device_cb, this);
        IOHIDManagerRegisterDeviceRemovalCallback(_manager, GamepadChangedObserver_Darwin::removing_device_cb, this);

        CFRunLoopRef runloop = static_cast<CFRunLoopRef>(_eventloop->content());
        IOHIDManagerScheduleWithRunLoop(_manager, runloop, kCFRunLoopDefaultMode);
        IOHIDManagerOpen(_manager, kIOHIDOptionsTypeNone);
    }
    
    void GamepadChangedObserver_Darwin::unobserve_impl() {
        CFRunLoopRef runloop = static_cast<CFRunLoopRef>(_eventloop->content());
        IOHIDManagerUnscheduleFromRunLoop(_manager, runloop, kCFRunLoopDefaultMode);
        IOHIDManagerRegisterDeviceMatchingCallback(_manager, NULL, NULL);
        IOHIDManagerRegisterDeviceRemovalCallback(_manager, NULL, NULL);
        IOHIDManagerClose(_manager, kIOHIDOptionsTypeNone);
        CFRelease(_manager);
        _manager = NULL;
        _active_devices.clear();
    }
    
    GamepadChangedObserver* GamepadChangedObserver::create_impl(void* self, Callback callback, Eventloop* eventloop) {
        return new GamepadChangedObserver_Darwin(self, callback, eventloop);
    }
}

