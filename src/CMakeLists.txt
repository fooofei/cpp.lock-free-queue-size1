
cmake_minimum_required(VERSION 3.1)
project(conwait-test)

set(BIN_NAME test)

add_executable(${BIN_NAME} conwait.h conwait.c test.cc)
if(WIN32)
	target_compile_options(${BIN_NAME} PRIVATE /source-charset:utf-8 /execution-charset:utf-8)
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()
