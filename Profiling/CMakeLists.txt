cmake_minimum_required (VERSION 3.8)

add_library (Profiling
	"private/ProfilerMeta.h"
	"private/ProfilerMeta.cpp"
	"private/Profiler.h"
	"private/Profiler.cpp"
	"private/ProfilerJobSystemMeta.h"
	"private/ProfilerJobSystemMeta.cpp"
	"public/Profiling.h"
	"private/Profiling.cpp"
)

target_include_directories(Profiling INTERFACE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
target_include_directories(Profiling PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/private")

target_link_libraries(Profiling INTERFACE PUBLIC Base Jobs)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Profiling PROPERTY CXX_STANDARD 20)
endif()
