#pragma once

#include "UDP.h"

#include <list>
#include <set>

#include <mutex>
#include <semaphore>

namespace udp
{
	class UDPResponseBucket
	{
	private:
		std::list<UDPRes> m_list1;
		std::list<UDPRes> m_list2;

		std::list<UDPRes>* m_workingList = nullptr;

		std::mutex m_swapListMutex;
		std::binary_semaphore m_semaphore{ 0 };

		std::list<UDPRes>& SwapLists();

		std::set<size_t> m_received;

	public:
		UDPResponseBucket();

		void PushUDPRes(const UDPRes& res);
		std::list<UDPRes>& GetAccumulated();
	};
}