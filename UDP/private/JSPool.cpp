#include "JSPool.h"

#include "JobSystemMeta.h"

udp::JSPool::JSPool(size_t poolSize, size_t jsSize)
{
	m_mutex.lock();
	
	for (size_t i = 0; i < poolSize; ++i)
	{
		jobs::JobSystem* js = new jobs::JobSystem(jobs::JobSystemMeta::GetInstance(), jsSize);
		m_pool.push_back(js);
	}

	m_mutex.unlock();
}

jobs::JobSystem* udp::JSPool::AcquireJS()
{
	jobs::JobSystem* res = nullptr;

	m_mutex.lock();

	if (!m_pool.empty())
	{
		res = m_pool.front();
		m_pool.pop_front();
	}

	m_mutex.unlock();

	return res;
}

void udp::JSPool::ReleaseJS(jobs::JobSystem* js)
{
	m_mutex.lock();

	m_pool.push_back(js);

	m_mutex.unlock();
}

