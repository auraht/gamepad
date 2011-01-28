/*
 
Gamepad.hpp ... Represents a game controller.

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

#ifndef GAMEPAD_HPP_9aoj909c6g7am7vi
#define GAMEPAD_HPP_9aoj909c6g7am7vi 1

#include <utility>
#include <climits>

namespace GP {
    class Gamepad {        
    public:
        enum Axis {
            kAxisInvalid = -1,
            kAxisX = 0x30,
            kAxisY,
            kAxisZ,
            kAxisRx,
            kAxisRy,
            kAxisRz,
            kAxisVx = 0x40,
            kAxisVy,
            kAxisVz,
            kAxisVbrx,
            kAxisVbry,
            kAxisVbrz,
            kAxisVno
        };
        
        typedef void (*AxisChangedCallback)(void* self, Gamepad* gamepad, Axis axis, long new_value);
        typedef void (*ButtonChangedCallback)(void* self, Gamepad* gamepad, int button, bool is_pressed);
        
    protected:
        static const int kMaxAxisIndex = 13;
        
        // combine the usage page and usage into a single number for axis.
        static inline int compile_axis_usage(int usage_page, int usage) {
            return (usage_page - 1) << 16 | usage;
        }
        
        // combine the usage page and usage into a single number of button.
        static inline int compile_button_usage(int usage_page, int usage) {
            return (usage_page - 9) << 16 | usage;
        }
        
        // convert an axis to an array index.
        static inline int to_index(Axis axis) {
            if (kAxisX <= axis && axis <= kAxisRz)
                return axis - kAxisX;
            else if (kAxisVx <= axis && axis <= kAxisVno)
                return axis + 6 - kAxisVx;
            else
                return -1;
        }
        
        // convert the compiled usage into an Axis. 
        // returns kAxisInvalid if the compiled usage is not an axis we can recognize.
        static inline Axis axis_from_usage(int usage) {
            if ((kAxisX <= usage && usage <= kAxisRz) || (kAxisVx <= usage && usage <= kAxisVno))
                return static_cast<Axis>(usage);
            else
                return kAxisInvalid;
        }

    private:                
        void* _axis_changed_self;
        AxisChangedCallback _axis_changed_callback;
        void* _button_changed_self;
        ButtonChangedCallback _button_changed_callback;
        
        void* _associated_object;
        void (*_associated_deleter)(void* _object);
        
        long _centroid[kMaxAxisIndex];
        
    protected:
        void set_bounds_for_axis(Axis axis, long minimum, long maximum) {
            int index = to_index(axis);
            /// TODO: Deal with drifting?
            if (index >= 0)
                _centroid[index] = (maximum + minimum + 1) / 2;
        }

        void handle_axis_change(Axis axis, long new_value) {
            if (_axis_changed_callback) {
                int index = to_index(axis);
                if (index >= 0)
                    _axis_changed_callback(_axis_changed_self, this, axis, new_value - _centroid[index]);
            }
        }
        void handle_button_change(int button, bool is_pressed) {
            if (_button_changed_callback)
                _button_changed_callback(_button_changed_self, this, button, is_pressed);
        }
        
        Gamepad()
            : _axis_changed_self(NULL), _axis_changed_callback(NULL),
              _button_changed_self(NULL), _button_changed_callback(NULL),
              _associated_object(NULL), _associated_deleter(NULL) {}
        
    public:
        void set_axis_changed_callback(void* self, AxisChangedCallback callback) {
            _axis_changed_self = self;
            _axis_changed_callback = callback;
        }
        void set_button_changed_callback(void* self, ButtonChangedCallback callback) {
            _button_changed_self = self;
            _button_changed_callback = callback;
        }
        
        /// Return the upper limit of value the axis can take.
        long axis_bound(Axis axis) const {
            int index = to_index(axis);
            return index >= 0 ? _centroid[index] : 0;
        }
        
        template <typename T>
        static const T* axis_name(Axis);

        void associate(void* object, void (*deleter)(void*) = NULL) {
            if (_associated_deleter)
                _associated_deleter(_associated_object);
            _associated_object = object;
            _associated_deleter = deleter;
        }
        
        void* associated_object() const { return _associated_object; }
        
        virtual ~Gamepad() {
            if (_associated_deleter)
                _associated_deleter(_associated_object);
        }
    };

    template <>
    inline const char* Gamepad::axis_name(Axis axis) { 
        const char* kAxisNames[] = {
            "X", "Y", "Z", "Rx", "Ry", "Rz",
            "Vx", "Vy", "Vz", "Vbrx", "Vbry", "Vbrz", "Vno"
        };
        int index = to_index(axis);
        return index >= 0 ? kAxisNames[index] : "";
    }

    template <>
    inline const wchar_t* Gamepad::axis_name(Axis axis) { 
        const wchar_t* kAxisNames[] = {
            L"X", L"Y", L"Z", L"Rx", L"Ry", L"Rz",
            L"Vx", L"Vy", L"Vz", L"Vbrx", L"Vbry", L"Vbrz", L"Vno"
        };
        int index = to_index(axis);
        return index >= 0 ? kAxisNames[index] : L"";
    }
}

#endif