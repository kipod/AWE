# CMakeList.txt : CMake project for lp_size, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

include_directories(../../..)

add_definitions(-DAWE_APP_SHOW_LARGE_PAGES -DWIN8)

# Add source to this project's executable.
add_executable (lp_size_8 ${SOURCES})

# TODO: Add tests and install targets if needed.
