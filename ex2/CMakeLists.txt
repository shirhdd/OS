cmake_minimum_required(VERSION 3.18)
project(ex2)

set(CMAKE_CXX_STANDARD 14)

add_executable(ex2 tests_amit.cpp uthreads.cpp thread.cpp)

#cmake_minimum_required(VERSION 3.1)
#set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
#set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
#project(threads VERSION 1.0 LANGUAGES C CXX)
#
#add_library(uthreads uthreads.h uthreads.cpp thread.cpp thread.h)
#
#set_property(TARGET uthreads PROPERTY CXX_STANDARD 11)
#target_compile_options(uthreads PUBLIC -Wall -Wextra)
#
#add_subdirectory(tests)

