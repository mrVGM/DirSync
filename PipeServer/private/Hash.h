#pragma once

#include <string>

#include "Common.h"

namespace crypto
{
	typedef struct {
		unsigned char data[64];
		unsigned int datalen;
		unsigned int bitlen[2];
		unsigned int state[8];
	} SHA256_CTX;

	std::string HashString(const std::string& data);

	void Init(SHA256_CTX& ctx);
	void Update(SHA256_CTX& ctx, const unsigned char* data, size_t dataSize);
	std::string Finalize(SHA256_CTX& ctx);

	void HashBinFile(const std::string& fileName, std::string& hash, ull& fileSize);
}