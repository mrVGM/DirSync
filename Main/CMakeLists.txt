﻿cmake_minimum_required (VERSION 3.8)

add_executable (Main
	"main.cpp"
)

target_link_libraries(Main PRIVATE
	Base
	JSONParser
	Jobs
	Lock
	PipeServer
	UDP
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Main PROPERTY CXX_STANDARD 20)
endif()
