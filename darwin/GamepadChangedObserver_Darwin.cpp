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

#include "../GamepadChangedObserver.hpp"
#include "../Gamepad.hpp"
#include "../Exception.hpp"

#include "Shared.hpp"
#include <IOKit/hid/IOHIDManager.h>
#include <algorithm>
#include <exception>
#include <unordered_map>
#include <memory>


namespace GP {
    struct IOHIDManager : CFType<IOHIDManagerRef> {
        IOHIDManager() : CFType(
            IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone)
        ) {}
        
        void set_device_matching_multiple(const CFArray& array) {
            IOHIDManagerSetDeviceMatchingMultiple(value, array.value);
        }
        void register_device_matched(IOHIDDeviceCallback callback, void* context) {
            IOHIDManagerRegisterDeviceMatchingCallback(value, callback, context);
        }
        void register_device_removal(IOHIDDeviceCallback callback, void* context) {
            IOHIDManagerRegisterDeviceRemovalCallback(value, callback, context);
        }
        
        void schedule(CFRunLoopRef runloop) {
            IOHIDManagerScheduleWithRunLoop(value, runloop, kCFRunLoopDefaultMode);
        }
        void open() {
            IOReturn retval = IOHIDManagerOpen(value, kIOHIDOptionsTypeNone);
            if (retval != kIOReturnSuccess)
                throw SystemErrorException(retval);
        }
        void unschedule(CFRunLoopRef runloop) {
            IOHIDManagerUnscheduleFromRunLoop(value, runloop, kCFRunLoopDefaultMode);
        }
        void close() {
            IOHIDManagerClose(value, kIOHIDOptionsTypeNone);
        }
    };
    
    
    struct GamepadChangedObserver::Impl {
    private:
        IOHIDManager _manager;
        CFRunLoopRef _runloop;
        std::unordered_map<IOHIDDeviceRef, std::shared_ptr<Gamepad>> _active_devices;
        // ^ we need to use a shared_ptr<> instead of unique_ptr<> because of
        //   http://gcc.gnu.org/bugzilla/show_bug.cgi?id=44436 
        
    public:
        void set_observe_joysticks();
        void add_active_device(IOHIDDeviceRef device, GamepadChangedObserver* observer);
        void remove_active_device(IOHIDDeviceRef device, GamepadChangedObserver* observer);
        
        Impl(GamepadChangedObserver* observer, CFRunLoopRef runloop);
        ~Impl();
        
        static void matched_device_cb(void* context, IOReturn, void*, IOHIDDeviceRef device);
        static void removing_device_cb(void* context, IOReturn, void*, IOHIDDeviceRef device);
    };
    
    GamepadChangedObserver::Impl::Impl(GamepadChangedObserver* observer, CFRunLoopRef runloop) {
        this->set_observe_joysticks();
        _manager.register_device_matched(&Impl::matched_device_cb, observer);
        _manager.register_device_removal(&Impl::removing_device_cb, observer);
        
        _runloop = runloop;
        _manager.schedule(runloop);
        _manager.open();
    }
    
    GamepadChangedObserver::Impl::~Impl() {
        _manager.unschedule(_runloop);
        _manager.register_device_matched(NULL, NULL);
        _manager.register_device_removal(NULL, NULL);
        _manager.close();
    }
    
    void GamepadChangedObserver::Impl::add_active_device(IOHIDDeviceRef device, GamepadChangedObserver* observer) {
        auto gamepad = std::make_shared<Gamepad>(device);
        _active_devices.insert({device, gamepad});
        observer->handle_event(gamepad.get(), GamepadState::attached);
    }
    
    void GamepadChangedObserver::Impl::remove_active_device(IOHIDDeviceRef device, GamepadChangedObserver* observer) {
        auto it = _active_devices.find(device);
        auto gamepad = it->second.get();
        observer->handle_event(gamepad, GamepadState::detaching);
        _active_devices.erase(it);
    }
    
    
    void GamepadChangedObserver::Impl::matched_device_cb(void* context, IOReturn, void*, IOHIDDeviceRef device) {
        auto this_ = static_cast<GamepadChangedObserver*>(context);
        this_->_impl->add_active_device(device, this_);        
    }
    
    void GamepadChangedObserver::Impl::removing_device_cb(void* context, IOReturn, void*, IOHIDDeviceRef device) {
        auto this_ = static_cast<GamepadChangedObserver*>(context);
        this_->_impl->remove_active_device(device, this_);
    }
    
    void GamepadChangedObserver::Impl::set_observe_joysticks() {
        CFNumber generic_desktop_number (kHIDPage_GenericDesktop);
        
        CFTypeRef keys[] = {CFSTR(kIOHIDDeviceUsagePageKey), CFSTR(kIOHIDDeviceUsageKey)};
        CFTypeRef values[2] = {generic_desktop_number.value};

        // we only care about joysticks, gamepads and multi-axis controllers,
        // just like SDL.
        CFNumber joystick_number (kHIDUsage_GD_Joystick);            
        values[1] = joystick_number.value;
        CFDictionary joystick (keys, values, 2);
        
        CFNumber game_pad_number (kHIDUsage_GD_GamePad);
        values[1] = game_pad_number.value;
        CFDictionary game_pad (keys, values, 2);
        
        CFNumber multi_axis_ctrler_number (kHIDUsage_GD_MultiAxisController);
        values[1] = multi_axis_ctrler_number.value;
        CFDictionary multi_axis_ctrler (keys, values, 2);
        
        CFTypeRef alt_values[] = {joystick.value, game_pad.value, multi_axis_ctrler.value};
        CFArray alternatives (alt_values, sizeof(alt_values)/sizeof(*alt_values));

        // tell the HID manager to observe those 3 kinds of devices.
        _manager.set_device_matching_multiple(alternatives);
    }
    
    
    void GamepadChangedObserver::create_impl(void* eventloop) {
        _impl = new Impl(this, static_cast<CFRunLoopRef>(eventloop));
    }
    
    GamepadChangedObserver::~GamepadChangedObserver() {
        delete _impl;
    }
}

