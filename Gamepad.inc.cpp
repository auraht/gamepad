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
    
    template <>
    struct hash<GP::AxisGroup> : public unary_function<GP::AxisGroup, size_t> {
        size_t operator() (GP::AxisGroup axis) const {
            return static_cast<size_t>(axis);
        }
    };
}


namespace GP {
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
    
    inline void Gamepad::set_bounds_for_axis(Axis axis, long minimum, long maximum) {
        if (valid(axis)) {
            int index = static_cast<int>(axis);
            _centroid[index] = (maximum + minimum + 1) / 2;
            _bounds[index] = (maximum - minimum + 1) / 2;
            _cached_axis_values[index] = 0;
        }
    }
    
    inline void Gamepad::handle_button_change(Button button, bool is_pressed) {
        if (_button_changed_callback)
            _button_changed_callback(*this, button, is_pressed);
    }
    
    inline void Gamepad::set_axis_value(Axis axis, long value) {
        if (valid(axis)) {
            int index = static_cast<int>(axis);
            _cached_axis_values[index] = value - _centroid[index];
        }
    }
    
    //---- some way to safely combine the following 8 functions?
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

#if __GNUC__
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
#endif

    template <>
    inline const char* name(AxisGroup axis_group) { 
        const char* kAxisNames[] = {
            "translation", "rotation", "vector", "body_relative_vector", "translation_2d"
        };
        return valid(axis_group) ? kAxisNames[static_cast<int>(axis_group)] : "";
    }

    template <>
    inline const wchar_t* name(AxisGroup axis_group) { 
        const wchar_t* kAxisNames[] = {
            L"translation", L"rotation", L"vector", L"body_relative_vector", L"translation_2d"
        };
        return valid(axis_group) ? kAxisNames[static_cast<int>(axis_group)] : L"";
    }

#if __GNUC__
    template <>
    inline const char16_t* name(AxisGroup axis_group) { 
        const char16_t* kAxisNames[] = {
            u"translation", u"rotation", u"vector", u"body_relative_vector", u"translation_2d"
        };
        return valid(axis_group) ? kAxisNames[static_cast<int>(axis_group)] : u"";
    }

    template <>
    inline const char32_t* name(AxisGroup axis_group) { 
        const char32_t* kAxisNames[] = {
            U"translation", U"rotation", U"vector", U"body_relative_vector", U"translation_2d"
        };
        return valid(axis_group) ? kAxisNames[static_cast<int>(axis_group)] : U"";
    }
#endif


}
