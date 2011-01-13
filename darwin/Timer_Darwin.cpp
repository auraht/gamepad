/*
 
Timer_Darwin.cpp ... Timer in Darwin

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
#include <CoreFoundation/CoreFoundation.h>

namespace GP {
    class Timer_Darwin : public Timer {
    private:
        CFRunLoopTimerRef _timer;
        
        void stop_impl();
                
    public:
        Timer_Darwin(void* self, Callback callback) : Timer(self, callback), _timer(NULL) {}
        
        void set_timer(CFRunLoopTimerRef timer) {
            if (!_timer)
                _timer = timer;
        }
        
        ~Timer_Darwin() {
            if (_timer)
                CFRunLoopTimerInvalidate(_timer);
        }
        
        static void timer_fired(CFRunLoopTimerRef timer, void* info);
    };
    
    void Timer_Darwin::stop_impl() {
        if (_timer) {
            CFRunLoopTimerInvalidate(_timer);
            _timer = NULL;
        }   
    }
    
    void Timer_Darwin::timer_fired(CFRunLoopTimerRef, void* info) {
        static_cast<Timer_Darwin*>(info)->handle_timer();
    }
    
    Timer* Timer::create(void* self, Callback callback, int milliseconds, void* eventloop) {
        Timer_Darwin* retval = new Timer_Darwin(self, callback);
        
        CFTimeInterval interval = milliseconds / 1000.0;
        CFAbsoluteTime next_fire_date = CFAbsoluteTimeGetCurrent() + interval;
        CFRunLoopTimerContext ctx = {0, retval, NULL, NULL, NULL};
        CFRunLoopTimerRef timer = CFRunLoopTimerCreate(kCFAllocatorDefault, next_fire_date, interval, 0, 0, Timer_Darwin::timer_fired, &ctx);
        
        retval->set_timer(timer);
        
        CFRunLoopAddTimer(static_cast<CFRunLoopRef>(eventloop), timer, kCFRunLoopCommonModes);
        CFRelease(timer);
        
        return retval;
    }    
}
