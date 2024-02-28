#pragma once

#include "BaseObjectMeta.h"
#include "BaseObject.h"

#include "UDP.h"
#include "UDPClient.h"

#include <functional>
#include <string>

namespace udp
{
	class FileDownloaderMeta : public BaseObjectMeta
	{
	public:
		static const FileDownloaderMeta& GetInstance();
		FileDownloaderMeta();
	};

	class FileDownloaderObject : public BaseObject
	{
	private:
		UDPClientObject& m_udpClient;

		int m_toFinish = 3;
		bool m_done = false;

		int m_fileId = -1;
		ull m_fileSize = 0;
		ull m_bytesReceived = 0;
		ull m_fileKBOffset = 0;
		std::string m_path;

		udp::UDPRes* m_dataReceived = nullptr;

		std::function<void()> m_downloadFinished;

	public:
		FileDownloaderObject(
			UDPClientObject& udpClient,
			const std::string& ipAddr,
			int fileId,
			ull fileSize,
			const std::string& path,
			const std::function<void()>& downloadFinished);

		virtual ~FileDownloaderObject();

		int GetFileId() const;
		void GetProgress(ull& finished, ull& all) const;
	};
}