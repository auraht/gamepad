/*
 
GamepadChangedObserver_Windows.hpp ... Implementation of GamepadChangedObserver
                                       for Windows.

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

#ifndef GAMEPAD_CHANGED_OBSERVER_WINDOWS_HPP_5qxep9lbh5ubx1or
#define GAMEPAD_CHANGED_OBSERVER_WINDOWS_HPP_5qxep9lbh5ubx1or 1

#include "../GamepadChangedObserver.hpp"
#include <unordered_map>
#include <memory>
#include <Windows.h>

namespace GP {
    class Gamepad_Windows;
    
    class GamepadChangedObserver_Windows : public GamepadChangedObserver {
    private:
		HWND _hwnd;
        HDEVNOTIFY _notif;
        std::tr1::unordered_map<HANDLE, std::tr1::shared_ptr<Gamepad_Windows> > _active_devices;

        void insert_device_with_path(HWND hwnd, LPCTSTR path);
        void populate_existing_devices(const GUID* phid_guid);

        static LRESULT CALLBACK message_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
        
    protected:
        virtual void observe_impl();
        void unobserve_impl();
        
    public:
        GamepadChangedObserver_Windows(void* self, Callback callback, HWND hwnd)
            : GamepadChangedObserver(self, callback), _hwnd(hwnd) {}
              
        ~GamepadChangedObserver_Windows() {
            this->unobserve_impl();
        }
    };
}

#endif
