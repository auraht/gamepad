/*
 
GamepadChangedObserver.hpp ... Observe if a gamepad device has been changed.

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

#ifndef GAMEPAD_CHANGED_OBSERVER_HPP_rskkt3ru5raa714i
#define GAMEPAD_CHANGED_OBSERVER_HPP_rskkt3ru5raa714i 1

#include "Compatibility.hpp"
#include <iterator>
#include <memory>

#if GP_PLATFORM == GP_PLATFORM_WINDOWS
#include <Windows.h>
#include <unordered_map>
#elif GP_PLATFORM == GP_PLATFORM_DARWIN
#include <IOKit/hid/IOHIDManager.h>
#include <CoreFoundation/CoreFoundation.h>
#include <unordered_map>
#include "darwin/Shared.hpp"
#elif GP_PLATFORM == GP_PLATFORM_LINUX
#include <ev.h>
#include <unordered_map>
#include <memory>
#include <libudev.h>
#endif


namespace GP {
    class Gamepad;

    ENUM_CLASS GamepadState {
        attached,
        detaching
    };


    class GamepadChangedObserver {
    public:            
        typedef void (*Callback)(void* self, const GamepadChangedObserver& observer, Gamepad& gamepad, GamepadState state);
    
    private:

        // Implementation-specific stuff...
#if GP_PLATFORM == GP_PLATFORM_WINDOWS
        
        HWND _hwnd;
        HDEVNOTIFY _notif;
        std::unordered_map<HANDLE, std::shared_ptr<Gamepad>> _active_devices;
        // ^ we need to use a shared_ptr<> instead of unique_ptr<> since
        //   MSVC does not have a proper .emplace().

        void create_impl(void* eventloop);

        void create_invisible_listener_window();
        void listen_for_hotplugged_devices();

        static LRESULT CALLBACK message_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
        void populate_existing_devices(const GUID* phid_guid);
        Gamepad* insert_device_with_path(HWND hwnd, LPCTSTR path);

#elif GP_PLATFORM == GP_PLATFORM_DARWIN

        IOHIDManager _manager;
        CFRunLoopRef _runloop;
        std::unordered_map<IOHIDDeviceRef, std::shared_ptr<Gamepad>> _active_devices;
        // ^ we need to use a shared_ptr<> instead of unique_ptr<> because of
        //   http://gcc.gnu.org/bugzilla/show_bug.cgi?id=44436

        void create_impl(void* runloop);
        
        void set_observe_joysticks();
        void add_active_device(IOHIDDeviceRef device);
        void remove_active_device(IOHIDDeviceRef device);
        
        static void matched_device_cb(void* context, IOReturn, void*, IOHIDDeviceRef device);
        static void removing_device_cb(void* context, IOReturn, void*, IOHIDDeviceRef device);

#elif GP_PLATFORM == GP_PLATFORM_LINUX

        struct ev_loop* _loop;
        udev* _udev;
        udev_monitor* _udev_mon;
        ev_io _udev_watcher;

        void create_impl(void* eventloop);
        
        std::unordered_map<dev_t, std::shared_ptr<Gamepad>> _active_devices;
        
        void observe_device_changes();
        void add_existing_devices();
        void open_device(udev_device* device);
        void close_device(udev_device* device);
        
        static void device_changed_cb(struct ev_loop* loop, ev_io* watcher, int revents);

#endif
        
        // Implementation-independent stuff...
        void* _self;
        Callback _callback;

    public:
        struct const_iterator : public std::iterator<std::forward_iterator_tag, Gamepad> {
        private:

#if GP_PLATFORM == GP_PLATFORM_WINDOWS
            std::unordered_map<HANDLE, std::shared_ptr<Gamepad>>::const_iterator cit;
#elif GP_PLATFORM == GP_PLATFORM_DARWIN
            std::unordered_map<IOHIDDeviceRef, std::shared_ptr<Gamepad>>::const_iterator cit;
#elif GP_PLATFORM == GP_PLATFORM_LINUX
            std::unordered_map<dev_t, std::shared_ptr<Gamepad>>::const_iterator cit;
#endif
            const_iterator(const decltype(cit)& it) : cit(it) {}            

        public:
            typedef std::forward_iterator_tag iterator_tag;
        
            const_iterator& operator++() {
                ++cit;
                return *this;
            }
            const_iterator operator++(int) {
                auto oldCit = cit;
                ++cit;
                return const_iterator(oldCit);
            }
            
            Gamepad& operator*() const { return *(cit->second); }
            Gamepad& operator->() const { return **this; }
            
            const_iterator() {};
            
            const_iterator(const const_iterator& other) : cit(other.cit) {}
            const_iterator(const_iterator&& other) : cit(std::move(other.cit)) {}
            const_iterator& operator=(const const_iterator& other) {
                if (this != &other)
                    cit = other.cit;
                return *this;
            }
            const_iterator& operator=(const_iterator&& other) {
                if (this != &other)
                    std::swap(cit, other.cit);
                return *this;
            }
            
            bool operator==(const const_iterator& other) const {
                return cit == other.cit;
            }
            bool operator!=(const const_iterator& other) const {
                return cit != other.cit;
            }
            
            friend class GamepadChangedObserver;
        };
        typedef const_iterator iterator;

    private:        
        void handle_event(Gamepad& gamepad, GamepadState state) const {
            if (_callback)
                _callback(_self, *this, gamepad, state);
        }
                
        friend struct Impl;
        
    public:
        GamepadChangedObserver(void* self, Callback callback, void* eventloop) : _self(self), _callback(callback) {
            this->create_impl(eventloop);
        }
        ~GamepadChangedObserver();

    private:
        GamepadChangedObserver(const GamepadChangedObserver&);
        GamepadChangedObserver(GamepadChangedObserver&&);
        GamepadChangedObserver& operator=(const GamepadChangedObserver&);
        GamepadChangedObserver& operator=(GamepadChangedObserver&&);

    public:
        const_iterator cbegin() const;
        const_iterator cend() const;
        iterator begin() const { return this->cbegin(); }
        iterator end() const { return this->cend(); }
    };
    
}

#endif
