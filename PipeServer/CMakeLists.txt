﻿cmake_minimum_required (VERSION 3.8)

add_library (PipeServer
	"public/PipeServer.h"
	"private/PipeServer.cpp"
	"private/Hash.h"
	"private/Hash.cpp"
	"private/xxhash64.h"
)

target_include_directories(PipeServer INTERFACE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
target_include_directories(PipeServer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/private")

target_link_libraries(PipeServer PUBLIC
	Base
	Jobs
	Lock
	JSONParser
	UDP
	Common
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET PipeServer PROPERTY CXX_STANDARD 20)
endif()
