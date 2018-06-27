# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#

set(SOURCES
../../get_size.cpp
)

# Include sub-projects.
add_subdirectory ("awe")
add_subdirectory ("lp")
