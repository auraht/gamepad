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

#include <algorithm>
#include <exception>
#include <memory>


namespace GP {
    void GamepadChangedObserver::create_impl(void* runloop) {
        this->set_observe_joysticks();
        _manager.register_device_matched(&matched_device_cb, this);
        _manager.register_device_removal(&removing_device_cb, this);
        
        _runloop = static_cast<CFRunLoopRef>(runloop);
        _manager.schedule(_runloop);
        _manager.open();
    }
    
    GamepadChangedObserver::~GamepadChangedObserver() {
        _manager.unschedule(_runloop);
        _manager.register_device_matched(NULL, NULL);
        _manager.register_device_removal(NULL, NULL);
        _manager.close();
    }
    
    void GamepadChangedObserver::add_active_device(IOHIDDeviceRef device) {
        auto gamepad = std::make_shared<Gamepad>(device);
        _active_devices.insert({device, gamepad});
        this->handle_event(*gamepad, GamepadState::attached);
    }
    
    void GamepadChangedObserver::remove_active_device(IOHIDDeviceRef device) {
        auto it = _active_devices.find(device);
        auto gamepad = (it->second);
        this->handle_event(*gamepad, GamepadState::detaching);
        _active_devices.erase(it);
    }
    
    
    void GamepadChangedObserver::matched_device_cb(void* context, IOReturn, void*, IOHIDDeviceRef device) {
        auto this_ = static_cast<GamepadChangedObserver*>(context);
        this_->add_active_device(device);        
    }
    
    void GamepadChangedObserver::removing_device_cb(void* context, IOReturn, void*, IOHIDDeviceRef device) {
        auto this_ = static_cast<GamepadChangedObserver*>(context);
        this_->remove_active_device(device);
    }
    
    void GamepadChangedObserver::set_observe_joysticks() {
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
    
    auto GamepadChangedObserver::cbegin() const -> const_iterator {
        return const_iterator(_active_devices.cbegin());
    }
    
    auto GamepadChangedObserver::cend() const -> const_iterator {
        return const_iterator(_active_devices.cend());
    }
}

