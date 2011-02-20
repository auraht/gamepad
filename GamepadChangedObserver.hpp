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

#include "Compatibility.hpp"
#include <iterator>
#include <memory>

#ifndef _AURAHT_FRIENDS
#define _AURAHT_FRIENDS
#endif

namespace GP {
    class Gamepad;


    ENUM_CLASS GamepadState {
        attached,
        detaching
    };


    class GamepadChangedObserver {
    public:            
        typedef void (*Callback)(void* self, const GamepadChangedObserver& observer, Gamepad& gamepad, GamepadState state);
    
    private:
        struct Impl;
        struct ImplDeleter {
            EXPORT void operator() (Impl* impl) const;
        };
        
        std::unique_ptr<Impl, ImplDeleter> _impl;
        
        EXPORT void create_impl(void* eventloop);
        
        void* _self;
        Callback _callback;

    public:
        struct const_iterator : public std::iterator<std::forward_iterator_tag, Gamepad> {
        private:
            struct Impl;
            struct ImplDeleter {
                EXPORT void operator() (Impl* impl) const;
            };

            std::unique_ptr<Impl, ImplDeleter> _impl;
            const_iterator(Impl* impl) : _impl(impl) {};

        public:
            typedef std::forward_iterator_tag iterator_tag;
        
            EXPORT const_iterator& operator++();
            EXPORT const_iterator operator++(int);
            
            EXPORT Gamepad& operator*() const;
            EXPORT Gamepad& operator->() const { return **this; }
            
            const_iterator() : _impl(static_cast<Impl*>(NULL)) {}
            
            EXPORT const_iterator(const const_iterator& other);
            EXPORT const_iterator(const_iterator&& other);
            EXPORT const_iterator& operator=(const const_iterator& other);
            EXPORT const_iterator& operator=(const_iterator&& other);
            
            EXPORT bool operator==(const const_iterator& other) const;
            EXPORT bool operator!=(const const_iterator& other) const;
            
            friend class GamepadChangedObserver;
        };
        typedef const_iterator iterator;

    private:        
        void handle_event(Gamepad& gamepad, GamepadState state) const {
            if (_callback)
                _callback(_self, *this, gamepad, state);
        }
                
        friend struct Impl;
        
    public:
        GamepadChangedObserver(void* self, Callback callback, void* eventloop) : _self(self), _callback(callback) {
            this->create_impl(eventloop);
        }
        
    private:
        GamepadChangedObserver(const GamepadChangedObserver&);
        GamepadChangedObserver(GamepadChangedObserver&&);
        GamepadChangedObserver& operator=(const GamepadChangedObserver&);
        GamepadChangedObserver& operator=(GamepadChangedObserver&&);

    public:
        EXPORT const_iterator cbegin() const;
        EXPORT const_iterator cend() const;
        iterator begin() const { return this->cbegin(); }
        iterator end() const { return this->cend(); }
        
        
        /// Attach a simulated gamepad and return the attached one.
        /// returns NULL if this is not supported.
        EXPORT Gamepad* attach_simulated_gamepad();
    };
    
}

#endif
