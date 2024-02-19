cmake_minimum_required (VERSION 3.8)

add_library (Base
	"public/BaseObject.h"
	"private/BaseObject.cpp"
	"public/BaseObjectMeta.h"
	"private/BaseObjectMeta.cpp"
	"private/BaseObjectMetaContainer.h"
	"private/BaseObjectMetaContainer.cpp"
	"public/BaseFramework.h"
	"private/BaseFramework.cpp"
	"public/BaseObjectContainer.h"
	"private/BaseObjectContainer.cpp"
	"public/BaseObjectMetaTag.h"
	"private/BaseObjectMetaTag.cpp"
)

target_include_directories(Base INTERFACE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
target_include_directories(Base PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/private")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Base PROPERTY CXX_STANDARD 20)
endif()
