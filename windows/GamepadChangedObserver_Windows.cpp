/*

GamepadChangedObserver_Windows.cpp ... Implementation of GamepadChangedObserver
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

#include "GamepadChangedObserver_Windows.hpp"
#include "Gamepad_Windows.hpp"
#include "../Exception.hpp"

#include <unordered_map>
#include <vector>

#include "hidusage.h"
const USHORT HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER = 8;

namespace GP {
    static std::unordered_map<HWND, GamepadChangedObserver_Windows*> _OBSERVERS;

    LRESULT CALLBACK GamepadChangedObserver_Windows::message_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        auto cit = _OBSERVERS.find(hwnd);
        /// TODO: Should we test whether cit is valid?
        GamepadChangedObserver_Windows* this_ = cit->second;

        switch (msg) {
        case WM_INPUT_DEVICE_CHANGE:
            {
                HANDLE hdevice = reinterpret_cast<HANDLE>(lparam);
                if (wparam == GIDC_ARRIVAL) {
                    std::tr1::shared_ptr<Gamepad_Windows> gamepad (new Gamepad_Windows(hdevice));
                    this_->_active_devices.insert(std::make_pair(hdevice, gamepad));
                    this_->handle_event(gamepad.get(), GamepadState::attached);
                } else if (wparam == GIDC_REMOVAL) {
                    auto it = this_->_active_devices.find(hdevice);
                    this_->handle_event(it->second.get(), GamepadState::detaching);
                    this_->_active_devices.erase(it);
                }
                break;
            }

        case WM_INPUT:
            {
                UINT buffer_size;
                HRAWINPUT hdata = reinterpret_cast<HRAWINPUT>(lparam);

                GetRawInputData(hdata, RID_INPUT, NULL, &buffer_size, sizeof(RAWINPUTHEADER));
                std::vector<char> buffer (buffer_size);
                RAWINPUT* input = reinterpret_cast<RAWINPUT*>(&buffer[0]);

                /// TODO: Throw if GetRawInputData returns incorrect size.
                GetRawInputData(hdata, RID_INPUT, input, &buffer_size, sizeof(RAWINPUTHEADER));
                
                auto it = this_->_active_devices.find(input->header.hDevice);
                Gamepad_Windows* gamepad = it->second.get();
                gamepad->handle_raw_input(input->data.hid);

                return DefWindowProc(hwnd, msg, wparam, lparam);
            }

        default:
            break;
        }

        return CallWindowProc(this_->_orig_wndproc, hwnd, msg, wparam, lparam);
    }

    void GamepadChangedObserver_Windows::observe_impl() {
        /// TODO: Need a mutex here.
        std::pair<std::unordered_map<HWND, GamepadChangedObserver_Windows*>::iterator, bool>
            res = _OBSERVERS.insert(std::make_pair(_hwnd, this));
        if (!res.second) {
            throw MultipleObserverException();
        }

        RAWINPUTDEVICE devices_to_register[] = {
            {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_JOYSTICK, RIDEV_DEVNOTIFY|RIDEV_INPUTSINK, _hwnd},
            {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_GAMEPAD, RIDEV_DEVNOTIFY|RIDEV_INPUTSINK, _hwnd},
            {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER, RIDEV_DEVNOTIFY|RIDEV_INPUTSINK, _hwnd}
        };
        RegisterRawInputDevices(devices_to_register, sizeof(devices_to_register)/sizeof(*devices_to_register), sizeof(*devices_to_register));


        _orig_wndproc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(_hwnd, GWLP_WNDPROC));
        SetWindowLongPtr(_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(GamepadChangedObserver_Windows::message_handler));
    }

    void GamepadChangedObserver_Windows::unobserve_impl() {
        /// TODO: Need a mutex here.
        SetWindowLongPtr(_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(_orig_wndproc));
        _OBSERVERS.erase(_hwnd);

        /// TODO: Are these necessary???
        RAWINPUTDEVICE devices_to_register[] = {
            {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_JOYSTICK, RIDEV_REMOVE, NULL},
            {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_GAMEPAD, RIDEV_REMOVE, NULL},
            {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER, RIDEV_REMOVE, NULL}
        };
        RegisterRawInputDevices(devices_to_register, sizeof(devices_to_register)/sizeof(*devices_to_register), sizeof(*devices_to_register));
    }

    __declspec(dllexport) GamepadChangedObserver* GamepadChangedObserver::create_impl(void* self, Callback callback, void* eventloop) {
        return new GamepadChangedObserver_Windows(self, callback, static_cast<HWND>(eventloop));
    }
}

