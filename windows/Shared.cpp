/*
 
Shared.cpp ... Shared routine for Windows

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

#include "Shared.hpp"

#include "hidusage.h"
#include "hidsdi.h"
#include <Windows.h>
#include <cstring>
#include <Dbt.h>

namespace GP {
    FileHandle::FileHandle(LPCTSTR path, bool async) : _handle(checked(
        CreateFile(path, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
                   NULL, OPEN_EXISTING, async ? FILE_FLAG_OVERLAPPED : 0, NULL),
        INVALID_HANDLE_VALUE, ERROR_ACCESS_DENIED)) {}
    
    void FileHandle::close() {
        if (_handle != INVALID_HANDLE_VALUE) {
            CloseHandle(_handle);
            _handle = INVALID_HANDLE_VALUE;
        }
    }

    void FileHandle::write(LPCVOID content, DWORD size) {
        if (_handle != INVALID_HANDLE_VALUE) {
            DWORD dummy;
            checked(WriteFile(_handle, content, size, &dummy, NULL));
        }
    }
    
    class HIDDLL {
        Library _lib;
        
#define HID_DECLARE(fname) decltype(&::fname) fname
#define HID_DEFINE(fname) fname = _lib.get<decltype(&::fname)>(#fname)
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
    MACRO(HidD_GetFeature); \
    MACRO(HidD_GetHidGuid)

    public:
        HID_PERFORM(HID_DECLARE);
        
        HIDDLL() : _lib("hid.dll") {
            HID_PERFORM(HID_DEFINE);
        }

#undef HID_PERFORM
#undef HID_DEFINE
#undef HID_DECLARE
        
    } hid;
    
    namespace HID {
        GUID get_hid_guid() {
            GUID guid;
            hid.HidD_GetHidGuid(&guid);
            return guid;
        }

        

        PreparsedData::PreparsedData(const Device& device) : _preparsed(NULL) {
            checked(hid.HidD_GetPreparsedData(device.handle(), &_preparsed));
        }

        PreparsedData::~PreparsedData() {
            if (_preparsed != NULL)
                hid.HidD_FreePreparsedData(_preparsed);
        }

        HIDP_CAPS PreparsedData::caps() const {
            HIDP_CAPS caps;
            check_status(hid.HidP_GetCaps(_preparsed, &caps));
            return caps;
        }

        void PreparsedData::get_value_caps(HIDP_REPORT_TYPE report_type, ULONG count, std::vector<HIDP_VALUE_CAPS>& buffer) const {
            buffer.resize(count);
            check_status(hid.HidP_GetValueCaps(report_type, &buffer[0], &count, _preparsed));
        }

        ULONG PreparsedData::max_usage_list_length(HIDP_REPORT_TYPE report_type) const {
            return hid.HidP_MaxUsageListLength(report_type, 0, _preparsed);
        }

        ULONG PreparsedData::usage_value(HIDP_REPORT_TYPE report_type, PCHAR report, ULONG report_length, USAGE usage_page, USAGE usage) const {
            ULONG res = 0;
            check_status(hid.HidP_GetUsageValue(report_type, usage_page, 0, usage, &res, _preparsed, report, report_length));
            return res;
        }

        void PreparsedData::set_usage_value(HIDP_REPORT_TYPE report_type, PCHAR report, ULONG report_length, USAGE usage_page, USAGE usage, ULONG value) const {
            check_status(hid.HidP_SetUsageValue(report_type, usage_page, 0, usage, value, _preparsed, report, report_length));
        }

        void PreparsedData::get_active_usages(HIDP_REPORT_TYPE report_type, PCHAR report, ULONG report_length, USAGE_AND_PAGE* buffer, ULONG* count) const {
            check_status(hid.HidP_GetUsagesEx(report_type, 0, buffer, count, _preparsed, report, report_length));
        }

        void PreparsedData::set_usage_activated(HIDP_REPORT_TYPE report_type, PCHAR report, ULONG report_length, USAGE usage_page, USAGE usage, long active) const {
            ULONG one = 1;
            auto function = active ? hid.HidP_SetUsages : hid.HidP_UnsetUsages;
            check_status(function(report_type, usage_page, 0, &usage, &one, _preparsed, report, report_length));
        }

        bool Device::read(LPVOID buffer, DWORD length) {
            if (_handle == INVALID_HANDLE_VALUE)
                return false;
            DWORD actual_length = 0;
            if (checked(ReadFile(_handle, buffer, length, &actual_length, NULL), FALSE, ERROR_DEVICE_NOT_CONNECTED))
                return length == actual_length;
            else
                return false;
        }

        void Device::set_feature(PVOID content, ULONG length) {
            if (_handle != INVALID_HANDLE_VALUE) {
                checked(hid.HidD_SetFeature(_handle, content, length));
            }
        }

        bool Device::get_feature(PVOID buffer, ULONG length) {
            if (_handle == INVALID_HANDLE_VALUE)
                return false;
            checked(hid.HidD_GetFeature(_handle, buffer, length));
            return true;
        }

        DeviceNotification::DeviceNotification(HWND hwnd, const Device& device) : _notif(NULL) {
            DEV_BROADCAST_HANDLE filter;
            filter.dbch_size = sizeof(filter);
            filter.dbch_devicetype = DBT_DEVTYP_HANDLE;
            filter.dbch_handle = device.handle();
            _notif = checked(RegisterDeviceNotification(hwnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE));
        }
    }



    
}

