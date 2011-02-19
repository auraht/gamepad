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

#include "GamepadExtra.hpp"
#include <vector>
#include <algorithm>
#include <iterator>
#include "hidsdi.h"
#include <Dbt.h>
#include "Shared.hpp"
#include "../Transaction.hpp"
#include "../Gamepad.hpp"
#include <process.h>
#include <unordered_set>

namespace GP {
    static const USAGE_AND_PAGE _VALID_USAGES[] = {
        {HID_USAGE_GENERIC_JOYSTICK, HID_USAGE_PAGE_GENERIC},
        {HID_USAGE_GENERIC_GAMEPAD, HID_USAGE_PAGE_GENERIC},
        {HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER, HID_USAGE_PAGE_GENERIC}
    };

    struct Gamepad::Impl {
        struct _AxisUsage {
            Axis axis;
            USAGE usage_page;
            USAGE usage;
        };

        HWND _hwnd;
        HID::Device _device;
        HID::PreparsedData _preparsed;
        Gamepad* _gamepad;

        std::vector<_AxisUsage> _valid_axes;
        std::vector<USAGE_AND_PAGE> _valid_feature_usages;
        ULONG _buttons_count, _feature_buttons_count;
        USHORT _input_report_size, _output_report_size, _feature_report_size;
        
        std::vector<char> _input_report_buffer;
        std::vector<USAGE_AND_PAGE> _active_usages_buffer;
        std::vector<USAGE_AND_PAGE> _last_active_usages;
        ULONG _last_active_usages_count;

        
        Event _thread_exit_event, _input_received_event;
        Thread _reader_thread;
        HID::DeviceNotification _notif;

    public:
        HANDLE device_handle() const { return _device.handle(); }

        Impl(HWND hwnd, LPCTSTR path, Gamepad* gamepad);
        ~Impl();

        bool analyze_caps(const HIDP_CAPS& caps, Gamepad* gamepad);

        static unsigned __stdcall reader_thread_entry(void* args);
    };

    Gamepad::Impl::~Impl() {
        _input_received_event.set();
        _thread_exit_event.set();
    }

    Gamepad::Impl::Impl(HWND hwnd, LPCTSTR path, Gamepad* gamepad) : _hwnd(hwnd), _gamepad(gamepad) {
        if (path != NULL) {
            // branch for actual gamepad...

            // 1. open a file handle to the HID class device from dev_path.
            _device = HID::Device(path);
            if (_device.handle() == INVALID_HANDLE_VALUE)
                return;

            // 2. retrieve the preparsed data
            _preparsed = HID::PreparsedData(_device);

            // 3. get capabilities from the preparsed data
            HIDP_CAPS caps = _preparsed.caps();

            // 4. make sure the HID class device *is* a gamepad.
            if (!this->analyze_caps(caps, gamepad)) {
                _device.close();
                return;
            }

            // 6. Start reader thread.
            //## TODO: Is using thread a good strategy?
            //         (Let's hope there won't be dozens of input devices connected to the same system.)
            _thread_exit_event = Event(false, false);
            _input_received_event = Event(true, true);
            _reader_thread = Thread(&Gamepad::Impl::reader_thread_entry, this);

            // 7. register broadcast (WM_DEVICECHANGED) to allow proper handling of unplugging device.
            _notif = HID::DeviceNotification(hwnd, _device);

        } else {
            // branch for simulated gamepad...
        }
    }

    unsigned __stdcall Gamepad::Impl::reader_thread_entry(void* args) {
        auto impl = static_cast<Gamepad::Impl*>(args);
        DWORD errcode = 0;

        auto input_buf = &impl->_input_report_buffer[0];
        auto input_size = impl->_input_report_size;

        LARGE_INTEGER last_counter = {0}, frequency = {0};
        checked(QueryPerformanceFrequency(&frequency));
        checked(QueryPerformanceCounter(&last_counter));
        //## TODO: Fallback to GetTickCount if QueryPerformanceCounter is not supported.

        while (!impl->_thread_exit_event.get()) {
            if (!impl->_device.read(input_buf, input_size))
                break;

            LARGE_INTEGER counter;
            checked(QueryPerformanceCounter(&counter));
            auto delta_c = counter.QuadPart - last_counter.QuadPart;
            last_counter = counter;
            auto nanosec = delta_c * 1000000000 / frequency.QuadPart;
            unsigned nanoseconds_elapsed = static_cast<unsigned>(nanosec);

            impl->_input_received_event.reset();
            //## TODO: Probably there's a more efficient method than PostMessage()?
            PostMessage(impl->_hwnd, report_arrived_message, nanoseconds_elapsed, reinterpret_cast<LPARAM>(impl->_gamepad));
            impl->_input_received_event.wait();
        }
        _endthreadex(0);
        return 0;
    }

