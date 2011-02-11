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
#include "GamepadChangedObserver.hpp"
#include "Gamepad.hpp"

struct Context {
    CFRunLoopSourceRef rls;
    bool quit;
};

void gamepad_axis_state_changed(void*, GP::Gamepad* gamepad, GP::Axis axis, GP::AxisState state) {
    if (axis != GP::Axis::Z)
        printf("Gamepad %p: Axis %s %s.\n", gamepad, GP::name<char>(axis), state == GP::AxisState::start_moving ? "start moving" : "stop moving");
}

void gamepad_axis_changed(void*, GP::Gamepad* gamepad, GP::Axis axis, long new_value) {
    if (axis != GP::Axis::Z)
        printf("Gamepad %p: Axis %s changed to %ld.\n", gamepad, GP::name<char>(axis), new_value);
}

void gamepad_axis_group_state_changed(void*, GP::Gamepad* gamepad, GP::AxisGroup ag, GP::AxisState state) {
    if (ag != GP::AxisGroup::translation)
        printf("Gamepad %p: Axis group %s %s.\n", gamepad, GP::name<char>(ag), state == GP::AxisState::start_moving ? "start moving" : "stop moving");
}

void gamepad_axis_group_changed(void*, GP::Gamepad* gamepad, GP::AxisGroup ag, long new_values[]) {
    if (ag != GP::AxisGroup::translation)
        printf("Gamepad %p: Axis group %s changed to <%ld %ld %ld>.\n", gamepad, GP::name<char>(ag), new_values[0], new_values[1], new_values[2]);
}


void gamepad_button_changed(void*, GP::Gamepad* gamepad, GP::Button button, bool is_pressed) {
    printf("Gamepad %p: Button %d is %s.\n", gamepad, button, is_pressed ? "down" : "up");
}


void gamepad_state_changed(void* self, GP::Gamepad* gamepad, GP::GamepadState state) {
    if (state == GP::GamepadState::detaching) {
        printf("Gamepad %p is detached, quitting.\n", gamepad);
        
        Context* ctx = static_cast<Context*>(self);
        ctx->quit = true;
        CFRunLoopSourceSignal(ctx->rls);
    } else {
        printf("Gamepad %p is attached.\n", gamepad);
        
        gamepad->set_axis_changed_callback(NULL, gamepad_axis_changed);
        gamepad->set_axis_state_changed_callback(NULL, gamepad_axis_state_changed);
        gamepad->set_axis_group_changed_callback(NULL, gamepad_axis_group_changed);
        gamepad->set_axis_group_state_changed_callback(NULL, gamepad_axis_group_state_changed);
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
