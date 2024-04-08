cmake_minimum_required (VERSION 3.8)

add_library (UDP
	"public/FileManager.h"
	"private/FileManager.cpp"
	"public/UDP.h"
	"private/UDP.cpp"
	"public/FileServer.h"
	"private/FileServer.cpp"
	"public/FileDownloader.h"
	"private/FileDownloader.cpp"
	"private/JSPool.h"
	"private/JSPool.cpp"
	"private/ChunkDownloader.h"
	"private/ChunkDownloader.cpp"
)

target_include_directories(UDP INTERFACE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
target_include_directories(UDP PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/private")

target_link_libraries(UDP PRIVATE ws2_32)

target_link_libraries(UDP PUBLIC
	Base
	Jobs
	Common
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET UDP PROPERTY CXX_STANDARD 20)
endif()
