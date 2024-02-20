#pragma once

#include "BaseObjectMeta.h"
#include "BaseObject.h"

#include <string>
#include <map>

namespace udp
{
	struct FileEntry
	{
		int m_id = -1;
		std::string m_path;
		char* m_buff = nullptr;
		
		~FileEntry();

		void Init(int id, const std::string& path);
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
		std::map<int, FileEntry> m_filesMap;

	public:
		FileManagerObject();
		virtual ~FileManagerObject();

		FileEntry& RegisterFile(int id, const std::string& path);
	};
}