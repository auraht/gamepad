/*
 
Transaction.hpp ... Output / feature transaction

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

#ifndef TRANSACTION_HPP_d94ip2x27yzaor
#define TRANSACTION_HPP_d94ip2x27yzaor 1

#include <vector>

namespace GP {
    class Transaction {
    public:
        struct Value {
            int usage_page;
            int usage;
            long value;
        };
        
    private:
        std::vector<Value> _output_values, _feature_values;
        std::vector<Value> _output_buttons, _feature_buttons;
        
    public:
        void set_output_value(int usage_page, int usage, long value) {
            Value v = {usage_page, usage, value};
            _output_values.push_back(v);
        }
        
        void set_output_button(int usage_page, int usage, bool active) {
            Value v = {usage_page, usage, active};
            _output_buttons.push_back(v);
        }
        
        void set_feature_value(int usage_page, int usage, long value) {
            Value v = {usage_page, usage, value};
            _feature_values.push_back(v);
        }
        
        void set_feature_button(int usage_page, int usage, bool active) {
            Value v = {usage_page, usage, active};
            _feature_buttons.push_back(v);
        }
        
        const std::vector<Value>& output_values() const { return _output_values; }
        const std::vector<Value>& feature_values() const { return _feature_values; }
        const std::vector<Value>& output_buttons() const { return _output_buttons; }
        const std::vector<Value>& feature_buttons() const { return _feature_buttons; }
    };
}

#endif