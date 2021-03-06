/*
 
GamepadEventloop_Darwin.cpp ... Implementation of GamepadEventloop for Darwin
                                (i.e. Mac OS X).

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

#include "Gamepad_Darwin.hpp"
#include "../Transaction.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <tr1/unordered_map>
#include <algorithm>
#include <mach/mach_time.h>

namespace GP {
    struct Ctx {
        Gamepad_Darwin* gamepad;
        CFMutableArrayRef restricted_elements;
    };
    
    void Gamepad_Darwin::collect_axis_bounds(const void* element_, void* self) {
        Ctx* ctx = static_cast<Ctx*>(self);
        IOHIDElementRef element = static_cast<IOHIDElementRef>(const_cast<void*>(element_));
        
        int usage_page = IOHIDElementGetUsagePage(element);
        int usage = IOHIDElementGetUsage(element);
        Axis axis = axis_from_usage(usage_page, usage);
        auto elem_type = IOHIDElementGetType(element);
        
        if (axis != Axis::invalid) {
            ctx->gamepad->set_bounds_for_axis(axis, IOHIDElementGetLogicalMin(element), IOHIDElementGetLogicalMax(element));
        } else if (elem_type != kIOHIDElementTypeInput_Button) {
            if (elem_type == kIOHIDElementTypeOutput || elem_type == kIOHIDElementTypeFeature) {
                int compiled_usage = usage_page << 16 | usage;
                auto the_map = (elem_type == kIOHIDElementTypeOutput ? ctx->gamepad->_valid_output_elements : ctx->gamepad->_valid_feature_elements);
                // ^ note: the parenthesis is necessary to make the_map an rvalue reference.
                the_map.insert({compiled_usage, element});
            }
            element = NULL;
        }
        
        if (element != NULL) {
            CFTypeRef key = CFSTR(kIOHIDElementCookieKey);
            IOHIDElementCookie cookie = IOHIDElementGetCookie(element);
            CFTypeRef value = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &cookie);
            CFDictionaryRef dictionary = CFDictionaryCreate(kCFAllocatorDefault, &key, &value, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
            CFRelease(value);
            
            CFArrayAppendValue(ctx->restricted_elements, dictionary);
            CFRelease(dictionary);
        }
    }
    
    void Gamepad_Darwin::handle_input_value(void* context, IOReturn, void*, IOHIDValueRef value) {
        Gamepad_Darwin* this_ = static_cast<Gamepad_Darwin*>(context);
        IOHIDElementRef element = IOHIDValueGetElement(value);
        long new_value = IOHIDValueGetIntegerValue(value);
        
        int usage = IOHIDElementGetUsage(element);
        int usage_page = IOHIDElementGetUsagePage(element);
        Axis axis = axis_from_usage(usage_page, usage);
        
        if (axis != Axis::invalid) {
            this_->set_axis_value(axis, new_value);
        } else {
            Button button = button_from_usage(usage_page, usage);
            this_->handle_button_change(button, new_value);
        }
    }
    
    void Gamepad_Darwin::handle_report(void* context, IOReturn, void*, IOHIDReportType, uint32_t, uint8_t*, CFIndex) {
        // Ref: http://developer.apple.com/library/mac/#qa/qa2004/qa1398.html
        static mach_timebase_info_data_t timebase_info;
        if (!timebase_info.denom)
            mach_timebase_info(&timebase_info);

        Gamepad_Darwin* this_ = static_cast<Gamepad_Darwin*>(context);
        
        auto time_now = mach_absolute_time();
        auto timestamp_elapsed = time_now - this_->_last_report_time;
        unsigned nanoseconds_elapsed = timestamp_elapsed * timebase_info.numer / timebase_info.denom;
        this_->_last_report_time = time_now;
        
        this_->handle_axes_change(nanoseconds_elapsed);
    }
    
    void send(int usage_page, int usage, const unsigned char* content, size_t content_size);
    void retrieve(int usage_page, int usage, unsigned char* buffer, size_t buffer_size);

        
    Gamepad_Darwin::Gamepad_Darwin(IOHIDDeviceRef device) : Gamepad(), _device{device}, _last_report_time{mach_absolute_time()} {
        CFArrayRef elements = IOHIDDeviceCopyMatchingElements(device, NULL, kIOHIDOptionsTypeNone);
        CFMutableArrayRef restricted_elements = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
        
        Ctx ctx = {this, restricted_elements};
        CFArrayApplyFunction(elements, CFRangeMake(0, CFArrayGetCount(elements)), collect_axis_bounds, &ctx);
        CFRelease(elements);
        
        IOHIDDeviceSetInputValueMatchingMultiple(device, restricted_elements);
        CFRelease(restricted_elements);
        
        IOHIDDeviceRegisterInputValueCallback(device, Gamepad_Darwin::handle_input_value, this);
        IOHIDDeviceRegisterInputReportCallback(device, NULL, 0, Gamepad_Darwin::handle_report, this);
    }
    
    //## WARNING: THE FOLLOWING TWO METHODS ARE NOT TESTED! 
    //            Testing will be delayed to when I've met a device that the
    //             output format is reliably decoded. Do not use them in 
    //             productive environment.
    bool Gamepad_Darwin::commit_transaction(const Transaction& transaction) {
        auto trans = IOHIDTransactionCreate(kCFAllocatorDefault, _device, kIOHIDTransactionDirectionTypeOutput, 0);
        
        auto abs_time = mach_absolute_time();        
        
        struct TransactionAppender {
            uint64_t abs_time;
            const std::unordered_map<int, IOHIDElementRef>& valid_elements;
            std::unordered_map<int, IOHIDElementRef>::const_iterator invalid_element_it;
            IOHIDTransactionRef trans;
            
            void operator() (const Transaction::Value& elem) {
                auto cit = valid_elements.find(elem.usage_page << 16 | elem.usage);
                if (cit != invalid_element_it) {
                    auto value = IOHIDValueCreateWithIntegerValue(kCFAllocatorDefault, cit->second, abs_time, elem.value);
                    IOHIDTransactionAddElement(trans, cit->second);
                    IOHIDTransactionSetValue(trans, cit->second, value, 0);
                    CFRelease(value);
                }
            }
            
            TransactionAppender(IOHIDTransactionRef transaction_, const std::unordered_map<int, IOHIDElementRef>& elements, uint64_t abs_time_)
                : abs_time{abs_time_}, valid_elements(elements), invalid_element_it{elements.cend()}, trans{transaction_} {}
        
        } output_appender(trans, _valid_output_elements, abs_time), 
            feature_appender(trans, _valid_feature_elements, abs_time);

        
        auto output_values = transaction.output_values();
        std::for_each(output_values.cbegin(), output_values.cend(), output_appender);
                
        auto output_buttons = transaction.output_buttons();
        std::for_each(output_buttons.cbegin(), output_buttons.cend(), output_appender);
        
        auto feature_values = transaction.feature_values();
        std::for_each(feature_values.cbegin(), feature_values.cend(), feature_appender);
        
        auto feature_buttons = transaction.feature_buttons();
        std::for_each(feature_buttons.cbegin(), feature_buttons.cend(), feature_appender);
                
        IOReturn retval = IOHIDTransactionCommit(trans);
        CFRelease(trans);
        
        return retval == kIOReturnSuccess;
    }
    
    bool Gamepad_Darwin::get_features(Transaction& transaction) {
        IOHIDTransactionRef trans = IOHIDTransactionCreate(kCFAllocatorDefault, _device, kIOHIDTransactionDirectionTypeInput, 0);
        std::for_each(_valid_feature_elements.cbegin(), _valid_feature_elements.cend(), [trans](const std::pair<int, IOHIDElementRef>& elem) {
            IOHIDTransactionAddElement(trans, elem.second);
        });
        
        IOReturn retval = IOHIDTransactionCommit(trans);
        bool succeed = (retval == kIOReturnSuccess);
        if (succeed) {
            std::for_each(_valid_feature_elements.cbegin(), _valid_feature_elements.cend(), [trans, &transaction](const std::pair<int, IOHIDElementRef>& elem) {
                IOHIDValueRef value = IOHIDTransactionGetValue(trans, elem.second, 0);
                if (value) {
                    long intval = IOHIDValueGetIntegerValue(value);
                    int usage_page = (elem.first >> 16) & 0xffff;
                    int usage = elem.first & 0xffff;
                    transaction.set_feature_value(usage_page, usage, intval);
                }
            });
        }
        
        CFRelease(trans);
        
        return succeed;
    }
    //## END WARNING
}
