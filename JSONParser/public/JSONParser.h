#pragma once

#include "symbol.h"
#include "codeSource.h"

#include <string>

namespace json_parser
{
	void Boot();
	scripting::ISymbol* Parse(scripting::CodeSource& codeSource);
}