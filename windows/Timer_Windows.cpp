/*
 
Timer_Windows.cpp ... Timer in Windows

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

#include "../Timer.hpp"
#include <Windows.h>
#include <unordered_map>

namespace std {
    template <>
    struct hash<std::pair<HWND, UINT_PTR> > {
        std::size_t operator() (std::pair<HWND, UINT_PTR> value) const {
            size_t hash1 = std::hash<HWND>()(value.first);
            size_t hash2 = std::hash<UINT_PTR>()(value.second);
            // basically boost::hash_combine
            return (hash2 + 0x9e3779b9u + (hash1 << 6) + (hash1 >> 2)) ^ hash1;
        }
    };
}

namespace GP {
    class Timer_Windows : public Timer {
    private:
        HWND _hwnd;
    
        void stop_impl();
                
    public:
        Timer_Windows(void* self, Callback callback, HWND hwnd)
            : Timer(self, callback), _hwnd(hwnd) {}
        
        ~Timer_Windows() {
            this->stop_impl();
        }
        
        static void CALLBACK timer_fired(HWND hwnd, UINT, UINT_PTR timer_id, DWORD);
    };
    
    void Timer_Windows::stop_impl() {
        if (_hwnd) {
            auto timer_id = reinterpret_cast<UINT_PTR>(this);
            KillTimer(_hwnd, timer_id);
            _hwnd = NULL;
        }
    }
    
    void CALLBACK Timer_Windows::timer_fired(HWND hwnd, UINT, UINT_PTR timer_id, DWORD) {
        auto timer = reinterpret_cast<Timer_Windows*>(timer_id);
        timer->handle_timer();
    }
    
    __declspec(dllexport) Timer* Timer::create(void* self, Callback callback, int milliseconds, void* eventloop) {
        HWND hwnd = static_cast<HWND>(eventloop);
        auto retval = new Timer_Windows(self, callback, hwnd);

        // そんなnIDEventで大丈夫か?
        SetTimer(hwnd, reinterpret_cast<UINT_PTR>(retval), milliseconds, Timer_Windows::timer_fired);
        
        return retval;
    }    
}
