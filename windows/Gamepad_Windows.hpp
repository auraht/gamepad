/*
 
Gamepad_Windows.hpp ... Implementation of Gamepad for Windows.

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

#ifndef GAMEPAD_WINDOWS_HPP_hq0p8ilfgiv34n29
#define GAMEPAD_WINDOWS_HPP_hq0p8ilfgiv34n29 1

#include "../Gamepad.hpp"
#include <Windows.h>
#include <vector>
#include <unordered_set>
#include <tchar.h>
#include "hidpi.h"
#include "../Transaction.hpp"

namespace GP {
    class Gamepad_Windows : public Gamepad {
    private:
        struct _AxisUsage {
            Axis axis;
            USAGE usage_page;
            USAGE usage;
        };

        HANDLE _handle;
        PHIDP_PREPARSED_DATA _preparsed;
        HDEVNOTIFY _notif_handle;

        ULONG _buttons_count;
        std::unordered_set<Button> _previous_active_buttons;
        std::vector<_AxisUsage> _valid_axes;

        size_t _input_report_size, _output_report_size, _feature_report_size;
        HANDLE _thread_exit_event;
        HANDLE _reader_thread_handle;

        std::vector<USAGE_AND_PAGE> _valid_feature_usages;
        ULONG _feature_buttons_count;
        
        bool analyze_caps(const HIDP_CAPS& caps);

        std::unique_ptr<char[]> fill_output_report(HIDP_REPORT_TYPE report_type, size_t report_size, const std::vector<Transaction::Value>& values, const std::vector<Transaction::Value>& buttons) const;

        bool commit_transaction(const Transaction&);
        bool get_features(Transaction&);

        void destroy();
        bool register_broadcast(HWND hwnd);

        void handle_input_report(PCHAR report, unsigned nanoseconds_elapsed);
        DWORD reader_thread();
        static DWORD WINAPI reader_thread_entry(LPVOID param) {
            return static_cast<Gamepad_Windows*>(param)->reader_thread();
        }

    public:
        ~Gamepad_Windows() { this->destroy(); }
        Gamepad_Windows(const TCHAR* dev_path);

        HANDLE device_handle() const { return _handle; }

        static Gamepad_Windows* insert(HWND hwnd, const TCHAR* dev_path);
    };
}

#endif
