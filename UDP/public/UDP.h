#pragma once

#include "BaseObjectMeta.h"
#include "BaseObject.h"

namespace udp
{
	struct UDPReq
	{
		unsigned int m_fileId;
		unsigned int m_offset;
		unsigned char m_mask[1024] = {};

		void UpBit(unsigned int bitNumber);
		bool GetBitState(unsigned int bitNumber);
	};

	struct UDPRes
	{
		unsigned int m_fileId;
		unsigned int m_offset;
		unsigned char m_data[1024] = {};
	};

	class UDPServerJSMeta : public BaseObjectMeta
	{
	public:
		static const UDPServerJSMeta& GetInstance();
		UDPServerJSMeta();
	};

	class UDPServerMeta : public BaseObjectMeta
	{
	public:
		static const UDPServerMeta& GetInstance();
		UDPServerMeta();
	};

	class UDPServerObject : public BaseObject
	{
	public:
		UDPServerObject();
		virtual ~UDPServerObject();
	};

	void Init();
	void UDPServer();
	void UDPClient();
}