#include "Hash.h"

#include "xxhash64.h"

#include <sstream>
#include <Windows.h>

void crypto::HashBinFile(const std::string& fileName, std::string& hash, ull& fileSize)
{
	hash = "";
	fileSize = 0;

	const size_t buffSize = 8 * 1024 * 1024;

	HANDLE fHandle = CreateFile(
		fileName.c_str(),
		GENERIC_READ,
		NULL,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	
	if (!fHandle)
	{
		return;
	}

	XXHash64 xxHash(43674);
	unsigned char* buff = new unsigned char[buffSize];

	DWORD read;
	ReadFile(
		fHandle,
		buff,
		buffSize,
		&read,
		NULL);

	fileSize = read;
	while (read > 0)
	{
		xxHash.add(buff, buffSize);

		ReadFile(
			fHandle,
			buff,
			buffSize,
			&read,
			NULL);
		fileSize += read;
	}

	CloseHandle(fHandle);
	delete[] buff;

	uint64_t res = xxHash.hash();

	std::stringstream ss;
	ss << res;
	ss >> hash ;
}