    bool Gamepad::Impl::analyze_caps(const HIDP_CAPS& caps, Gamepad* gamepad) {
        // 4. make sure the HID class device *is* a gamepad.
        USAGE_AND_PAGE up = {caps.Usage, caps.UsagePage};
        auto cend = _VALID_USAGES + sizeof(_VALID_USAGES)/sizeof(*_VALID_USAGES);
        if (std::find(_VALID_USAGES, cend, up) == cend) {
            return false;
        }

        // 5. retrieve rest of the capabilities...

        // 5.1. input caps
        std::vector<HIDP_VALUE_CAPS> value_caps;
        _preparsed.get_value_caps(HidP_Input, caps.NumberInputValueCaps, value_caps);

        std::for_each(value_caps.cbegin(), value_caps.cend(), [this, gamepad](const HIDP_VALUE_CAPS& k) {
            auto max_usage = k.IsRange ? k.Range.UsageMax : k.NotRange.Usage;
            for (auto usage = k.Range.UsageMin; usage <= max_usage; ++ usage) {
                Axis axis = axis_from_usage(k.UsagePage, usage);
                if (valid(axis)) {
                    gamepad->set_bounds_for_axis(axis, k.LogicalMin, k.LogicalMax);
                    _AxisUsage axis_usage = {axis, k.UsagePage, usage};
                    _valid_axes.push_back(axis_usage);
                }
            }
        });

        _buttons_count = _preparsed.max_usage_list_length(HidP_Input);
        _input_report_size = caps.InputReportByteLength;
        _output_report_size = caps.OutputReportByteLength;
        _feature_report_size = caps.FeatureReportByteLength;

        _input_report_buffer.resize(caps.InputReportByteLength);
        _active_usages_buffer.resize(_buttons_count);
        _last_active_usages.resize(_buttons_count);
        _last_active_usages_count = 0;

        // 5.2. feature caps
        if (caps.NumberFeatureValueCaps) {
            _preparsed.get_value_caps(HidP_Feature, caps.NumberFeatureValueCaps, value_caps);

            std::for_each(value_caps.cbegin(), value_caps.cend(), [this](const HIDP_VALUE_CAPS& k) {
                auto max_usage = k.IsRange ? k.Range.UsageMax : k.NotRange.Usage;
                for (auto usage = k.Range.UsageMin; usage <= max_usage; ++ usage) {
                    USAGE_AND_PAGE up = {usage, k.UsagePage};
                    _valid_feature_usages.push_back(up);
                }
            });
        }

        _feature_buttons_count = _preparsed.max_usage_list_length(HidP_Feature);
        return true;
    }

    void Gamepad::create_impl(void* implementation_data) {
        auto data = static_cast<GamepadData*>(implementation_data);
        _impl = new Impl(data->hwnd, data->path, this);
        data->device_handle = _impl->device_handle();
    }

    void Gamepad::destroy_impl() {
        delete _impl;
    }


