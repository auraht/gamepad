/*
 
GamepadChangedObserver.hpp ... Observe if a gamepad device has been changed.

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

#ifndef GAMEPAD_CHANGED_OBSERVER_HPP_rskkt3ru5raa714i
#define GAMEPAD_CHANGED_OBSERVER_HPP_rskkt3ru5raa714i 1

#ifdef _WINDLL
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

namespace GP {
    class Gamepad;

    enum class GamepadState {
        attached,
        detaching
    };

    class GamepadChangedObserver {
    public:        
    
        typedef void (*Callback)(void* self, Gamepad* gamepad, GamepadState state);
    
    private:
        void* _self;
        Callback _callback;
        
    protected:
        virtual void observe_impl() = 0;
        
        void handle_event(Gamepad* gamepad, GamepadState state) const {
            if (_callback)
                _callback(_self, gamepad, state);
        }
        
        GamepadChangedObserver(void* self, Callback callback)
            : _self(self), _callback(callback) {}
        
        static EXPORT GamepadChangedObserver* create_impl(void* self, Callback callback, void* eventloop);
        
    public:
        // remember to use 'delete' to kill the observer.
        static GamepadChangedObserver* create(void* self, Callback callback, void* eventloop) {
            GamepadChangedObserver* retval = create_impl(self, callback, eventloop);
            retval->observe_impl();
            return retval;
        }
        
        virtual ~GamepadChangedObserver() {}
    };
    
}

#endif
