# CMakeList.txt : CMake project for MemInfo, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(SOURCES
../MemInfo.cpp
../MemInfo.h
)

# Include sub-projects.
add_subdirectory ("7")
add_subdirectory ("8")

