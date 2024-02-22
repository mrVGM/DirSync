#include "FileManager.h"

#include "Jobs.h"
#include "Job.h"
#include "JobSystem.h"
#include "JobSystemMeta.h"

#include "BaseObjectContainer.h"


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
}

udp::FileEntry& udp::FileManagerObject::RegisterFile(int id, const std::string& path)
{
	auto it = m_filesMap.find(id);
	if (it == m_filesMap.end())
	{
		m_filesMap[id] = FileEntry();
		FileEntry& f = m_filesMap[id];
		f.Init(id, path);
		return f;
	}

	return it->second;
}

udp::FileEntry* udp::FileManagerObject::GetFile(int id)
{
	auto it = m_filesMap.find(id);
	if (it == m_filesMap.end())
	{
		return nullptr;
	}
	
	return &it->second;
}

udp::FileEntry::~FileEntry()
{
	if (m_f)
	{
		fclose(m_f);
	}

	if (m_buff)
	{
		delete[] m_buff;
	}

	m_buff = nullptr;
}

void udp::FileEntry::Init(int id, const std::string& path)
{
	m_id = id;
	m_path = path;
	m_buff = new KB[8 * 1024];
}


const udp::FileEntry::KB& udp::FileEntry::GetKB(size_t index)
{
	bool read = false;
	if (!m_f)
	{
		read = true;
		fopen_s(&m_f, m_path.c_str(), "rb");
	}

	size_t startKB = index / (8 * 1024);
	size_t curPos = 1024 * startKB;

	if (m_curPos != curPos)
	{
		read = true;
	}

	m_curPos = curPos;
	if (read)
	{
		fseek(m_f, m_curPos, 0);
	}

	fread_s(m_buff, 1024 * sizeof(KB), sizeof(KB), 1024, m_f);
	return m_buff[index - startKB];
}