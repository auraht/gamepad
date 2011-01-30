/*
 
Gamepad.inc.cpp ... Inline code for Gamepad.hpp

Copyright (c) 2011  aura Human Technology Ltd.  <rnd@auraht.com>
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

#include <functional>

namespace std {
    template <>
    struct hash<GP::Button> : public unary_function<GP::Button, size_t> {
        size_t operator() (GP::Button button) const {
            return static_cast<size_t>(button);
        }
    };

    template <>
    struct hash<GP::Axis> : public unary_function<GP::Axis, size_t> {
        size_t operator() (GP::Axis axis) const {
            return static_cast<size_t>(axis);
        }
    };
}


namespace GP {
    
    static inline bool valid(Axis axis) {
        return axis > Axis::invalid && axis < Axis::count;
    }
    
    inline Axis axis_from_usage(int usage_page, int usage) {
        if (usage_page != 1)
            return Axis::invalid;
        else if (0x30 <= usage && usage <= 0x36)
            return static_cast<Axis>(usage - 0x30 + static_cast<int>(Axis::X));
        else if (0x40 <= usage && usage <= 0x47)
            return static_cast<Axis>(usage - 0x40 + static_cast<int>(Axis::Vx));
        else
            return Axis::invalid;
    }
    
    inline Button button_from_usage(int usage_page, int usage) {
        return static_cast<Button>((usage_page - 9) << 16 | usage);
    }
    
    inline void Gamepad::set_bounds_for_axis(Axis axis, long minimum, long maximum) {
        if (valid(axis)) {
            _cached_axis_values[static_cast<int>(axis)] = _centroid[static_cast<int>(axis)] = (maximum + minimum + 1) / 2;
        }
    }

    inline void Gamepad::set_axis_value(Axis axis, long value) {
        if (valid(axis))
            _cached_axis_values[static_cast<int>(axis)] = value;
    }

    inline void Gamepad::handle_axes_change() {
        bool has_state_callback = _axis_state_callback != NULL;
        
        if (_axis_changed_callback) {
            for (int i = 0; i < static_cast<int>(Axis::count); ++ i) {
                long value = _cached_axis_values[i] - _centroid[i];
                if (value != 0) {
                    if (has_state_callback && _old_axis_state[i] != AxisState::start_moving) {
                        _old_axis_state[i] = AxisState::start_moving;
                        _axis_state_callback(_axis_state_self, this, static_cast<Axis>(i), AxisState::start_moving);
                    }
                    _axis_changed_callback(_axis_changed_self, this, static_cast<Axis>(i), value);
                } else if (has_state_callback && _old_axis_state[i] != AxisState::stop_moving) {
                    _old_axis_state[i] = AxisState::stop_moving;
                    _axis_state_callback(_axis_state_self, this, static_cast<Axis>(i), AxisState::stop_moving);
                }
            }
        }
    }
    
    inline void Gamepad::handle_button_change(Button button, bool is_pressed) {
        if (_button_changed_callback)
            _button_changed_callback(_button_changed_self, this, button, is_pressed);
    }
    
    inline void Gamepad::set_axis_changed_callback(void* self, AxisChangedCallback callback) {
        _axis_changed_self = self;
        _axis_changed_callback = callback;
    }
    inline void Gamepad::set_axis_state_changed_callback(void* self, AxisStateChangedCallback callback) {
        _axis_state_self = self;
        _axis_state_callback = callback;
    }
    inline void Gamepad::set_button_changed_callback(void* self, ButtonChangedCallback callback) {
        _button_changed_self = self;
        _button_changed_callback = callback;
    }
    
    inline long Gamepad::axis_bound(Axis axis) const {
        return valid(axis) ? _centroid[static_cast<int>(axis)] : 0;
    }
    
    inline void Gamepad::associate(void* object, void (*deleter)(void*)) {
        if (_associated_deleter)
            _associated_deleter(_associated_object);
        _associated_object = object;
        _associated_deleter = deleter;
    }
    
    inline void* Gamepad::associated_object() const {
        return _associated_object;
    }
    
    inline Gamepad::~Gamepad() {
        if (_associated_deleter)
            _associated_deleter(_associated_object);
    }


    
    //---- some way to safely combine the following 4 functions?
    
    template <>
    inline const char* name(Axis axis) { 
        const char* kAxisNames[] = {
            "X", "Y", "Z", "Rx", "Ry", "Rz",
            "Vx", "Vy", "Vz", "Vbrx", "Vbry", "Vbrz", "Vno"
        };
        return valid(axis) ? kAxisNames[static_cast<int>(axis)] : "";
    }

    template <>
    inline const wchar_t* name(Axis axis) { 
        const wchar_t* kAxisNames[] = {
            L"X", L"Y", L"Z", L"Rx", L"Ry", L"Rz",
            L"Vx", L"Vy", L"Vz", L"Vbrx", L"Vbry", L"Vbrz", L"Vno"
        };
        return valid(axis) ? kAxisNames[static_cast<int>(axis)] : L"";
    }

    template <>
    inline const char16_t* name(Axis axis) { 
        const char16_t* kAxisNames[] = {
            u"X", u"Y", u"Z", u"Rx", u"Ry", u"Rz",
            u"Vx", u"Vy", u"Vz", u"Vbrx", u"Vbry", u"Vbrz", u"Vno"
        };
        return valid(axis) ? kAxisNames[static_cast<int>(axis)] : u"";
    }

    template <>
    inline const char32_t* name(Axis axis) { 
        const char32_t* kAxisNames[] = {
            U"X", U"Y", U"Z", U"Rx", U"Ry", U"Rz",
            U"Vx", U"Vy", U"Vz", U"Vbrx", U"Vbry", U"Vbrz", U"Vno"
        };
        return valid(axis) ? kAxisNames[static_cast<int>(axis)] : U"";
    }

}
