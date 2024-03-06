#include "Common.h"

#include <Windows.h>

void guid::CreateGUID(std::string& str)
{
	GUID gidReference;
	HRESULT hCreateGuid = CoCreateGuid(&gidReference);

	OLECHAR* guidString;
	StringFromCLSID(gidReference, &guidString);

	std::wstring wstr(guidString);
	::CoTaskMemFree(guidString);

	str = std::string(++wstr.begin(), --wstr.end());;
}