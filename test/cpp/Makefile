##
## Copyright (c) 2015, Jozef Stefan Institute, Quintelligence d.o.o. and contributors
## All rights reserved.
## 
## This source code is licensed under the FreeBSD license found in the
## LICENSE file in the root directory of this source tree.
##
#
# Makefile for non-Microsoft compilers
# 	tested only on Linux, Mac OS X
#
# Prerequisites:
#	- download google test: https://code.google.com/p/googletest/
#
# Compile and run all the tests:
#	make run
#

# dependencies
LIBS += -lgtest
GLIB = ../../src/glib
include $(GLIB)/Makefile.config

## Main application file
MAIN = run-all-tests

TEST_SRCS = test-AggrOnStep.cpp

TEST_OBJS = $(TEST_SRCS:.cpp=.o)

# we test in release	
CXXFLAGS += -O0 -DEBUG

.PHONY: all run
all: $(MAIN)
run: test

# COMPILE
.cpp.o:
	$(CC) $(CXXFLAGS) -c $<

$(MAIN): $(MAIN).o $(TEST_OBJS) $(GLIB)/glib.a
	$(CC) -o $(MAIN) $^ $(LDFLAGS) $(LIBS)

.PHONY: $(GLIB)/glib.a
$(GLIB)/glib.a:
	$(MAKE) -C $(GLIB) debug

.PHONY: test
test: $(MAIN)
	./$(MAIN)

.PHONY: clean
clean:
	$(MAKE) -C $(GLIB) clean
	rm -f *.o $(MAIN)
	rm -rf test*.dat *.Err
