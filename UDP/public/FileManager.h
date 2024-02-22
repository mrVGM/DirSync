#pragma once

#include "BaseObjectMeta.h"
#include "BaseObject.h"

#include <string>
#include <map>

namespace udp
{
	struct FileEntry
	{
		struct KB
		{
			char m_data[1024];
		};

		int m_id = -1;
		std::string m_path;

		size_t m_curPos = 0;
		KB* m_buff = nullptr;

		FILE* m_f = nullptr;
		
		~FileEntry();

		void Init(int id, const std::string& path);
		const KB& GetKB(size_t index);
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
		FileEntry* GetFile(int id);
	};
}