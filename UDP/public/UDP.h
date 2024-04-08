#pragma once

#include "Common.h"

#include <list>
#include <set>
#include <map>

#include <mutex>
#include <semaphore>


namespace udp
{
	struct KB
	{
		char m_data[1024] = {};

		void UpBit(unsigned int bitNumber);
		bool GetBitState(unsigned int bitNumber) const;
	};

	enum EPacketType
	{
		Error,
		Ping,
		Empty,
		Full,
		Blank,
		Bitmask
	};

	struct PacketType
	{
		char m_data = 'p';

		static PacketType m_ping;
		static PacketType m_empty;
		static PacketType m_full;
		static PacketType m_blank;
		static PacketType m_bitmask;

		bool Equals(const PacketType& other) const;
		EPacketType GetPacketType() const;
	};

	struct Packet
	{
		PacketType m_packetType;
		ull m_id;
		ull m_offset;
		ull m_chunkId;
		KB m_payload;
	};

	class Bucket
	{
	private:
		friend class BucketManager;

		ull m_id;
		std::list<Packet> m_list1;
		std::list<Packet> m_list2;

		std::list<Packet>* m_workingList = nullptr;

		std::mutex m_swapListMutex;
		std::binary_semaphore m_semaphore{ 0 };

		std::list<Packet>& SwapLists();

		std::set<ull> m_received;

	public:
		Bucket(ull id);

		void PushPacket(const Packet& res);
		std::list<Packet>& GetAccumulated();
	};

	class BucketManager
	{
	private:
		std::mutex m_mutex;
		std::map<ull, Bucket*> m_buckets;

	public:

		struct BucketContainer
		{
		private:
			friend class BucketManager;
			Bucket* m_bucket = nullptr;
			BucketManager* m_manager = nullptr;

		public:
			Bucket* GetBucket()
			{
				return m_bucket;
			}
			~BucketContainer()
			{
				if (m_manager)
				{
					m_manager->m_mutex.unlock();
				}
			}
		};

		Bucket* GetOrCreateBucket(ull id, bool& justCreated);
		void PushPacket(ull id, const Packet& packet);
		void DestroyBucket(ull id);
		~BucketManager();
	};

	class Endpoint
	{
	protected:
		unsigned __int64 m_socket = 0;

	public:
		virtual void Init() = 0;
	};

	void Boot();
}