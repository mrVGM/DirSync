#pragma once

#include <string>

typedef unsigned long long ull;

namespace guid
{
	void CreateGUID(std::string& str);

}

namespace statics
{
	const size_t MAX_PARALLEL_DOWNLOADS = 4;
}