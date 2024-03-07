#pragma once

#include "UDP.h"

#include "BaseObject.h"
#include "BaseObjectMeta.h"

namespace udp
{
	class FileServerJSMeta : public BaseObjectMeta
	{
	public:
		static const FileServerJSMeta& GetInstance();
		FileServerJSMeta();
	};

	class FileServerHandlersJSMeta : public BaseObjectMeta
	{
	public:
		static const FileServerHandlersJSMeta& GetInstance();
		FileServerHandlersJSMeta();
	};

	class FileServerMeta : public BaseObjectMeta
	{
	public:
		static const FileServerMeta& GetInstance();
		FileServerMeta();
	};

	class FileServerObject : public BaseObject, public Endpoint
	{
	private:
		BucketManager m_bucketManager;

	public:
		FileServerObject();
		virtual ~FileServerObject();

		void Init() override;
	};
}