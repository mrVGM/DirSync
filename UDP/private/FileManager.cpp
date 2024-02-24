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
}

void udp::FileEntry::Init(int id, const std::string& path)
{
	m_id = id;
	m_path = path;
}


bool udp::FileEntry::GetKB(size_t index, KB& outKB)
{
	if (!m_f)
	{
		fopen_s(&m_f, m_path.c_str(), "rb");
		fread_s(m_fileChunk.GetData(), udp::FileChunk::m_chunkKBSize * sizeof(KB), sizeof(KB), udp::FileChunk::m_chunkKBSize, m_f);
	}

	udp::FileChunk::KBPos KBPos = m_fileChunk.GetKBPos(index);
	if (KBPos == udp::FileChunk::Left)
	{
		return false;
	}

	if (KBPos == udp::FileChunk::Right)
	{
		m_fileChunk.m_startingByte += FileChunk::m_chunkKBSize * sizeof(KB);
		fseek(m_f, m_fileChunk.m_startingByte, 0);
		fread_s(m_fileChunk.GetData(), udp::FileChunk::m_chunkKBSize * sizeof(KB), sizeof(KB), udp::FileChunk::m_chunkKBSize, m_f);
	}

	return m_fileChunk.GetKB(index, outKB);
}