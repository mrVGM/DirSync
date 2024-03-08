#include "UDP.h"

#include <iostream>

#include <WinSock2.h>

namespace
{
	bool m_udpInitialized = false;
}

udp::PacketType udp::PacketType::m_ping { 'p' };
udp::PacketType udp::PacketType::m_empty { 'e' };
udp::PacketType udp::PacketType::m_full { 'f' };
udp::PacketType udp::PacketType::m_blank { 'b' };
udp::PacketType udp::PacketType::m_bitmask { 'm' };


void udp::KB::UpBit(unsigned int bitNumber)
{
	unsigned int byte = bitNumber / 8;
	unsigned int bitOffset = bitNumber % 8;

	unsigned char byteMask = 1;
	byteMask = byteMask << bitOffset;

	m_data[byte] |= byteMask;
}

bool udp::KB::GetBitState(unsigned int bitNumber) const
{
	unsigned int byte = bitNumber / 8;
	unsigned int bitOffset = bitNumber % 8;

	unsigned char byteMask = 1;
	byteMask = byteMask << bitOffset;

	return (m_data[byte] & byteMask);
}

bool udp::PacketType::Equals(const PacketType& other) const
{
	return m_data == other.m_data;
}

udp::EPacketType udp::PacketType::GetPacketType() const
{
	if (Equals(m_ping))
	{
		return EPacketType::Ping;
	}

	if (Equals(m_empty))
	{
		return EPacketType::Empty;
	}

	if (Equals(m_full))
	{
		return EPacketType::Full;
	}

	if (Equals(m_blank))
	{
		return EPacketType::Blank;
	}

	if (Equals(m_bitmask))
	{
		return EPacketType::Bitmask;
	}

	return EPacketType::Error;
}

udp::Bucket::Bucket(ull id) :
	m_workingList(&m_list1),
	m_id(id)
{
}

void udp::Bucket::PushPacket(const Packet& res)
{
	m_swapListMutex.lock();

	if (res.m_packetType.GetPacketType() != EPacketType::Full || !m_received.contains(res.m_offset))
	{
		m_workingList->push_back(res);
		m_received.insert(res.m_offset);
	}

	m_semaphore.release();

	m_swapListMutex.unlock();
}

std::list<udp::Packet>& udp::Bucket::GetAccumulated()
{
	m_semaphore.acquire();

	m_swapListMutex.lock();

	std::list<udp::Packet>& res = SwapLists();
	m_received.clear();

	m_swapListMutex.unlock();

	return res;
}

std::list<udp::Packet>& udp::Bucket::SwapLists()
{
	std::list<udp::Packet>* cur = m_workingList;
	std::list<udp::Packet>* other = cur == &m_list1 ? &m_list2 : &m_list1;
	m_workingList = other;

	return *cur;
}

udp::Bucket* udp::BucketManager::GetOrCreateBucket(ull id, bool& justCreated)
{
	justCreated = false;
	m_mutex.lock();

	Bucket* res = nullptr;

	auto it = m_buckets.find(id);
	if (it == m_buckets.end())
	{
		res = new Bucket(id);
		m_buckets[id] = res;
		justCreated = true;
	}
	else
	{
		res = it->second;
	}

	m_mutex.unlock();

	return res;
}

udp::BucketManager::~BucketManager()
{
	for (auto it = m_buckets.begin(); it != m_buckets.end(); ++it)
	{
		delete it->second;
	}
}

void udp::Boot()
{
	if (m_udpInitialized)
	{
		return;
	}

	WSADATA wsaData;
	int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res != NO_ERROR) {
		std::cout << "WSAStartup failed with error " << res << std::endl;
	}

	m_udpInitialized = true;
}

