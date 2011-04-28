/*
 
Gamepad.cpp ... Source code for Gamepad.hpp

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

#include <functional>
#include <cstring>
#include "Gamepad.hpp"

namespace GP {
    static void no_op(void*) {}

    Gamepad::Gamepad(void* implementation_data)
        : _axis_changed_callback(NULL), _button_changed_callback(NULL), _axis_state_callback(NULL),
          _axis_group_changed_callback(NULL), _axis_group_state_changed_callback(NULL),
          _associated_object(NULL, no_op) {

        memset(_centroid, 0, sizeof(_centroid));
        memset(_bounds, 0, sizeof(_bounds));
        memset(_cached_axis_values, 0, sizeof(_cached_axis_values));
        memset(_old_axis_state, 0, sizeof(_old_axis_state));
        memset(_old_axis_group_state, 0, sizeof(_old_axis_group_state));
        
        this->create_impl(implementation_data);
    }
    
    
    void Gamepad::associate(void* object, void (*deleter)(void*)) {
        if (deleter == NULL)
            deleter = no_op;
        _associated_object = std::unique_ptr<void, void(*)(void*)>(object, deleter);
    }

    void Gamepad::handle_axes_change(unsigned nanoseconds_elapsed) {
        if (_axis_changed_callback || _axis_state_callback) {
            for (int i = 0; i < static_cast<int>(Axis::count); ++ i) {
                long value = _cached_axis_values[i];
                if (value != 0) {
                    if (_axis_state_callback && _old_axis_state[i] != AxisState::start_moving) {
                        _old_axis_state[i] = AxisState::start_moving;
                        _axis_state_callback(*this, static_cast<Axis>(i), AxisState::start_moving);
                    }
                    if (_axis_changed_callback)
                        _axis_changed_callback(*this, static_cast<Axis>(i), value, nanoseconds_elapsed);
                } else if (_axis_state_callback && _old_axis_state[i] != AxisState::stop_moving) {
                    _old_axis_state[i] = AxisState::stop_moving;
                    _axis_state_callback(*this, static_cast<Axis>(i), AxisState::stop_moving);
                }
            }
        }
        
        if (_axis_group_changed_callback || _axis_group_state_changed_callback) {
            Axis axis_groups[][7] = {
                {Axis::X, Axis::Y, Axis::Z, Axis::invalid, Axis::invalid, Axis::invalid, Axis::invalid},
                {Axis::Rx, Axis::Ry, Axis::Rz, Axis::invalid, Axis::invalid, Axis::invalid, Axis::invalid},
                {Axis::Vx, Axis::Vy, Axis::Vz, Axis::invalid, Axis::invalid, Axis::invalid, Axis::invalid},
                {Axis::Vbrx, Axis::Vbry, Axis::Vbrz, Axis::invalid, Axis::invalid, Axis::invalid, Axis::invalid},
                {Axis::X, Axis::Y, Axis::invalid, Axis::invalid, Axis::invalid, Axis::invalid, Axis::invalid},
                {Axis::X, Axis::Y, Axis::Z, Axis::Rx, Axis::Ry, Axis::Rz, Axis::invalid}
            };
            
            for (int i = 0; i < static_cast<int>(AxisGroup::group_count); ++ i) {
                long values[sizeof(*axis_groups)/sizeof(**axis_groups)];
                bool modified = false;
                for (unsigned j = 0; j < sizeof(values)/sizeof(*values); ++ j) {
                    if (axis_groups[i][j] == Axis::invalid)
                        break;
                    
                    values[j] = _cached_axis_values[static_cast<int>(axis_groups[i][j])];
                    if (values[j])
                        modified = true;
                }
                
                if (modified) {
                    if (_axis_group_state_changed_callback && _old_axis_group_state[i] != AxisState::start_moving) {
                        _old_axis_group_state[i] = AxisState::start_moving;
                        _axis_group_state_changed_callback(*this, static_cast<AxisGroup>(i), AxisState::start_moving);
                    }
                    if (_axis_group_changed_callback)
                        _axis_group_changed_callback(*this, static_cast<AxisGroup>(i), values, nanoseconds_elapsed);
                } else if (_axis_group_state_changed_callback && _old_axis_group_state[i] != AxisState::stop_moving) {
                    _old_axis_group_state[i] = AxisState::stop_moving;
                    _axis_group_state_changed_callback(*this, static_cast<AxisGroup>(i), AxisState::stop_moving);
                }
            }
        }
    }

}
