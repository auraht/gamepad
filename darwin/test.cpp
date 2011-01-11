/*
 
test.cpp ... Test functionality of libgamepad.

Copyright (c) 2011  KennyTM~ <kennytm@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, 
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.
* Neither the name of the KennyTM~ nor the names of its contributors may be
  used to endorse or promote products derived from this software without
  specific prior written permission.

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
#include "GamepadChangedObserver.hpp"
#include "Gamepad.hpp"

struct Context {
    CFRunLoopSourceRef rls;
    bool quit;
};

void gamepad_axis_changed(void*, GP::Gamepad* gamepad, GP::Gamepad::Axis axis, long new_value) {
    printf("Gamepad %lx: Axis %s changed to %ld.\n", gamepad->local_address(), GP::Gamepad::axis_name(axis), new_value);
}

void gamepad_button_changed(void*, GP::Gamepad* gamepad, int button, bool is_pressed) {
    printf("Gamepad %lx: Button %d is %s.\n", gamepad->local_address(),button, is_pressed ? "down" : "up");
}


void gamepad_state_changed(void* self, GP::Gamepad* gamepad, GP::GamepadChangedObserver::State state) {
    if (state == GP::GamepadChangedObserver::kDetaching) {
        printf("Gamepad %lx is detached, quitting.\n", gamepad->local_address());
        
        Context* ctx = static_cast<Context*>(self);
        ctx->quit = true;
        CFRunLoopSourceSignal(ctx->rls);
    } else {
        printf("Gamepad %lx is attached.\n", gamepad->local_address());
        
        gamepad->set_axis_changed_callback(NULL, gamepad_axis_changed);
        gamepad->set_button_changed_callback(NULL, gamepad_button_changed);
    }
}

int main () {
    CFRunLoopRef rl = CFRunLoopGetMain();
        
    CFRunLoopSourceContext rlsctx;
    memset(&rlsctx, 0, sizeof(rlsctx));
    CFRunLoopSourceRef rls = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &rlsctx);
    CFRunLoopAddSource(rl, rls, kCFRunLoopDefaultMode);
    
    Context ctx = {rls, false};
    
    GP::GamepadChangedObserver* observer = GP::GamepadChangedObserver::create(&ctx, gamepad_state_changed, rl);

    while (CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1.0, true)) {
        if (ctx.quit)
            break;
    }
    
    CFRunLoopSourceInvalidate(rls);
    CFRelease(rls);
    
    delete observer;
    
    return 0;
}
