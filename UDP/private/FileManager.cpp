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
}

void udp::FileEntry::Init(int id, const std::string& path)
{
	m_id = id;
	m_path = path;
}
