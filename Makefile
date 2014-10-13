# Makefile for hellojs

# Configuration - you'll need to change at least SM_BUILD to get this to work
# for you!
SM_LIB_VERSION = 35a1
SM_BUILD = $(CURDIR)/../gecko/js/src/d-obj
SM_INCLUDE = $(SM_BUILD)/dist/include
SM_LIB = $(SM_BUILD)/dist/lib
SM_CANARIES = $(SM_INCLUDE)/jsapi.h $(SM_LIB)/libmozjs-$(SM_LIB_VERSION).so

CXX=g++
CXXFLAGS=-std=c++11 -Wno-invalid-offsetof -I $(SM_INCLUDE) -L $(SM_LIB) -lmozjs-$(SM_LIB_VERSION)

hellojs: hellojs.cpp $(SM_CANARIES)
	$(CXX) $(CXXFLAGS) -o hellojs hellojs.cpp

