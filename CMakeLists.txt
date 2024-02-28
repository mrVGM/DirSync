# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("DirSync")

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DDEBUG=1)
ENDIF()

# Include sub-projects.
add_subdirectory ("Main")
add_subdirectory ("Base")
add_subdirectory ("Jobs")
add_subdirectory ("Lock")
add_subdirectory ("JSONParser")
add_subdirectory ("Parser")
add_subdirectory ("Profiling")
add_subdirectory ("PipeServer")
add_subdirectory ("UDP")
add_subdirectory ("Common")
