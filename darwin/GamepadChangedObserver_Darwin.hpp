/*
 
GamepadChangedObserver_Darwin.hpp ... Implementation of GamepadChangedObserver
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

#ifndef GAMEPAD_CHANGED_OBSERVER_DARWIN_HPP_wludp11y72lw61or
#define GAMEPAD_CHANGED_OBSERVER_DARWIN_HPP_wludp11y72lw61or 1

#include "../GamepadChangedObserver.hpp"
#include <IOKit/hid/IOHIDManager.h>
#include <CoreFoundation/CoreFoundation.h>
#include <tr1/unordered_map>
#include <tr1/memory>

namespace GP {
    class Gamepad_Darwin;
    
    class GamepadChangedObserver_Darwin : public GamepadChangedObserver {
    private:
        IOHIDManagerRef _manager;
        std::tr1::unordered_map<IOHIDDeviceRef, std::tr1::shared_ptr<Gamepad_Darwin> > _active_devices;
        CFRunLoopRef _runloop;
        
        static void matched_device_cb(void* context, IOReturn result, void* sender, IOHIDDeviceRef device);
        static void removing_device_cb(void* context, IOReturn result, void* sender, IOHIDDeviceRef device);
        
    protected:
        virtual void observe_impl();
        void unobserve_impl();
        
    public:
        GamepadChangedObserver_Darwin(void* self, Callback callback, void* eventloop)
            : GamepadChangedObserver(self, callback),
              _manager(NULL), _runloop(static_cast<CFRunLoopRef>(eventloop)) {}
              
        ~GamepadChangedObserver_Darwin() {
            this->unobserve_impl();
        }
    };
}

#endif
