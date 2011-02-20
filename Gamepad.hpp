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
#include <memory>
#include "Compatibility.hpp"

namespace GP {
    class Transaction;
    
    ENUM_CLASS Axis {
        invalid = -1,
        X, Y, Z, Rx, Ry, Rz,
        Vx, Vy, Vz, Vbrx, Vbry, Vbrz, Vno,
        count
    };
    
    ENUM_CLASS AxisGroup {
        translation,            // X, Y, Z
        rotation,               // Rx, Ry, Rz
        vector,                 // Vx, Vy, Vz
        body_relative_vector,   // Vbrx, Vbry, Vbrz
        translation_2d,         // X, Y
        group_count
    };
    
    ENUM_CLASS Button {
        _1 = 1, _2, _3, _4, _5, _6, _7,
        _8, _9, _10, _11, _12, _13, _14, _15,
        _16, _17, _18, _19, _20, _21, _22, _23,
        _24, _25, _26, _27, _28, _29, _30, _31,
        menu = 3<<16 | 0x40,
        play_pause = 3<<16 | 0xcd,
        volume_increase = 3<<16 | 0xe9,
        volume_decrease = 3<<16 | 0xea
    };
    
    ENUM_CLASS AxisState {
        stop_moving = 0,
        start_moving
    };
        
    static inline Button button_from_usage(int usage_page, int usage) { return static_cast<Button>((usage_page - 9) << 16 | usage); }
    static Axis axis_from_usage(int usage_page, int usage);    
    static inline bool valid(Axis axis) { return axis >= static_cast<Axis>(0) && axis < Axis::count; }
    static inline bool valid(AxisGroup axis_group) { return axis_group >= static_cast<AxisGroup>(0) && axis_group < AxisGroup::group_count; }

    template <typename T>
    static const T* name(Axis);

    template <typename T>
    static const T* name(AxisGroup);


    class Gamepad {
    public:
        typedef void (*AxisChangedCallback)(Gamepad& gamepad, Axis axis, long new_value, unsigned nanoseconds_elapsed);
        typedef void (*ButtonChangedCallback)(Gamepad& gamepad, Button button, bool is_pressed);
        typedef void (*AxisStateChangedCallback)(Gamepad& gamepad, Axis axis, AxisState state);
        
        typedef void (*AxisGroupChangedCallback)(Gamepad& gamepad, AxisGroup axis_group, long new_values[], unsigned nanoseconds_elapsed);
        typedef void (*AxisGroupStateChangedCallback)(Gamepad& gamepad, AxisGroup axis, AxisState state);
        
    private:
        struct Impl;
        struct ImplDeleter {
            void operator() (Impl* impl) const;
        };
        
        std::unique_ptr<Impl, ImplDeleter> _impl;

        void create_impl(void* implementation_data);
        void perform_impl_action(void* data);
    
    private:
        AxisChangedCallback _axis_changed_callback;
        ButtonChangedCallback _button_changed_callback;
        AxisStateChangedCallback _axis_state_callback;
        AxisGroupChangedCallback _axis_group_changed_callback;
        AxisGroupStateChangedCallback _axis_group_state_changed_callback;
        
        std::unique_ptr<void, void(*)(void*)> _associated_object;
        
        long _centroid[static_cast<int>(Axis::count)];
        long _bounds[static_cast<int>(Axis::count)];
        long _cached_axis_values[static_cast<int>(Axis::count)];
        AxisState _old_axis_state[static_cast<int>(Axis::count)];
        AxisState _old_axis_group_state[static_cast<int>(AxisGroup::group_count)];
        
    private:
        void handle_axes_change(unsigned nanoseconds_elapsed);

        void set_bounds_for_axis(Axis axis, long minimum, long maximum);        
        void handle_button_change(Button button, bool is_pressed);
        void set_axis_value(Axis axis, long value);

        friend struct Impl;
        friend class GamepadChangedObserver;
        
    private:
        Gamepad(const Gamepad&);
        Gamepad(Gamepad&&);
        Gamepad& operator=(const Gamepad&);
        Gamepad& operator=(Gamepad&&);
        
    public:
        Gamepad(void* implementation_data);
        // ^ This has to be public due to std::make_shared.
    
        void set_axis_changed_callback(AxisChangedCallback callback) { _axis_changed_callback = callback; }
        void set_axis_state_changed_callback(AxisStateChangedCallback callback) { _axis_state_callback = callback; }
        void set_button_changed_callback(ButtonChangedCallback callback) { _button_changed_callback = callback; }
        
        // These events are called when any axis in the group is changed.
        void set_axis_group_changed_callback(AxisGroupChangedCallback callback) { _axis_group_changed_callback = callback; }
        void set_axis_group_state_changed_callback(AxisGroupStateChangedCallback callback) { _axis_group_state_changed_callback = callback; }
        
        /// Return the upper limit of value the axis can take.
        long axis_bound(Axis axis) const { return valid(axis) ? _bounds[static_cast<int>(axis)] : 0; }


        //## WARNING: THE FOLLOWING TWO METHODS ARE NOT TESTED! 
        //            Testing will be delayed to when I've met a device that the
        //             output format is reliably decoded. Do not use them in 
        //             productive environment.
        
        // send output and features from a transaction.
        bool commit_transaction(const Transaction&);
        // get features from the device and encode into a transaction.
        bool get_features(Transaction&);
        
        //## END WARNING
        
        void associate(void* object, void (*deleter)(void*) = NULL);
        void* associated_object() const { return _associated_object.get(); }
    };
}

#include "Gamepad.inc.cpp"

#endif