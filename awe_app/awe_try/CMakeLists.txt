# CMakeList.txt : CMake project for awe_try, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(SOURCES
awe_try.cpp
awe_try.h
)

# Add source to this project's executable.
add_executable (awe_try ${SOURCES})

# TODO: Add tests and install targets if needed.
