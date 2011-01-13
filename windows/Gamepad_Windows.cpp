/*

Gamepad_Windows.cpp ... Implementation of Gamepad for Windows.

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

#include "Gamepad_Windows.hpp"
#include <vector>
#include <algorithm>
#include <iterator>

// This stuff is to avoid the need to install the WDK just to get hid.lib.
static const struct HIDDLL {
    HMODULE _module;

#define HID_DECLARE(fname) decltype(&::fname) fname
#define HID_DEFINE(fname) fname = reinterpret_cast<decltype(fname)>(GetProcAddress(_module, #fname))
#define HID_PERFORM(MACRO) \
    MACRO(HidP_GetCaps); \
    MACRO(HidP_GetSpecificValueCaps); \
    MACRO(HidP_GetUsageValue); \
    MACRO(HidP_MaxUsageListLength); \
    MACRO(HidP_GetUsages)


    HID_PERFORM(HID_DECLARE);

    HIDDLL() {
        memset(this, 0, sizeof(*this));
        _module = LoadLibrary("hid.dll");
        if (_module) {
            HID_PERFORM(HID_DEFINE);
        }
    }

#undef HID_PERFORM
#undef HID_DECLARE
#undef HID_DEFINE

    ~HIDDLL() {
        if (_module)
            FreeLibrary(_module);
    }
} hid;

namespace GP {
    Gamepad_Windows::Gamepad_Windows(HANDLE hdevice) : Gamepad(), _hdevice(hdevice) {
        memset(_previous_axis_values, 0, sizeof(_previous_axis_values));

        UINT buf_size;
        GetRawInputDeviceInfo(hdevice, RIDI_PREPARSEDDATA, NULL, &buf_size);
        _preparsed_buf.resize(buf_size);
        _preparsed = reinterpret_cast<PHIDP_PREPARSED_DATA>(&_preparsed_buf[0]);

        GetRawInputDeviceInfo(hdevice, RIDI_PREPARSEDDATA, _preparsed, &buf_size);

        HIDP_CAPS caps;
        hid.HidP_GetCaps(_preparsed, &caps);

        std::vector<HIDP_VALUE_CAPS> value_caps (caps.NumberInputValueCaps);
        ULONG actual_value_caps_count = caps.NumberInputValueCaps;
        hid.HidP_GetValueCaps(HidP_Input, &value_caps[0], &actual_value_caps_count, _preparsed);

        std::unordered_set<Axis> valid_axes;
        std::for_each(value_caps.cbegin(), value_caps.cend(), [this, &valid_axes](const HIDP_VALUE_CAPS& k) {
            if (k.UsagePage == HID_USAGE_PAGE_GENERIC) {
                Axis axis = axis_from_usage(k.NotRange.Usage);
                valid_axes.insert(axis);
                this->set_bounds_for_axis(axis, k.LogicalMin, k.LogicalMax);
                _previous_axis_values[to_index(axis)] = this->axis_bound(axis);
            }
        });
        std::copy(valid_axes.cbegin(), valid_axes.cend(), std::back_inserter(_axes));

        _buttons_count = hid.HidP_MaxUsageListLength(HidP_Input, HID_USAGE_PAGE_BUTTON, _preparsed);
    }

    void Gamepad_Windows::handle_raw_input(const RAWHID& input) {
        auto report = reinterpret_cast<PCHAR>(const_cast<BYTE*>(input.bRawData));
        auto report_size = input.dwSizeHid;
        if (input.dwCount != 1) {
            /// TODO: Handle error when the report count is not 1.
        }

        std::for_each(_axes.cbegin(), _axes.cend(), [this, report, report_size](Axis axis) {
            ULONG usage_value = 0;
            if (hid.HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, axis, &usage_value, _preparsed, report, report_size) == HIDP_STATUS_SUCCESS) {
                int index = to_index(axis);
                if (_previous_axis_values[index] != usage_value) {
                    this->handle_axis_change(axis, usage_value);
                    _previous_axis_values[index] = usage_value;
                }
            }
        });

        std::vector<USAGE> active_buttons_vector (_buttons_count);
        ULONG active_buttons_count = _buttons_count;
        hid.HidP_GetUsages(HidP_Input, HID_USAGE_PAGE_BUTTON, 0, &active_buttons_vector[0], &active_buttons_count, _preparsed, report, report_size);
        std::unordered_set<USAGE> active_buttons (active_buttons_vector.cbegin(), active_buttons_vector.cbegin() + active_buttons_count);

        std::for_each(active_buttons.cbegin(), active_buttons.cend(), [this](USAGE usage) {
            if (_previous_active_buttons.find(usage) == _previous_active_buttons.cend()) {
                this->handle_button_change(usage, true);
            }
        });
        std::for_each(_previous_active_buttons.cbegin(), _previous_active_buttons.cend(), [this, &active_buttons](USAGE usage) {
            if (active_buttons.find(usage) == active_buttons.cend()) {
                this->handle_button_change(usage, false);
            }
        });

        _previous_active_buttons.swap(active_buttons);
    }
}

