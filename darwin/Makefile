#!/usr/bin/env gnumake -f
#
# Makefile ... Create libgamepad.dylib for Mac OS X
# 
# Copyright (c) 2011  aura Human Technology Ltd.  <rnd@auraht.com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice, 
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name of "aura Human Technology Ltd." nor the names of its
#   contributors may be used to endorse or promote products derived from this
#   software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


FRAMEWORKS=-framework CoreFoundation -framework IOKit
OBJECTS=Gamepad_Darwin.o GamepadChangedObserver_Darwin.o Timer_Darwin.o

CXX=g++
CPPFLAGS=
CXXFLAGS=-Wall -Wextra -O3
LDFLAGS=$(FRAMEWORKS) -dynamiclib 

.PHONY: all clean


all: libgamepad.dylib

clean: 
	$(RM) $(OBJECTS)

test: test.cpp
	$(CXX) -o $@ $(CXXFLAGS) -framework CoreFoundation -L. -lgamepad -iquote .. $^

test_timer: test_timer.cpp
	$(CXX) -o $@ $(CXXFLAGS) -framework CoreFoundation -L. -lgamepad -iquote .. $^

libgamepad.dylib: $(OBJECTS)
	$(CXX) -o $@ $(LDFLAGS) $^
