#pragma once

#include "BaseObjectMeta.h"
#include "BaseObject.h"

#include "UDP.h"

#include <functional>
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
		int m_toFinish = 2;
		bool m_done = false;

		int m_fileId = -1;
		size_t m_fileSize = 0;
		size_t m_bytesReceived = 0;
		size_t m_fileKBOffset = 0;
		std::string m_path;

		udp::UDPRes* m_dataReceived = nullptr;

		void* m_clientSock = nullptr;

		std::function<void()> m_downloadFinished;

	public:
		FileDownloaderObject(const std::string& ipAddr, int fileId, size_t fileSize, const std::string& path, const std::function<void()>& downloadFinished);
		virtual ~FileDownloaderObject();

		int GetFileId() const;
		void GetProgress(size_t& finished, size_t& all) const;
	};
}