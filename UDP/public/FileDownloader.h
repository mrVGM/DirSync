#pragma once

#include "BaseObjectMeta.h"
#include "BaseObject.h"

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
	public:
		FileDownloaderObject();
		virtual ~FileDownloaderObject();
	};
}