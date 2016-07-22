cmake_minimum_required (VERSION 2.6)
project (Tutorial)
# The version number.
set (Tutorial_VERSION_MAJOR 1)
set (Tutorial_VERSION_MINOR 0)
 set(CMAKE_CXX_FLAGS "-Wall -g -std=c++0x -stdlib=libc++")
 
# configure a header file to pass some of the CMake settings
# to the source code

 
# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories("${PROJECT_BINARY_DIR}")
 
# add the executable
add_executable(Main main.cc)