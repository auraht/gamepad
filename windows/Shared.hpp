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

namespace GP {
    /// An RAII wrapper around a Windows Critical Section object.
    class CriticalSection {
        CRITICAL_SECTION sect;

        CriticalSection(const CriticalSection&);
        CriticalSection& operator=(const CriticalSection&);
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
        CriticalSectionLocker& operator=(const CriticalSectionLocker&);
    public:
        CriticalSectionLocker(CriticalSection& cs) : _cs(cs) { EnterCriticalSection(&cs.sect); }
        ~CriticalSectionLocker() { LeaveCriticalSection(&_cs.sect); }
    };
}

static enum : USHORT { HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER = 8 };

static inline bool operator== (const USAGE_AND_PAGE left, const USAGE_AND_PAGE right) {
    return HidP_IsSameUsageAndPage(left, right);
}

#endif
