#pragma once

#include "UDP.h"

#include "BaseObject.h"
#include "BaseObjectMeta.h"

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

	class FileDownloaderObject : public BaseObject, public Endpoint
	{
	public:
		unsigned int m_fileId;
		Bucket m_bucket;

		ull m_fileSize = 0;
		ull m_received = 0;
		std::string m_path;
		std::string m_serverIP;

		FileDownloaderObject(
			const std::string& serverIP,
			unsigned int fileId,
			ull fileSize,
			const std::string& path);

		virtual ~FileDownloaderObject();

		void Init() override;
	};
}