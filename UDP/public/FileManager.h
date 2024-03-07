#pragma once

#include "BaseObjectMeta.h"
#include "BaseObject.h"

#include "UDP.h"
#include "Common.h"

#include <string>
#include <map>
#include <mutex>

namespace udp
{
	struct FileChunk
	{
		enum GetKBResult
		{
			OK,
			ToTheLeft,
			ToTheRight
		};

		static ull m_chunkSizeInKBs;

		ull m_offsetKB = 0;
		KB* m_data = nullptr;

		FileChunk();
		~FileChunk();

		GetKBResult GetKB(KB& kb, ull offsetKB) const;
	};

	class FileEntry
	{
	private:
		std::mutex m_getDataMutex;

		void Init(int id, const std::string& path);

	public:
		int m_id = -1;
		std::string m_path;

		void* m_fHandle = nullptr;
		FileChunk* m_fileChunk = nullptr;
		
		FileEntry(int id, const std::string& path);
		~FileEntry();

		bool GetKB(KB& outKB, ull offset);
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