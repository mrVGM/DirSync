#pragma once

#include "BaseObjectMeta.h"
#include "BaseObject.h"

namespace udp
{
	struct KB
	{
		char m_data[1024] = {};
	};

	struct FileChunk
	{
	private:
		KB* m_data = nullptr;

	public:
		enum KBPos
		{
			Inside,
			Left,
			Right
		};

		static size_t m_chunkKBSize;

		size_t m_startingByte = 0;

		~FileChunk();

		bool GetKB(size_t globalKBPos, KB& outKB);
		KBPos GetKBPos(size_t globalKBPos);

		KB* GetData();
	};

	class UDPResState
	{
	private:
		char m_state;

	public:
		static UDPResState m_empty;
		static UDPResState m_full;
		static UDPResState m_blank;

		UDPResState(char state);

		bool Equals(const UDPResState & other);
	};

	struct UDPReq
	{
		unsigned int m_fileId;
		size_t m_offset;
		unsigned char m_mask[1024] = {};

		void UpBit(unsigned int bitNumber);
		bool GetBitState(unsigned int bitNumber) const;
	};

	struct UDPRes
	{
		UDPResState m_state = UDPResState::m_empty;
		unsigned int m_fileId;
		size_t m_offset;
		KB m_data;
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
}