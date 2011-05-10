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

#include "../Transaction.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <algorithm>
#include <mach/mach_time.h>
#include "Shared.hpp"
#include "../Gamepad.hpp"
#include "Shared.hpp"

struct IOHIDTransaction : GP::CFType<IOHIDTransactionRef> {
    IOHIDTransaction(IOHIDDeviceRef device, IOHIDTransactionDirectionType direction) : CFType(
        IOHIDTransactionCreate(kCFAllocatorDefault, device, direction, 0)
    ) {}
    
    void set(IOHIDElementRef element, IOHIDValueRef content) {
        IOHIDTransactionAddElement(value, element);
        IOHIDTransactionSetValue(value, element, content, 0);
    }
    
    void add(IOHIDElementRef element) {
        IOHIDTransactionAddElement(value, element);
    }
    
    IOHIDValueRef get(IOHIDElementRef element) const {
        return IOHIDTransactionGetValue(value, element, 0);
    }
    
    IOReturn commit() {
        return IOHIDTransactionCommit(value);
    }
};

struct IOHIDValue : GP::CFType<IOHIDValueRef> {
    IOHIDValue(IOHIDElementRef element, uint64_t timestamp, CFIndex the_value) : CFType(
        IOHIDValueCreateWithIntegerValue(kCFAllocatorDefault, element, timestamp, the_value)
    ) {}
};


namespace GP {
    void Gamepad::create_impl(void* implementation_data) {
        auto device = static_cast<IOHIDDeviceRef>(implementation_data);
        this->create_impl_impl(device);
    }
    
    void Gamepad::create_impl_impl(IOHIDDeviceRef device) {
        _last_report_time = mach_absolute_time();
        _device = device;

        CFArray elements ( IOHIDDeviceCopyMatchingElements(device, NULL, kIOHIDOptionsTypeNone) );
        CFMutableArray restricted_elements;
        
        struct Ctx {
            Gamepad* gamepad;
            CFMutableArray& restricted_elements;
        } ctx = {this, restricted_elements};
        
        elements.for_each(&ctx, [](const void* value, void* context) {
            auto ctx = static_cast<Ctx*>(context);
            IOHIDElementRef element = static_cast<IOHIDElementRef>(const_cast<void*>(value));
            
            int usage_page = IOHIDElementGetUsagePage(element);
            int usage = IOHIDElementGetUsage(element);
            Axis axis = axis_from_usage(usage_page, usage);
            auto elem_type = IOHIDElementGetType(element);
            
            if (axis != Axis::invalid) {
                ctx->gamepad->set_bounds_for_axis(axis, IOHIDElementGetLogicalMin(element), IOHIDElementGetLogicalMax(element));
            } else if (elem_type != kIOHIDElementTypeInput_Button) {
                if (elem_type == kIOHIDElementTypeOutput || elem_type == kIOHIDElementTypeFeature) {
                    int compiled_usage = usage_page << 16 | usage;
                    auto the_map = (elem_type == kIOHIDElementTypeOutput
                                        ? ctx->gamepad->_valid_output_elements
                                        : ctx->gamepad->_valid_feature_elements);
                    // ^ note: the parenthesis is necessary to make the_map an rvalue reference.
                    the_map.insert({compiled_usage, element});
                }
                element = NULL;
            }
            
            if (element != NULL) {
                CFNumber cookie (IOHIDElementGetCookie(element));
                CFDictionary dictionary(CFSTR(kIOHIDElementCookieKey), cookie.value);
                ctx->restricted_elements.append(dictionary.value);
            }
        });
        
        IOHIDDeviceSetInputValueMatchingMultiple(device, restricted_elements.value);
        
        IOHIDDeviceRegisterInputValueCallback(device, &handle_input_value, this);
        IOHIDDeviceRegisterInputReportCallback(device, NULL, 0, &handle_report, this);
        
    }
    
    void Gamepad::handle_input_value(void* context, IOReturn, void*, IOHIDValueRef value) {
        Gamepad* this_ = static_cast<Gamepad*>(context);
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
        
    void Gamepad::handle_report(void* context, IOReturn, void*, IOHIDReportType, uint32_t, uint8_t*, CFIndex) {
        // Ref: http://developer.apple.com/library/mac/#qa/qa2004/qa1398.html
        static mach_timebase_info_data_t timebase_info;
        if (!timebase_info.denom)
            mach_timebase_info(&timebase_info);

        Gamepad* this_ = static_cast<Gamepad*>(context);
        
        auto time_now = mach_absolute_time();
        auto timestamp_elapsed = time_now - this_->_last_report_time;
        unsigned nanoseconds_elapsed = timestamp_elapsed * timebase_info.numer / timebase_info.denom;
        this_->_last_report_time = time_now;
        
        this_->handle_axes_change(nanoseconds_elapsed);
    }
    
    
    //## WARNING: THE FOLLOWING TWO METHODS ARE NOT TESTED! 
    //            Testing will be delayed to when I've met a device that the
    //             output format is reliably decoded. Do not use them in 
    //             productive environment.
    bool Gamepad::commit_transaction(const Transaction& transaction) {
        IOHIDTransaction trans (_device, kIOHIDTransactionDirectionTypeOutput);
        auto abs_time = mach_absolute_time();        
        
        struct TransactionAppender {
            uint64_t abs_time;
            const std::unordered_map<int, IOHIDElementRef>& valid_elements;
            std::unordered_map<int, IOHIDElementRef>::const_iterator invalid_element_it;
            IOHIDTransaction& trans;
            
            void operator() (const Transaction::Value& elem) {
                auto cit = valid_elements.find(elem.usage_page << 16 | elem.usage);
                if (cit != invalid_element_it) {
                    IOHIDValue value (cit->second, abs_time, elem.value);
                    trans.set(cit->second, value.value);
                }
            }
            
            TransactionAppender(IOHIDTransaction& transaction_, const std::unordered_map<int, IOHIDElementRef>& elements, uint64_t abs_time_)
                : abs_time{abs_time_}, valid_elements(elements), invalid_element_it{elements.cend()}, trans(transaction_) {}
        
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
                
        IOReturn retval = trans.commit();
        return retval == kIOReturnSuccess;
    }
    
    bool Gamepad::get_features(Transaction& transaction) {
        IOHIDTransaction trans (_device, kIOHIDTransactionDirectionTypeInput);
        
        std::for_each(_valid_feature_elements.cbegin(), _valid_feature_elements.cend(),
            [&trans](const std::pair<int, IOHIDElementRef>& elem) {
                trans.add(elem.second);
            }
        );
        
        IOReturn retval = trans.commit();
        bool succeed = (retval == kIOReturnSuccess);
        
        if (succeed) {
            std::for_each(_valid_feature_elements.cbegin(), _valid_feature_elements.cend(),
                [&trans, &transaction](const std::pair<int, IOHIDElementRef>& elem) {
                    IOHIDValueRef value = trans.get(elem.second);
                    if (value) {
                        long intval = IOHIDValueGetIntegerValue(value);
                        int usage_page = (elem.first >> 16) & 0xffff;
                        int usage = elem.first & 0xffff;
                        transaction.set_feature_value(usage_page, usage, intval);
                    }
                }
            );
        }
        
        return succeed;
    }
    //## END WARNING

    Gamepad::~Gamepad() {}
}
