# CMakeList.txt : CMake project for MemInfo, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(SOURCES
MemInfo.cpp
MemInfo.h
)

# Add source to this project's executable.
add_executable (MemInfo ${SOURCES})

# TODO: Add tests and install targets if needed.
