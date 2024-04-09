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

ull udp::FileChunk::m_chunkSizeInKBs = 8 * sizeof(KB);

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

udp::FileEntry& udp::FileManagerObject::RegisterFile(int id, const std::string& path, int maxLoadedChunks)
{
	auto it = m_filesMap.find(id);
	if (it == m_filesMap.end())
	{
		FileEntry* f = new FileEntry(id, path, maxLoadedChunks);
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

udp::FileEntry::FileEntry(int id, const std::string& path, int maxLoadedChunks) :
	m_maxLoadedChunks(maxLoadedChunks)
{
	Init(id, path);
}

udp::FileEntry::~FileEntry()
{
	UnloadData();
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

	auto getNextChunk = [&]() {
		if (m_fileChunks.empty())
		{
			FileChunk* c = new FileChunk();
			c->m_offsetKB = 0;
			DWORD read;
			ReadFile(
				m_fHandle,
				c->m_data,
				FileChunk::m_chunkSizeInKBs * sizeof(KB),
				&read,
				NULL
			);
			m_fileChunks.push_back(c);
			return c;
		}

		if (m_fileChunks.size() < m_maxLoadedChunks)
		{
			FileChunk* c = new FileChunk();
			c->m_offsetKB = m_fileChunks.back()->m_offsetKB + FileChunk::m_chunkSizeInKBs;
			DWORD read;
			ReadFile(
				m_fHandle,
				c->m_data,
				FileChunk::m_chunkSizeInKBs * sizeof(KB),
				&read,
				NULL
			);
			m_fileChunks.push_back(c);
			return c;
		}

		FileChunk* c = m_fileChunks.front();
		m_fileChunks.erase(m_fileChunks.begin());
		c->m_offsetKB = m_fileChunks.back()->m_offsetKB + FileChunk::m_chunkSizeInKBs;
		DWORD read;
		ReadFile(
			m_fHandle,
			c->m_data,
			FileChunk::m_chunkSizeInKBs * sizeof(KB),
			&read,
			NULL
		);
		m_fileChunks.push_back(c);

		return c;
	};

	KB tmp;
	for (auto it = m_fileChunks.begin(); it != m_fileChunks.end(); ++it)
	{
		if (FileChunk::GetKBResult::ToTheLeft == (*it)->GetKB(tmp, offset))
		{
			return false;
		}

		if (FileChunk::GetKBResult::OK == (*it)->GetKB(tmp, offset))
		{
			outKB = tmp;
			return true;
		}
	}

	FileChunk::GetKBResult res = FileChunk::GetKBResult::ToTheLeft;
	FileChunk* c = getNextChunk();
	while (FileChunk::GetKBResult::ToTheRight == (res = c->GetKB(tmp, offset)))
	{
		c = getNextChunk();
	}

	if (res != FileChunk::GetKBResult::OK)
	{
		return false;
	}

	outKB = tmp;
	return true;
}

void udp::FileEntry::UnloadData()
{
	if (m_fHandle)
	{
		CloseHandle(m_fHandle);
	}

	for (auto it = m_fileChunks.begin(); it != m_fileChunks.end(); ++it)
	{
		delete *it;
	}

	m_fHandle = nullptr;
}