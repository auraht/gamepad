/*
 
Shared.hpp ... Shared routines for Windows.

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

#ifndef SHARED_HPP_xurm394tin8hyqfr
#define SHARED_HPP_xurm394tin8hyqfr 1

#include <Windows.h>
#include "hidpi.h"
#include "../Exception.hpp"
#include <memory>
#include <vector>
#include <process.h>

namespace GP {
    enum {
        device_attached_message = WM_USER + 0x493e,
        report_arrived_message = WM_USER + 0x493f
    };

    
    /// Check that the input is not 'n'. Throws GetLastError() if so.
    template <typename T>
    static inline T checked(T input, T n = 0, DWORD pass = ERROR_SUCCESS) {
        if (input == n) {
            DWORD err = GetLastError();
            if (err != pass)
                throw SystemErrorException(err);
        }
        return input;
    }


    
    
    /// An RAII wrapper a Windows DLL handle.
    class Library {
        HMODULE _module;
        
        Library(const Library&);
        Library(Library&&);
        Library& operator=(const Library&);
        Library& operator=(Library&&);
    public:
        explicit Library(LPCTSTR path) : _module(checked(LoadLibrary(path))) {}
        ~Library() { FreeLibrary(_module); }
        
        template <typename T>
        T get(const char* proc_name) const {
            auto retval = checked( GetProcAddress(_module, proc_name) );
            return reinterpret_cast<T>(retval);
        }
    };

    
    /// An RAII wrapper around a Windows Critical Section object.
    class CriticalSection {
        CRITICAL_SECTION sect;

        CriticalSection(const CriticalSection&);
        CriticalSection(CriticalSection&&);
        CriticalSection& operator=(const CriticalSection&);
        CriticalSection& operator=(CriticalSection&&);
    public:
        CriticalSection() { InitializeCriticalSection(&sect); }
        ~CriticalSection() { DeleteCriticalSection(&sect); }

        friend class CriticalSectionLocker;
    };

    /// An RAII wrapper for locking a Windows Critical Section object. The usage is:
    ///
    ///     CriticalSection cs;
    ///     ...
    ///     {
    ///        CriticalSectionLocker locker(cs);
    ///        code_that_requires_synchronized_access();
    ///     }
    ///
    class CriticalSectionLocker {
        CriticalSection& _cs;

        CriticalSectionLocker(const CriticalSectionLocker&);
        CriticalSectionLocker(CriticalSectionLocker&&);
        CriticalSectionLocker& operator=(const CriticalSectionLocker&);
        CriticalSectionLocker& operator=(CriticalSectionLocker&&);
    public:
        explicit CriticalSectionLocker(CriticalSection& cs) : _cs(cs) { EnterCriticalSection(&cs.sect); }
        ~CriticalSectionLocker() { LeaveCriticalSection(&_cs.sect); }
    };



    /// Replace WNDPROC of a HWND with another, and return the current one.
    static inline WNDPROC replace_wndproc(HWND hwnd, WNDPROC new_wndproc) {
        LONG_PTR setval = reinterpret_cast<LONG_PTR>(new_wndproc);
        auto retval = checked( SetWindowLongPtr(hwnd, GWLP_WNDPROC, setval) );
        return reinterpret_cast<WNDPROC>(retval);
    }

    /// An RAII wrapper of an (unnamed) event.
    class Event {
        HANDLE _event;

        Event(const Event&);
        Event& operator=(const Event&);
    public:
        Event() : _event(NULL) {}
        Event(Event&& other) : _event(other._event) { other._event = NULL; }
        Event& operator=(Event&& other) { std::swap(_event, other._event); return *this; }

        explicit Event(bool manual_reset, bool initial_state)
            : _event(checked(CreateEvent(NULL, manual_reset, initial_state, NULL))) {}
        ~Event() { if (_event != NULL) CloseHandle(_event); }

        void set() { if (_event != NULL) checked(SetEvent(_event)); }
        void reset() { if (_event != NULL) checked(ResetEvent(_event)); }
        bool wait(DWORD timeout = INFINITE) { return checked(WaitForSingleObject(_event, timeout), WAIT_FAILED) == WAIT_OBJECT_0; }
        bool get() { return this->wait(0); }

        HANDLE handle() const { return _event; }
    };


    /// An RAII wrapper of a Thread.
    class Thread {
        HANDLE _thread;

        Thread(const Thread&);
        Thread& operator=(const Thread&);
    public:
        Thread() : _thread(NULL) {}
        Thread(Thread&& other) : _thread(other._thread) { other._thread = NULL; }
        Thread& operator=(Thread&& other) { std::swap(_thread, other._thread); return *this; }

        explicit Thread(unsigned(__stdcall *entry)(void*), void* args = NULL)
            : _thread(reinterpret_cast<HANDLE>(checked(_beginthreadex(NULL, 0, entry, args, 0, NULL)))) {}
        ~Thread() { if (_thread != NULL) CloseHandle(_thread); }
    };



    /// An RAII wrapper of a HANDLE created by CreateFile.
    class FileHandle {
    protected:
        HANDLE _handle;
    private:
        FileHandle(const FileHandle&);
        FileHandle& operator=(const FileHandle&);
    public:
        FileHandle() : _handle(INVALID_HANDLE_VALUE) {}
        FileHandle(FileHandle&& other) : _handle(other._handle) { other._handle = INVALID_HANDLE_VALUE; }
        FileHandle& operator=(FileHandle&& other) {
            std::swap(_handle, other._handle);
            return *this; 
        }

        explicit FileHandle(LPCTSTR path, bool async = false);
        ~FileHandle() { this->close(); }

        void close();
        HANDLE handle() const { return _handle; }
        void write(LPCVOID content, DWORD size);
    };


    /// An RAII wrapper of a Timer.
    class Timer {
        HWND _hwnd;
        UINT_PTR _timer;
        UINT _interval;
        TIMERPROC _callback;
        bool _running;

        Timer(const Timer&);
        Timer& operator=(const Timer&);
    public:
        Timer() : _timer(0), _running(false) {};
        Timer(Timer&& other)
            : _hwnd(other._hwnd), _timer(other._timer), _interval(other._interval), 
              _callback(other._callback), _running(other._running) 
        { 
            other._timer = 0; 
            other._running = false; 
        }
        Timer& operator=(Timer&& other) {
            std::swap(_hwnd, other._hwnd);
            std::swap(_timer, other._timer);
            _interval = other._interval;
            _callback = other._callback;
            std::swap(_running, other._running);
            return *this; 
        }
        ~Timer() { this->stop(); }

        explicit Timer(HWND hwnd, void* timer_id, UINT millisec_intervals, TIMERPROC callback) 
            : _hwnd(hwnd), _timer(reinterpret_cast<UINT_PTR>(timer_id)), _interval(millisec_intervals),
              _callback(callback), _running(false) {}

        void restart() {
            if (_timer) {
                checked(SetTimer(_hwnd, _timer, _interval, _callback));
                _running = true;
            }
        }
        void start() { this->restart(); }
        void stop() {
            if (_running && _timer) {
                checked(KillTimer(_hwnd, _timer));
                _running = false;
            }
        }
    };


    namespace HID {
        class Device;

        static inline void check_status(NTSTATUS input, NTSTATUS succ_code = HIDP_STATUS_SUCCESS) {
            if (input != succ_code)
                throw HIDErrorException(input);
        }

        GUID get_hid_guid();
        
        class PreparsedData {
            PHIDP_PREPARSED_DATA _preparsed;

            PreparsedData(const PreparsedData&);
            PreparsedData& operator=(const PreparsedData&);
        public:
            PreparsedData() : _preparsed(NULL) {}
            PreparsedData(PreparsedData&& other) : _preparsed(other._preparsed) { other._preparsed = NULL; }
            PreparsedData& operator=(PreparsedData&& other) {
                std::swap(_preparsed, other._preparsed);
                return *this; 
            }

            explicit PreparsedData(const Device& device);
            ~PreparsedData();

            HIDP_CAPS caps() const;
            void get_value_caps(HIDP_REPORT_TYPE report_type, ULONG count, std::vector<HIDP_VALUE_CAPS>& buffer) const;
            ULONG max_usage_list_length(HIDP_REPORT_TYPE report_type) const;
            ULONG usage_value(HIDP_REPORT_TYPE report_type, PCHAR report, ULONG report_length, USAGE usage_page, USAGE usage) const;
            void set_usage_value(HIDP_REPORT_TYPE report_type, PCHAR report, ULONG report_length, USAGE usage_page, USAGE usage, ULONG value) const;
            void get_active_usages(HIDP_REPORT_TYPE report_type, PCHAR report, ULONG report_length, USAGE_AND_PAGE* buffer, ULONG* count) const;
            void set_usage_activated(HIDP_REPORT_TYPE report_type, PCHAR report, ULONG report_length, USAGE usage_page, USAGE usage, long active) const;
        };

        class Device : public FileHandle {
        public:
            Device() : FileHandle() {}
            Device(Device&& other) { _handle = other._handle; other._handle = INVALID_HANDLE_VALUE; }
            Device& operator=(Device&& other) { std::swap(_handle, other._handle); return *this; }

            explicit Device(LPCTSTR path) : FileHandle(path) {}

            bool read(LPVOID buffer, DWORD length);
            void set_feature(PVOID content, ULONG length);
            bool get_feature(PVOID buffer, ULONG length);
        };

        class DeviceNotification {
            HDEVNOTIFY _notif;
            DeviceNotification(const DeviceNotification&);
            DeviceNotification& operator=(const DeviceNotification&);
        public:
            DeviceNotification() : _notif(NULL) {}
            DeviceNotification(DeviceNotification&& other) : _notif(other._notif) { other._notif = NULL; }
            DeviceNotification& operator=(DeviceNotification&& other) { std::swap(_notif, other._notif); return *this; }

            explicit DeviceNotification(HWND hwnd, const Device& device);
            ~DeviceNotification() { if (_notif != NULL) UnregisterDeviceNotification(_notif); }
        };
    }

}

static enum : USHORT { HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER = 8 };

static inline bool operator==(const USAGE_AND_PAGE left, const USAGE_AND_PAGE right) {
    return HidP_IsSameUsageAndPage(left, right);
}
static inline bool operator<(const USAGE_AND_PAGE left, const USAGE_AND_PAGE right) {
    if (sizeof(left) == sizeof(UINT))
        return *reinterpret_cast<const UINT*>(&left) < *reinterpret_cast<const UINT*>(&right);
    else
        return left.Usage < right.Usage || left.Usage == right.Usage && left.UsagePage < right.UsagePage;
}



template <typename InputIterator1, typename InputIterator2, typename UnaryFunctor1, typename UnaryFunctor2>
static inline void for_each_diff(
    InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2, InputIterator2 end2,
    UnaryFunctor1 action1, UnaryFunctor2 action2) 
{
    while (begin1 != end1 && begin2 != end2) {
        auto left = (*begin1);
        auto right = (*begin2);
        if (left < right) {
            action1(left);
            ++ begin1;
        } else if (right < left) {
            action2(right);
            ++ begin2;
        } else {
            ++ begin1;
            ++ begin2;
        }
    }
    while (begin1 != end1) {
        action1(*begin1);
        ++ begin1;
    }
    while (begin2 != end2) {
        action2(*begin2);
        ++ begin2;
    }
}

#endif
