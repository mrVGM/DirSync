#pragma once

#include "BaseObjectMeta.h"
#include "BaseObject.h"

#include "UDPResponseBucket.h"

#include <map>
#include <mutex>

namespace udp
{
	class UDPClientJSMeta : public BaseObjectMeta
	{
	public:
		static const UDPClientJSMeta& GetInstance();
		UDPClientJSMeta();
	};

	class FileDownloadersJSMeta : public BaseObjectMeta
	{
	public:
		static const FileDownloadersJSMeta& GetInstance();
		FileDownloadersJSMeta();
	};

	class PingServerJSMeta : public BaseObjectMeta
	{
	public:
		static const PingServerJSMeta& GetInstance();
		PingServerJSMeta();
	};

	class UDPClientMeta : public BaseObjectMeta
	{
	public:
		static const UDPClientMeta& GetInstance();
		UDPClientMeta();
	};

	class UDPClientObject : public BaseObject
	{
	private:
		void* m_clientSock = nullptr;

		std::mutex m_bucketMutex;
		std::map<int, UDPResponseBucket*> m_buckets;

	public:
		UDPClientObject();
		virtual ~UDPClientObject();

		UDPResponseBucket* GetBucket(int id);
		UDPResponseBucket& GetOrCreateBucket(int id);
		void* GetClientSock();
	};
}