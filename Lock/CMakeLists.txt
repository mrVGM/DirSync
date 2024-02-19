cmake_minimum_required (VERSION 3.8)

add_library (Lock
	"public/Lock.h"
	"private/Lock.cpp"
)

target_include_directories(Lock INTERFACE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
target_include_directories(Lock PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/private")

target_link_libraries(Lock INTERFACE PUBLIC Base)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Lock PROPERTY CXX_STANDARD 20)
endif()
