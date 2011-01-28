/*
 
Timer.hpp ... Platform-independent timer

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

#ifndef TIMER_HPP_ngil72o5nr7fogvi
#define TIMER_HPP_ngil72o5nr7fogvi 1

#if _WINDLL
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

namespace GP {
    class Timer {
    public:
        typedef void (*Callback)(void* self, Timer* timer);

    private:
        void* _self;
        Callback _callback;
        
    protected:
        bool _stopped;
        
        Timer(void* self, Callback callback)
            : _self(self), _callback(callback), _stopped(false) {}
            
        void handle_timer() {
            if (_callback)
                _callback(_self, this);
        }
        
        virtual void stop_impl() = 0;
              
    public:
        bool running() const { return !_stopped; }
        void stop() {
            if (!_stopped) {
                this->stop_impl();
                _stopped = true; 
            }
        }
        virtual ~Timer() {}
    
        static EXPORT Timer* create(void* self, Callback callback, int milliseconds, void* eventloop);
    };
}

#endif