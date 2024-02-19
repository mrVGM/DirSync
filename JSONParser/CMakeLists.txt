cmake_minimum_required (VERSION 3.8)

add_library (JSONParser
	"public/JSONParser.h"
	"private/JSONParser.cpp"
	"public/JSONValue.h"
	"private/JSONValue.cpp"
	"public/JSONBuilder.h"
	"private/JSONBuilder.cpp"
)

target_include_directories(JSONParser INTERFACE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
target_include_directories(JSONParser PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/private")

target_link_libraries(JSONParser PUBLIC Parser)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET JSONParser PROPERTY CXX_STANDARD 20)
endif()
