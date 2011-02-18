/*
 
Shared.hpp ... Shared routines for Darwin.

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

#ifndef SHARED_HPP_kiq5j0brst2wqaor
#define SHARED_HPP_kiq5j0brst2wqaor 1

#include <CoreFoundation/CoreFoundation.h>

template <typename T>
struct CFType {
    T value;
    ~CFType() { CFRelease(value); }
    CFType(const CFType&) = delete;
protected:
    CFType(T value_) : value(value_) {}
};

struct CFNumber : CFType<CFNumberRef> {
    CFNumber(int integer) : CFType(
        CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &integer)
    ) {}
};

struct CFDictionary : CFType<CFDictionaryRef> {
    CFDictionary(CFTypeRef keys[], CFTypeRef values[], int values_count) : CFType(
        CFDictionaryCreate(kCFAllocatorDefault,
            keys, values, values_count,
            &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks)
    ) {}
    
    CFDictionary(CFTypeRef key, CFTypeRef value) : CFType(
        CFDictionaryCreate(kCFAllocatorDefault,
            &key, &value, 1,
            &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks)
    ) {}
};

struct CFArray : CFType<CFArrayRef> {
    CFArray(CFTypeRef values[], int values_count) : CFType(
        CFArrayCreate(kCFAllocatorDefault,
            values, values_count, &kCFTypeArrayCallBacks)
    ) {}
    
    CFArray(CFArrayRef array) : CFType(array) {}
    
    void for_each(void* context, CFArrayApplierFunction f) const {
        CFRange range = CFRangeMake(0, CFArrayGetCount(value));
        CFArrayApplyFunction(value, range, f, context);
    }
};

struct CFMutableArray : CFType<CFMutableArrayRef> {
    CFMutableArray() : CFType(
        CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks)
    ) {}
    
    void append(CFTypeRef new_value) {
        CFArrayAppendValue(value, new_value);
    }
};

#endif
