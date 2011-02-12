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
#include "hidsdi.h"
#include <Dbt.h>
#include "Shared.hpp"
#include "../Transaction.hpp"

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
    MACRO(HidP_GetUsagesEx); \
    MACRO(HidD_GetPreparsedData); \
    MACRO(HidD_FreePreparsedData); \
    MACRO(HidP_SetUsageValue); \
    MACRO(HidP_SetUsages); \
    MACRO(HidP_UnsetUsages); \
    MACRO(HidD_SetFeature); \
    MACRO(HidD_GetFeature)

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
    static const USAGE_AND_PAGE _VALID_USAGES[] = {
        {HID_USAGE_GENERIC_JOYSTICK, HID_USAGE_PAGE_GENERIC},
        {HID_USAGE_GENERIC_GAMEPAD, HID_USAGE_PAGE_GENERIC},
        {HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER, HID_USAGE_PAGE_GENERIC}
    };

    void Gamepad_Windows::destroy() {
        if (_reader_thread_handle) {
            DWORD errcode = ERROR_OBJECT_NOT_FOUND;
            if (_input_received_event)
                SetEvent(_input_received_event);
            if (_thread_exit_event)
                errcode = SignalObjectAndWait(_thread_exit_event, _reader_thread_handle, 1000, false);
            if (errcode)
                TerminateThread(_reader_thread_handle, errcode);
            CloseHandle(_reader_thread_handle);
            _reader_thread_handle = NULL;
        }
        if (_input_received_event) {
            CloseHandle(_input_received_event);
            _input_received_event = NULL;
        }
        if (_thread_exit_event) {
            CloseHandle(_thread_exit_event);
            _thread_exit_event = NULL;
        }
        if (_preparsed) {
            hid.HidD_FreePreparsedData(_preparsed);
            _preparsed = NULL;
        }
        if (_notif_handle != NULL) {
            UnregisterDeviceNotification(_notif_handle);
            _notif_handle = NULL;
        }
        if (_handle != INVALID_HANDLE_VALUE) {
            CloseHandle(_handle);
            _handle = INVALID_HANDLE_VALUE;
        }
    }

    bool Gamepad_Windows::analyze_caps(const HIDP_CAPS& caps) {
        // 4. make sure the HID class device *is* a gamepad.
        USAGE_AND_PAGE up = {caps.Usage, caps.UsagePage};
        auto cend = _VALID_USAGES + sizeof(_VALID_USAGES)/sizeof(*_VALID_USAGES);
        if (std::find(_VALID_USAGES, cend, up) == cend) {
            return false;
        }

        // 5. retrieve rest of the capabilities...

        // 5.1. input caps
        std::vector<HIDP_VALUE_CAPS> value_caps (caps.NumberInputValueCaps);
        ULONG actual_value_caps_count = caps.NumberInputValueCaps;
        hid.HidP_GetValueCaps(HidP_Input, &value_caps[0], &actual_value_caps_count, _preparsed);

        std::for_each(value_caps.cbegin(), value_caps.cend(), [this](const HIDP_VALUE_CAPS& k) {
            auto max_usage = k.IsRange ? k.Range.UsageMax : k.NotRange.Usage;
            for (auto usage = k.Range.UsageMin; usage <= max_usage; ++ usage) {
                Axis axis = axis_from_usage(k.UsagePage, usage);
                if (valid(axis)) {
                    this->set_bounds_for_axis(axis, k.LogicalMin, k.LogicalMax);
                    _AxisUsage axis_usage = {axis, k.UsagePage, usage};
                    _valid_axes.push_back(axis_usage);
                }
            }
        });

        _buttons_count = hid.HidP_MaxUsageListLength(HidP_Input, 0, _preparsed);
        _input_report_size = caps.InputReportByteLength;
        _output_report_size = caps.OutputReportByteLength;
        _feature_report_size = caps.FeatureReportByteLength;

        // 5.2. feature caps
        actual_value_caps_count = caps.NumberFeatureValueCaps;
        if (actual_value_caps_count) {
            value_caps.resize(actual_value_caps_count);
            hid.HidP_GetValueCaps(HidP_Feature, &value_caps[0], &actual_value_caps_count, _preparsed);

            std::for_each(value_caps.cbegin(), value_caps.cend(), [this](const HIDP_VALUE_CAPS& k) {
                auto max_usage = k.IsRange ? k.Range.UsageMax : k.NotRange.Usage;
                for (auto usage = k.Range.UsageMin; usage <= max_usage; ++ usage) {
                    USAGE_AND_PAGE up = {usage, k.UsagePage};
                    _valid_feature_usages.push_back(up);
                }
            });
        }

        _feature_buttons_count = hid.HidP_MaxUsageListLength(HidP_Feature, 0, _preparsed);
        return true;
    }

    Gamepad_Windows::Gamepad_Windows(HWND hwnd, const TCHAR* dev_path)
        : Gamepad(), _handle(INVALID_HANDLE_VALUE), _preparsed(NULL), _notif_handle(NULL), _thread_exit_event(NULL), _reader_thread_handle(NULL),
          _input_received_event(NULL), _hwnd(hwnd)
    {
        // 1. open a file handle to the HID class device from dev_path.
        _handle = CreateFile(dev_path, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (_handle == INVALID_HANDLE_VALUE)
            return;
        
        // 2. retrieve the preparsed data
        if (!hid.HidD_GetPreparsedData(_handle, &_preparsed) || !_preparsed) {
            this->destroy();
            return;
        }

        // 3. get capabilities from the preparsed data
        HIDP_CAPS caps;
        if (hid.HidP_GetCaps(_preparsed, &caps) != HIDP_STATUS_SUCCESS) {
            this->destroy();
            return;
        }

        if (!this->analyze_caps(caps)) {
            this->destroy();
            return;
        }
        
        // 6. Start reader thread.
        _thread_exit_event = CreateEvent(NULL, false, false, NULL);
        if (!_thread_exit_event) {
            this->destroy();
            return;
        }
        _input_received_event = CreateEvent(NULL, true, true, NULL);
        if (!_input_received_event) {
            this->destroy();
            return;
        }
        _reader_thread_handle = CreateThread(NULL, 0, Gamepad_Windows::reader_thread_entry, this, 0, NULL);
        if (!_reader_thread_handle) {
            this->destroy();
            return;
        }

        // 7. register broadcast (WM_DEVICECHANGED) to allow proper handling of unplugging device.
        if (!this->register_broadcast(hwnd)) {
            this->destroy();
            return;
        }
    }


    DWORD Gamepad_Windows::reader_thread() {
        _input_report_buffer.resize(_input_report_size);
        DWORD errcode = 0;

        LARGE_INTEGER last_counter = {0}, frequency = {0};
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&last_counter);
        //## TODO: Fallback to GetTickCount if QueryPerformanceCounter is not supported.

        while (true) {
            errcode = WaitForSingleObject(_thread_exit_event, 0);
            if (errcode != WAIT_TIMEOUT)
                break;

            DWORD bytes_read;
            auto succeed = ReadFile(_handle, &_input_report_buffer[0], _input_report_size, &bytes_read, NULL);
            if (bytes_read != _input_report_size)
                succeed = false;
            if (succeed) {
                LARGE_INTEGER counter;
                QueryPerformanceCounter(&counter);
                auto delta_c = counter.QuadPart - last_counter.QuadPart;
                last_counter = counter;
                auto nanosec = delta_c * 1000000000 / frequency.QuadPart;
                unsigned nanoseconds_elapsed = static_cast<unsigned>(nanosec);

                //this->handle_input_report(&_input_report_buffer[0], nanoseconds_elapsed);
                ResetEvent(_input_received_event);
                PostMessage(_hwnd, WM_USER + 0x493f, nanoseconds_elapsed, reinterpret_cast<LPARAM>(this));
                WaitForSingleObject(_input_received_event, INFINITE);

            }
            else {
                errcode = GetLastError();
                break;
            }
        }

        if (errcode) {
            printf("Reader thread exited with error code %d.\n", errcode);
            //## TODO: Maybe we should throw an exception?
        }
        return errcode;
    }

    void Gamepad_Windows::handle_input_report(unsigned nanoseconds_elapsed) {
        PCHAR report = &_input_report_buffer[0];

        std::for_each(_valid_axes.cbegin(), _valid_axes.cend(), [this, report](_AxisUsage axis_usage) {
            ULONG result = 0;
            if (hid.HidP_GetUsageValue(HidP_Input, axis_usage.usage_page, 0, axis_usage.usage, &result, _preparsed, report, _input_report_size) == HIDP_STATUS_SUCCESS) {
                this->set_axis_value(axis_usage.axis, result);
            }
        });

        
        std::vector<USAGE_AND_PAGE> active_buttons_vector (_buttons_count);
        ULONG active_buttons_count = _buttons_count;

        hid.HidP_GetUsagesEx(HidP_Input, 0, &active_buttons_vector[0], &active_buttons_count, _preparsed, report, _input_report_size);

        std::unordered_set<Button> active_buttons;
        std::transform(active_buttons_vector.cbegin(), active_buttons_vector.cbegin() + active_buttons_count,
            std::inserter(active_buttons, active_buttons.end()),
            [](USAGE_AND_PAGE usage) { return button_from_usage(usage.UsagePage, usage.Usage); }
        );

        SetEvent(_input_received_event);

        this->handle_axes_change(nanoseconds_elapsed);

        auto prev_end = _previous_active_buttons.cend();
        auto active_end = active_buttons.cend();
        std::for_each(active_buttons.cbegin(), active_buttons.cend(), [this, prev_end](Button button) {
            if (_previous_active_buttons.find(button) == prev_end) {
                this->handle_button_change(button, true);
            }
        });
        std::for_each(_previous_active_buttons.cbegin(), _previous_active_buttons.cend(), [this, &active_buttons, active_end](Button button) {
            if (active_buttons.find(button) == active_end) {
                this->handle_button_change(button, false);
            }
        });

        _previous_active_buttons.swap(active_buttons);
    }

    bool Gamepad_Windows::register_broadcast(HWND hwnd) {
        DEV_BROADCAST_HANDLE filter;
        filter.dbch_size = sizeof(filter);
        filter.dbch_devicetype = DBT_DEVTYP_HANDLE;
        filter.dbch_handle = _handle;

        _notif_handle = RegisterDeviceNotification(hwnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);
        return _notif_handle != NULL;
    }

    Gamepad_Windows* Gamepad_Windows::insert(HWND hwnd, const TCHAR* dev_path) {
        auto gamepad = new Gamepad_Windows(hwnd, dev_path);
        if (gamepad->_handle != INVALID_HANDLE_VALUE) {
            return gamepad;
        }
        delete gamepad;
        return NULL;
    }


    /*
    void Gamepad_Windows::handle_raw_input(const RAWHID& input) {
        auto report = reinterpret_cast<PCHAR>(const_cast<BYTE*>(input.bRawData));
        auto report_size = input.dwSizeHid;
        if (input.dwCount != 1) {
            //## TODO: Handle error when the report count is not 1.
        }

       
    }
    */

    std::unique_ptr<char[]> Gamepad_Windows::fill_output_report(HIDP_REPORT_TYPE report_type, size_t report_size, const std::vector<Transaction::Value>& values, const std::vector<Transaction::Value>& buttons) const {
        if (!report_size)
            return nullptr;

        if (values.empty() && buttons.empty())
            return nullptr;

        std::unique_ptr<char[]> report_buffer(new char[report_size]);
        PCHAR report = report_buffer.get();
        report[0] = 0;

        std::for_each(values.begin(), values.end(), [this, report, report_type, report_size](const Transaction::Value& elem) {
            hid.HidP_SetUsageValue(report_type, elem.usage_page, 0, elem.usage, elem.value, _preparsed, report, report_size);
        });

        std::for_each(buttons.begin(), buttons.end(), [this, report, report_type, report_size](const Transaction::Value& elem) {
            ULONG one = 1;
            USAGE usage = elem.usage;
            auto function = elem.value ? hid.HidP_SetUsages : hid.HidP_UnsetUsages;
            function(report_type, elem.usage_page, 0, &usage, &one, _preparsed, report, report_size);
        });

        return std::move(report_buffer);
    }

    bool Gamepad_Windows::commit_transaction(const Transaction& transaction) {
        bool succeed = true;

        auto output_buffer = this->fill_output_report(HidP_Output, _output_report_size, transaction.output_values(), transaction.output_buttons());
        if (output_buffer != nullptr) {
            DWORD actual_bytes_written;
            if (!WriteFile(_handle, output_buffer.get(), _output_report_size, &actual_bytes_written, NULL))
                succeed = false;
        }

        auto feature_buffer = this->fill_output_report(HidP_Feature, _feature_report_size, transaction.feature_values(), transaction.feature_buttons());
        if (feature_buffer != nullptr)
            if (!hid.HidD_SetFeature(_handle, feature_buffer.get(), _feature_report_size))
                succeed = false;

        return succeed;
    }

    bool Gamepad_Windows::get_features(Transaction& transaction) {
        std::unique_ptr<char[]> feature_buffer(new char[_feature_report_size]);
        auto report = feature_buffer.get();
        report[0] = 0;

        if (!hid.HidD_GetFeature(_handle, report, _feature_report_size))
            return false;

        std::for_each(_valid_feature_usages.cbegin(), _valid_feature_usages.cend(), [this, report, &transaction](const USAGE_AND_PAGE& up) {
            ULONG value;
            if (hid.HidP_GetUsageValue(HidP_Feature, up.UsagePage, 0, up.Usage, &value, _preparsed, report, _feature_report_size))
                transaction.set_feature_value(up.UsagePage, up.Usage, value);
        });

        ULONG active_buttons_count = _feature_buttons_count;
        std::vector<USAGE_AND_PAGE> active_buttons_vector (active_buttons_count);
        hid.HidP_GetUsagesEx(HidP_Feature, 0, &active_buttons_vector[0], &active_buttons_count, _preparsed, report, _feature_report_size);

        std::for_each(active_buttons_vector.cbegin(), active_buttons_vector.cbegin() + active_buttons_count, [&transaction](const USAGE_AND_PAGE& up) {
            transaction.set_feature_button(up.UsagePage, up.Usage, true);
        });

        return true;
    }
}
