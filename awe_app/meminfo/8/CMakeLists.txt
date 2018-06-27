# CMakeList.txt : CMake project for MemInfo, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

add_definitions(-DWIN8)

# Add source to this project's executable.
add_executable (MemInfo_8 ${SOURCES})

# TODO: Add tests and install targets if needed.
