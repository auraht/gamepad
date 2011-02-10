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
    MACRO(HidD_FreePreparsedData)


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
            if (_thread_exit_event)
                errcode = SignalObjectAndWait(_thread_exit_event, _reader_thread_handle, 1000, false);
            if (errcode)
                TerminateThread(_reader_thread_handle, errcode);
            CloseHandle(_reader_thread_handle);
            _reader_thread_handle = NULL;
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

    Gamepad_Windows::Gamepad_Windows(const TCHAR* dev_path)
        : Gamepad(), _handle(INVALID_HANDLE_VALUE), _preparsed(NULL), _notif_handle(NULL), _thread_exit_event(NULL), _reader_thread_handle(NULL)
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

        // 4. make sure the HID class device *is* a gamepad.
        USAGE_AND_PAGE up = {caps.Usage, caps.UsagePage};
        auto cend = _VALID_USAGES + sizeof(_VALID_USAGES)/sizeof(*_VALID_USAGES);
        if (std::find(_VALID_USAGES, cend, up) == cend) {
            this->destroy();
            return;
        }
        
        // 5. retrieve rest of the capabilities...
        std::vector<HIDP_VALUE_CAPS> value_caps (caps.NumberInputValueCaps);
        ULONG actual_value_caps_count = caps.NumberInputValueCaps;
        hid.HidP_GetValueCaps(HidP_Input, &value_caps[0], &actual_value_caps_count, _preparsed);

        std::for_each(value_caps.cbegin(), value_caps.cend(), [this](const HIDP_VALUE_CAPS& k) {
            Axis axis = axis_from_usage(k.UsagePage, k.NotRange.Usage);
            if (valid(axis)) {
                this->set_bounds_for_axis(axis, k.LogicalMin, k.LogicalMax);
                _AxisUsage axis_usage = {axis, k.UsagePage, k.NotRange.Usage};
                _valid_axes.push_back(axis_usage);
            }
        });

        _buttons_count = hid.HidP_MaxUsageListLength(HidP_Input, 0, _preparsed);
        _input_report_size = caps.InputReportByteLength;
        
        // 6. Start reader thread.
        _thread_exit_event = CreateEvent(NULL, false, false, NULL);
        if (!_thread_exit_event) {
            this->destroy();
            return;
        }
        _reader_thread_handle = CreateThread(NULL, 0, Gamepad_Windows::reader_thread_entry, this, 0, NULL);
        if (!_reader_thread_handle) {
            this->destroy();
            return;
        }
    }


    DWORD Gamepad_Windows::reader_thread() {
        std::unique_ptr<char[]> input_report (new char[_input_report_size]);
        DWORD errcode = 0;

        while (true) {
            errcode = WaitForSingleObject(_thread_exit_event, 0);
            if (errcode != WAIT_TIMEOUT)
                break;

            DWORD bytes_read;
            bool succeed = ReadFile(_handle, input_report.get(), _input_report_size, &bytes_read, NULL);
            if (bytes_read != _input_report_size)
                succeed = false;
            if (succeed)
                this->handle_input_report(input_report.get());
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

    void Gamepad_Windows::handle_input_report(PCHAR report) {
         std::for_each(_valid_axes.cbegin(), _valid_axes.cend(), [this, report](_AxisUsage axis_usage) {
            ULONG result = 0;
            if (hid.HidP_GetUsageValue(HidP_Input, axis_usage.usage_page, 0, axis_usage.usage, &result, _preparsed, report, _input_report_size) == HIDP_STATUS_SUCCESS) {
                this->set_axis_value(axis_usage.axis, result);
            }
        });

        this->handle_axes_change();

        std::vector<USAGE_AND_PAGE> active_buttons_vector (_buttons_count);
        ULONG active_buttons_count = _buttons_count;

        hid.HidP_GetUsagesEx(HidP_Input, 0, &active_buttons_vector[0], &active_buttons_count, _preparsed, report, _input_report_size);

        std::unordered_set<Button> active_buttons;
        std::transform(active_buttons_vector.cbegin(), active_buttons_vector.cbegin() + active_buttons_count,
            std::inserter(active_buttons, active_buttons.end()),
            [](USAGE_AND_PAGE usage) { return button_from_usage(usage.UsagePage, usage.Usage); }
        );

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
        auto gamepad = new Gamepad_Windows(dev_path);
        if (gamepad->_handle != INVALID_HANDLE_VALUE) {
            // 7. register broadcast (WM_DEVICECHANGED) to allow proper handling of unplugging device.
            if (gamepad->register_broadcast(hwnd))
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

    bool Gamepad_Windows::send(int usage_page, int usage, const void* content, size_t content_size) {
        return false;
    }

    bool Gamepad_Windows::retrieve(int usage_page, int usage, void* buffer, size_t buffer_size) {
        return false;
    }

}
