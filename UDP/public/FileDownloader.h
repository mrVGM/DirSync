#pragma once

#include "BaseObjectMeta.h"
#include "BaseObject.h"

#include "UDP.h"

#include <string>

namespace udp
{
	class FileDownloaderJSMeta : public BaseObjectMeta
	{
	public:
		static const FileDownloaderJSMeta& GetInstance();
		FileDownloaderJSMeta();
	};

	class FileDownloaderMeta : public BaseObjectMeta
	{
	public:
		static const FileDownloaderMeta& GetInstance();
		FileDownloaderMeta();
	};

	class FileDownloaderObject : public BaseObject
	{
	private:
		int m_fileId = -1;
		size_t m_fileSize = 0;
		size_t m_fileOffset = 0;
		std::string m_path;

		udp::UDPRes* m_dataReceived = nullptr;

		void* m_clientSock = nullptr;

	public:
		FileDownloaderObject(int fileId, size_t fileSize, const std::string& path);
		virtual ~FileDownloaderObject();
	};
}