    void Gamepad::perform_impl_action(void* data) {
        unsigned nanoseconds_elapsed = *static_cast<WPARAM*>(data);
        auto impl = this->_impl;

        {
            auto active_begin = &impl->_active_usages_buffer[0];
            auto last_active_begin = &impl->_last_active_usages[0];
            ULONG active_usages_count = impl->_buttons_count;

            {
                struct ContinueReadOnExit {
                    Event& read_event;
                    ContinueReadOnExit(Event& ev) : read_event(ev) {}
                    ~ContinueReadOnExit() { read_event.set(); }
                } continue_read_on_exit (impl->_input_received_event);

                PCHAR report = &impl->_input_report_buffer[0];

                std::for_each(impl->_valid_axes.cbegin(), impl->_valid_axes.cend(), [report, impl, this](Impl::_AxisUsage axis_usage) {
                    auto result = impl->_preparsed.usage_value(HidP_Input, report, impl->_input_report_size, axis_usage.usage_page, axis_usage.usage);
                    this->set_axis_value(axis_usage.axis, result);
                });

                impl->_preparsed.get_active_usages(HidP_Input, report, impl->_input_report_size, active_begin, &active_usages_count);
            }

            this->handle_axes_change(nanoseconds_elapsed);

            std::sort(active_begin, active_begin + active_usages_count);
            for_each_diff(active_begin, active_begin + active_usages_count, last_active_begin, last_active_begin + impl->_last_active_usages_count,
                [this](USAGE_AND_PAGE active_usage) {
                    this->handle_button_change(button_from_usage(active_usage.UsagePage, active_usage.Usage), true);
                },
                [this](USAGE_AND_PAGE inactive_usage) {
                    this->handle_button_change(button_from_usage(inactive_usage.UsagePage, inactive_usage.Usage), false);
                }
            );
            
            impl->_last_active_usages_count = active_usages_count;
            impl->_last_active_usages.swap(impl->_active_usages_buffer);
        }
    }

    bool fill_output_report(const HID::PreparsedData& preparsed, HIDP_REPORT_TYPE report_type, char* report, size_t report_size, const std::vector<Transaction::Value>& values, const std::vector<Transaction::Value>& buttons) {
        if (!report_size)
            return false;
        if (values.empty() && buttons.empty())
            return false;

        std::for_each(values.cbegin(), values.cend(), [report_type, report, report_size, &preparsed](const Transaction::Value& elem) {
            preparsed.set_usage_value(report_type, report, report_size, elem.usage_page, elem.usage, elem.value);
        });
        std::for_each(buttons.cbegin(), buttons.cend(), [report_type, report, report_size, &preparsed](const Transaction::Value& elem) {
            preparsed.set_usage_activated(report_type, report, report_size, elem.usage_page, elem.usage, elem.value);
        });
        return true;
    }


    bool Gamepad::commit_transaction(const Transaction& transaction) {
        std::vector<char> buf (_impl->_output_report_size);

        if (fill_output_report(_impl->_preparsed, HidP_Output, &buf[0], _impl->_output_report_size, transaction.output_values(), transaction.output_buttons())) 
            _impl->_device.write(&buf[0], _impl->_output_report_size);

        if (_impl->_feature_report_size > _impl->_output_report_size)
            buf.resize(_impl->_feature_report_size);

        if (fill_output_report(_impl->_preparsed, HidP_Feature, &buf[0], _impl->_feature_report_size, transaction.feature_values(), transaction.feature_buttons())) 
            _impl->_device.set_feature(&buf[0], _impl->_feature_report_size);
        
        return true;
    }

    bool Gamepad::get_features(Transaction& transaction) {
        std::vector<char> buf (_impl->_feature_report_size);
        if (!_impl->_device.get_feature(&buf[0], _impl->_feature_report_size))
            return false;

        PCHAR report = &buf[0];


        std::for_each(_impl->_valid_feature_usages.cbegin(), _impl->_valid_feature_usages.cend(), [this, report, &transaction](USAGE_AND_PAGE up) {
            auto value = _impl->_preparsed.usage_value(HidP_Feature, report, _impl->_feature_report_size, up.UsagePage, up.Usage);
            transaction.set_feature_value(up.UsagePage, up.Usage, value);
        });

        ULONG active_buttons_count = _impl->_feature_buttons_count;
        std::vector<USAGE_AND_PAGE> active_buttons_vector (active_buttons_count);
        auto active_buttons_begin = &active_buttons_vector[0];
        _impl->_preparsed.get_active_usages(HidP_Feature, report, _impl->_feature_report_size, active_buttons_begin, &active_buttons_count);
        std::for_each(active_buttons_begin, active_buttons_begin + active_buttons_count, [&transaction](USAGE_AND_PAGE up) {
            transaction.set_feature_button(up.UsagePage, up.Usage, true);
        });

        return true;
    }
}
