/*
 
Gamepad_Linux.cpp ... Implementation of Gamepad for Linux.

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

#include "GamepadExtra.hpp"
#include "../Gamepad.hpp"
#include <linux/hidraw.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cassert>
//#include <cstdio>

namespace GP {
    void Gamepad::create_impl(void* implementation_data) {
        auto data = static_cast<GamepadData*>(implementation_data);
        data->valid = this->create_impl_impl(data->path, data->loop);
    }
    
    bool Gamepad::create_impl_impl(const char* path, struct ev_loop* loop) {
#define CHECK(cond) if(!(cond)) return false
    
        // Should O_NONBLOCK exist??
        _fd = open(path, O_RDONLY|O_NONBLOCK);
        CHECK(_fd != -1);
        
        struct hidraw_report_descriptor descriptor;
        CHECK(ioctl(_fd, HIDIOCGRDESCSIZE, &descriptor.size) >= 0);
        CHECK(ioctl(_fd, HIDIOCGRDESC, &descriptor) >= 0);
        
        CHECK(descriptor.size == GP_KNOWN_DESCRIPTOR_LENGTH);
        CHECK(memcmp(descriptor.value, GP_KNOWN_DESCRIPTOR, GP_KNOWN_DESCRIPTOR_LENGTH) == 0);
        
        _loop = loop;
        ev_io_init(&_watcher, Gamepad::read_report_cb, _fd, EV_READ);
        _watcher.data = this;
        ev_io_start(loop, &_watcher);
        
        this->set_bounds_for_axis(Axis::X, -128, 127);
        this->set_bounds_for_axis(Axis::Y, -128, 127);
        this->set_bounds_for_axis(Axis::Z, -128, 127);
        this->set_bounds_for_axis(Axis::Rx, -128, 127);
        this->set_bounds_for_axis(Axis::Ry, -128, 127);
        this->set_bounds_for_axis(Axis::Rz, -128, 127);
        
        _last_report_time = ev_now(loop);
#undef CHECK
        return true;
    }
    
    Gamepad::~Gamepad() {
        if (_loop != NULL)
            ev_io_stop(_loop, &_watcher);
        if (_fd != -1)
            close(_fd);
    }
    
    void Gamepad::read_report_cb(struct ev_loop* loop, ev_io* watcher, int) {
        Gamepad* this_ = static_cast<Gamepad*>(watcher->data);
        assert(this_ != NULL);
        
        auto current_report_time = ev_now(loop);
        auto seconds_elapsed = current_report_time - this_->_last_report_time;
        this_->_last_report_time = current_report_time;
        
        char report_data[GP_REPORT_SIZE];
        read(watcher->fd, report_data, GP_REPORT_SIZE);
        KnownReport report (report_data);
        
        for (size_t i = 0; i < 6; ++ i) {
            if (report.axes[i] != this_->_last_report.axes[i])
                this_->set_axis_value(static_cast<Axis>(i), report.axes[i]);
        }
        this_->handle_axes_change(static_cast<long>(seconds_elapsed * 1e9));
        
        for (size_t i = 0; i < 16; ++ i) {
            if (report.buttons[i] != this_->_last_report.buttons[i])
                this_->handle_button_change(static_cast<Button>(i+1), report.buttons[i]);
        }
        
        this_->_last_report = report;
    }
    
    //## WARNING: THE FOLLOWING TWO METHODS ARE NOT TESTED! 
    //            Testing will be delayed to when I've met a device that the
    //             output format is reliably decoded. Do not use them in 
    //             productive environment.
    bool Gamepad::commit_transaction(const Transaction&) {
        return false;
    }
    
    bool Gamepad::get_features(Transaction&) {
        return false;
    }
    //## END WARNING
}

