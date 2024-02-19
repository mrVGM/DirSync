cmake_minimum_required (VERSION 3.8)

add_library (Parser
	"private/codeSource.cpp"
	"public/codeSource.h"
	"private/grammar.cpp"
	"public/grammar.h"
	"private/parser.cpp"
	"public/parser.h"
	"private/parserState.cpp"
	"public/parserState.h"
	"private/parserTable.cpp"
	"public/parserTable.h"
	"private/rule.cpp"
	"public/rule.h"
	"private/symbol.cpp"
	"public/symbol.h"
	"private/tokenizer.cpp"
	"private/tokenizer.h"
 "private/codeSource.cpp" "private/grammar.cpp" "private/parser.cpp" "private/parserState.cpp" "private/parserTable.cpp" "private/rule.cpp" "private/symbol.cpp" "private/tokenizer.cpp" "private/tokenizer.h" "public/codeSource.h" "public/grammar.h" "public/parser.h" "public/parserState.h" "public/parserTable.h" "public/rule.h" "public/symbol.h")

target_include_directories(Parser INTERFACE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
target_include_directories(Parser PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/private")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Parser PROPERTY CXX_STANDARD 20)
endif()
