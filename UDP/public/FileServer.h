#pragma once

#include "UDP.h"

#include "BaseObject.h"
#include "BaseObjectMeta.h"

#include <set>

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
		std::mutex m_checkWorkingBuckets;
		BucketManager m_bucketManager;
		std::set<ull> m_workingBuckets;

		void StartBucket(ull bucketID);
		bool CheckBucket(ull bucketID);
		int m_port = -1;
	public:
		FileServerObject();
		virtual ~FileServerObject();

		void Init() override;
		void StopBucket(ull bucketID);
		int GetPort() const;
	};
}