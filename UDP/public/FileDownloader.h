#pragma once

#include "UDP.h"

#include "Job.h"

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
		int m_serverPort = -1;

		int m_numWorkers = 1;
		int m_downloadWindow = 1;
		int m_pingDelay = 10;

		jobs::Job* m_done = nullptr;

		FileDownloaderObject(
			int serverPort,
			const std::string& serverIP,
			unsigned int fileId,
			ull fileSize,
			const std::string& path,
			int numWorkers,
			int downloadWindow,
			int pingDelay,
			jobs::Job* done);

		virtual ~FileDownloaderObject();

		void Init() override;
	};
}