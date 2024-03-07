#pragma once

#include "BaseObjectMeta.h"
#include "BaseObject.h"

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

	public:
		int m_id = -1;
		std::string m_path;

		void* m_fHandle = nullptr;
		
		FileEntry(int id, const std::string& path);
		~FileEntry();
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