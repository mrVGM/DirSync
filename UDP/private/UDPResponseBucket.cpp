#include "UDPResponseBucket.h"

udp::UDPResponseBucket::UDPResponseBucket() :
	m_workingList(&m_list1)
{
}

void udp::UDPResponseBucket::PushUDPRes(const UDPRes& res)
{
	m_swapListMutex.lock();
	
	if (!m_received.contains(res.m_offset))
	{
		m_workingList->push_back(res);
		m_received.insert(res.m_offset);
	}

	m_semaphore.release();

	m_swapListMutex.unlock();
}

std::list<udp::UDPRes>& udp::UDPResponseBucket::GetAccumulated()
{
	m_semaphore.acquire();

	m_swapListMutex.lock();

	std::list<udp::UDPRes>& res = SwapLists();
	m_received.clear();

	m_swapListMutex.unlock();

	return res;
}

std::list<udp::UDPRes>& udp::UDPResponseBucket::SwapLists()
{
	std::list<udp::UDPRes>* cur = m_workingList;
	std::list<udp::UDPRes>* other = cur == &m_list1 ? &m_list2 : &m_list1;
	m_workingList = other;

	return *cur;
}
