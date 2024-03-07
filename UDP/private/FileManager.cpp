#include "FileManager.h"

#include "Jobs.h"
#include "Job.h"
#include "JobSystem.h"
#include "JobSystemMeta.h"

#include "BaseObjectContainer.h"

#include <Windows.h>

namespace
{
	udp::FileManagerMeta m_instance;
}

ull udp::FileChunk::m_chunkSizeInKBs = 8 * 1024;

udp::FileChunk::FileChunk()
{
	m_data = new KB[m_chunkSizeInKBs];
}

udp::FileChunk::~FileChunk()
{
	delete[] m_data;
}

udp::FileChunk::GetKBResult udp::FileChunk::GetKB(udp::KB& kb, ull offsetKB) const
{
	if (offsetKB < m_offsetKB)
	{
		return GetKBResult::ToTheLeft;
	}

	if (offsetKB >= m_offsetKB + m_chunkSizeInKBs)
	{
		return GetKBResult::ToTheRight;
	}

	kb = m_data[offsetKB - m_offsetKB];
	return GetKBResult::OK;
}

udp::FileManagerMeta::FileManagerMeta() :
	BaseObjectMeta(nullptr)
{
}

const udp::FileManagerMeta& udp::FileManagerMeta::GetInstance()
{
	return m_instance;
}

udp::FileManagerObject::FileManagerObject() :
	BaseObject(FileManagerMeta::GetInstance())
{
}

udp::FileManagerObject::~FileManagerObject()
{
	for (auto it = m_filesMap.begin(); it != m_filesMap.end(); ++it)
	{
		delete it->second;
	}
}

udp::FileEntry& udp::FileManagerObject::RegisterFile(int id, const std::string& path)
{
	auto it = m_filesMap.find(id);
	if (it == m_filesMap.end())
	{
		FileEntry* f = new FileEntry(id, path);
		m_filesMap[id] = f;
		return *f;
	}

	return *it->second;
}

udp::FileEntry* udp::FileManagerObject::GetFile(int id)
{
	auto it = m_filesMap.find(id);
	if (it == m_filesMap.end())
	{
		return nullptr;
	}
	
	return it->second;
}

udp::FileEntry::FileEntry(int id, const std::string& path)
{
	Init(id, path);
}

udp::FileEntry::~FileEntry()
{
	if (m_fHandle)
	{
		CloseHandle(m_fHandle);
	}

	if (m_fileChunk)
	{
		delete m_fileChunk;
	}
}

void udp::FileEntry::Init(int id, const std::string& path)
{
	m_id = id;
	m_path = path;
}


bool udp::FileEntry::GetKB(KB& outKB, ull offset)
{
	if (!m_fHandle)
	{
		std::wstring str(m_path.begin(), m_path.end());
		m_fHandle = CreateFile(
			m_path.c_str(),
			GENERIC_READ,
			NULL,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
	}

	if (!m_fileChunk)
	{
		m_fileChunk = new FileChunk();
		DWORD read;
		ReadFile(
			m_fHandle,
			m_fileChunk->m_data,
			FileChunk::m_chunkSizeInKBs * sizeof(KB),
			&read,
			NULL
		);
	}

	KB tmp;
	FileChunk::GetKBResult res = FileChunk::GetKBResult::ToTheLeft;
	while (FileChunk::GetKBResult::ToTheRight == (res = m_fileChunk->GetKB(tmp, offset)))
	{
		DWORD read;
		ReadFile(
			m_fHandle,
			m_fileChunk->m_data,
			FileChunk::m_chunkSizeInKBs * sizeof(KB),
			&read,
			NULL
		);
		m_fileChunk->m_offsetKB += FileChunk::m_chunkSizeInKBs;
	}

	if (res == FileChunk::GetKBResult::OK)
	{
		return false;
	}

	outKB = tmp;
	return true;
}