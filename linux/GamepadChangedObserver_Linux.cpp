/*
 
GamepadChangedObserver_Linux.cpp ... Implementation of GamepadChangedObserver
                                     for Linux.

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
#include "GamepadExtra.hpp"

#include <ev.h>
#include <cassert>
#include <libudev.h>

namespace GP {
    void GamepadChangedObserver::create_impl(void* eventloop) {
        _loop = static_cast<struct ev_loop*>(eventloop);
        ev_ref(_loop);
        
        _udev = udev_new();
        assert(_udev != NULL);
        
        this->observe_device_changes();
        this->add_existing_devices();
    }
    
    void GamepadChangedObserver::observe_device_changes() {
        _udev_mon = udev_monitor_new_from_netlink(_udev, "udev");
        udev_monitor_filter_add_match_subsystem_devtype(_udev_mon, "hidraw", NULL);
        udev_monitor_enable_receiving(_udev_mon);
        int mon_fd = udev_monitor_get_fd(_udev_mon);
        
        ev_io_init(&_udev_watcher, GamepadChangedObserver::device_changed_cb, mon_fd, EV_READ);
        _udev_watcher.data = this;
        ev_io_start(_loop, &_udev_watcher);
    }
    
    void GamepadChangedObserver::add_existing_devices() {
        auto enumerator = udev_enumerate_new(_udev);
        udev_enumerate_add_match_subsystem(enumerator, "hidraw");
    	udev_enumerate_scan_devices(enumerator);
    	
    	auto devices = udev_enumerate_get_list_entry(enumerator);
    	udev_list_entry* entry;
    	
    	udev_list_entry_foreach (entry, devices) {
    	    auto sys_path = udev_list_entry_get_name(entry);
    	    auto device = udev_device_new_from_syspath(_udev, sys_path);
    	    this->open_device(device);
    	    udev_device_unref(device);
    	}
    	
    	udev_enumerate_unref(enumerator);
    }
    
    void GamepadChangedObserver::device_changed_cb(struct ev_loop*, ev_io* watcher, int) {
        auto this_ = static_cast<GamepadChangedObserver*>(watcher->data);
        
        udev_device* device = udev_monitor_receive_device(this_->_udev_mon);
        assert(device != NULL);
        
        auto action = udev_device_get_action(device);
        
        if (strcmp(action, "add") == 0)
            this_->open_device(device);
        else if (strcmp(action, "remove") == 0)
            this_->close_device(device);
        else
            assert(false && "Unsupported action!");
        
        udev_device_unref(device);
    }
    
    void GamepadChangedObserver::open_device(udev_device* device) {
        const char* path = udev_device_get_devnode(device);
        GamepadData data = {path, _loop, false};
        auto gamepad = std::make_shared<Gamepad>(&data);
        if (data.valid) {
            dev_t devnum = udev_device_get_devnum(device);
            this->handle_event(*gamepad, GamepadState::attached);
            _active_devices.insert(std::make_pair(devnum, gamepad));
        }
    }
    
    void GamepadChangedObserver::close_device(udev_device* device) {
        dev_t devnum = udev_device_get_devnum(device);
        auto cit = _active_devices.find(devnum);
        if (cit != _active_devices.end()) {
            this->handle_event(*cit->second, GamepadState::detaching);
            _active_devices.erase(cit);
        }
    }
    
    GamepadChangedObserver::~GamepadChangedObserver() {
        ev_io_stop(_loop, &_udev_watcher);
        udev_unref(_udev);
        ev_unref(_loop);
    }

    auto GamepadChangedObserver::cbegin() const -> const_iterator {
        return const_iterator(_active_devices.cbegin());
    }
    
    auto GamepadChangedObserver::cend() const -> const_iterator {
        return const_iterator(_active_devices.cend());
    }
}

