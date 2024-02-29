#pragma once

#include <string>

#include "Common.h"

namespace crypto
{
	void HashBinFile(const std::string& fileName, std::string& hash, ull& fileSize);
}