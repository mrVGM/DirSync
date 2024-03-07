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
		Bucket m_bucket;

	public:
		FileDownloaderObject();
		virtual ~FileDownloaderObject();

		void Init() override;
	};
}