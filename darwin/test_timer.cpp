/*
 
test.cpp ... Test functionality of libgamepad.

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

#include <CoreFoundation/CoreFoundation.h>
#include "Timer.hpp"
#include <ctime>
#include <cstdio>

struct Context {
    CFRunLoopSourceRef rls;
    int countdown;
    CFAbsoluteTime init_time;
};


void timer_fired(void* self, GP::Timer*) {
    Context* ctx = static_cast<Context*>(self);
    printf("Timer fired (%2d/20) [Elapsed: %g]\n", ctx->countdown, CFAbsoluteTimeGetCurrent() - ctx->init_time);
    -- ctx->countdown;
    if (ctx->countdown <= 0) {
        CFRunLoopSourceSignal(ctx->rls);
    }
}

int main () {
    CFRunLoopRef rl = CFRunLoopGetMain();
        
    CFRunLoopSourceContext rlsctx;
    memset(&rlsctx, 0, sizeof(rlsctx));
    CFRunLoopSourceRef rls = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &rlsctx);
    CFRunLoopAddSource(rl, rls, kCFRunLoopDefaultMode);
    
    Context ctx = {rls, 20, CFAbsoluteTimeGetCurrent()};
    
    GP::Timer* timer = GP::Timer::create(&ctx, timer_fired, 250, rl);

    while (CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1.0, true)) {
        if (ctx.countdown <= 0)
            break;
    }
    
    CFRunLoopSourceInvalidate(rls);
    CFRelease(rls);
    
    delete timer;
    
    return 0;
}
