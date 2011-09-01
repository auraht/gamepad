/*
 
GamepadExtra.hpp ... Extra structures for implementation of Gamepad in Linux.

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

#ifndef GAMEPAD_EXTRA_HPP_85k167iqxcyjh5mi
#define GAMEPAD_EXTRA_HPP_85k167iqxcyjh5mi 1

#include <cstring>
#include <ev.h>

namespace GP {
    struct GamepadData {
        const char* path;
        struct ev_loop* loop;
        bool valid;
    };

//@@@TODO@@@: Avoid hard-coding these. Get a HID parser.
#define GP_KNOWN_DESCRIPTOR \
    "\x05\x01\x09\x04\xA1\x01\x95\x10\x75\x01\x05\x09\x19\x01\x29\x10" \
    "\x15\x00\x25\x01\x81\x02\x95\x06\x75\x08\x05\x01\x09\x30\x09\x31" \
    "\x09\x32\x09\x33\x09\x34\x09\x35\x15\x81\x25\x7F\x81\x06\xC0"
#define GP_KNOWN_DESCRIPTOR_LENGTH 0x2f
#define GP_REPORT_SIZE 8

    struct KnownReport {
        bool buttons[16];
        signed char axes[6];
        
        KnownReport() {
            memset(this, 0, sizeof(*this));
        }

        KnownReport(const char report_data[GP_REPORT_SIZE]) {
            for (size_t i = 0; i < 8; ++ i) {
                buttons[i] = report_data[0] & (1 << i);
                buttons[i+8] = report_data[1] & (1 << i);
            }
            for (size_t i = 0; i < 6; ++ i)
                axes[i] = report_data[i+2];
        }
    };
}

#endif
