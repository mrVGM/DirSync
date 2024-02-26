#pragma once

#include "BaseObjectMeta.h"
#include "BaseObject.h"

#include "UDP.h"

#include <string>
#include <map>
#include <mutex>

namespace udp
{
	class FileEntry
	{
	private:
		std::mutex m_getDataMutex;

		void Init(int id, const std::string& path);
		bool GetKBInternal(size_t index, KB& outKB);

	public:
		int m_id = -1;
		std::string m_path;

		udp::FileChunk m_fileChunk;
		void* m_fHandle = nullptr;
		
		FileEntry(int id, const std::string& path);
		~FileEntry();

		bool GetKB(size_t index, KB& outKB);
	};


	class FileManagerMeta : public BaseObjectMeta
	{
	public:
		static const FileManagerMeta& GetInstance();
		FileManagerMeta();
	};

	class FileManagerObject : public BaseObject
	{
	private:
		std::map<int, FileEntry*> m_filesMap;

	public:
		FileManagerObject();
		virtual ~FileManagerObject();

		FileEntry& RegisterFile(int id, const std::string& path);
		FileEntry* GetFile(int id);
	};
}